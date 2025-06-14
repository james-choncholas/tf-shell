# Copyright 2023 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
import tensorflow as tf
import tf_shell
from tf_shell_ml.model_base import SequentialBase
from tf_shell_ml import large_tensor


class DpSgdSequential(SequentialBase):
    def call(self, features, training=False, with_softmax=True):
        predictions = features
        for l in self.layers:
            predictions = l(predictions, training=training, split_forward_mode=True)

        if not with_softmax:
            return predictions
        # Perform the last layer activation since it is removed for training
        # purposes.
        return tf.nn.softmax(predictions)

    def _backward(self, dJ_dz, sensitivity_analysis_factor=None):
        # Backward pass. dJ_dz is the derivative of the loss with respect to the
        # last layer pre-activation.
        dJ_dw = []  # Derivatives of the loss with respect to the weights.
        dJ_dx = [dJ_dz]  # Derivatives of the loss with respect to the inputs.
        for l in reversed(self.layers):
            dw, dx = l.backward(
                dJ_dx[-1], sensitivity_analysis_factor=sensitivity_analysis_factor
            )
            dJ_dw.extend(dw)
            dJ_dx.append(dx)

        return [g for g in reversed(dJ_dw)]

    def compute_grads(self, features, enc_labels):
        scaling_factor = (
            enc_labels.scaling_factor
            if hasattr(enc_labels, "scaling_factor")
            else float("inf")
        )
        scaling_factor = tf.cast(scaling_factor, dtype=tf.keras.backend.floatx())

        # Reset layers for forward pass over multiple devices.
        for l in self.layers:
            l.reset_split_forward_mode()

        predictions_list = []
        max_two_norms_list = []

        with tf.device(self.features_party_dev):
            split_features, end_pad = self.split_with_padding(
                features, len(self.jacobian_devices)
            )

        for i, d in enumerate(self.jacobian_devices):
            with tf.device(d):
                f = tf.identity(split_features[i])  # copy to GPU if needed

                # First compute the real prediction. Manually perform the
                # softmax activation if necessary.
                prediction = self.call(
                    f, training=True, with_softmax=self.uses_cce_and_softmax
                )

                # Next perform the sensitivity analysis. Straightforward
                # backpropagation has mul/add depth proportional to the number
                # of layers and the encoding error accumulates through each op.
                # It is difficult to tightly bound and easier to simply compute.
                #
                # Perform backpropagation (in plaintext) for every possible
                # label using the worst casse quantization of weights.
                with tf.name_scope("sensitivity_analysis"):
                    sensitivity = tf.constant(0.0, dtype=tf.keras.backend.floatx())
                    worst_case_prediction = tf_shell.worst_case_rounding(
                        prediction, scaling_factor
                    )

                    def cond(possible_label_i, sensitivity):
                        return possible_label_i < self.out_classes

                    def body(possible_label_i, sensitivity):
                        possible_label = tf.one_hot(
                            possible_label_i,
                            self.out_classes,
                            dtype=tf.keras.backend.floatx(),
                        )
                        dJ_dz = worst_case_prediction - possible_label
                        possible_grads = self._backward(
                            dJ_dz, sensitivity_analysis_factor=scaling_factor
                        )
                        if i == len(self.jacobian_devices) - 1 and end_pad > 0:
                            # The last device's features may have been padded.
                            # Remove the extra gradients before computing the norm.
                            possible_grads = [g[:-end_pad] for g in possible_grads]

                        max_norm = self.max_per_example_global_norm(possible_grads)
                        sensitivity = tf.maximum(sensitivity, max_norm)
                        return possible_label_i + 1, sensitivity

                    # Using a tf.while_loop (vs. a python for loop) is preferred as
                    # it does not encode the unrolled loop into the graph, which may
                    # require lots of memory. The `parallel_iterations` argument
                    # allows explicit control over the loop's parallelism.
                    # Increasing parallel_iterations may be faster at the expense of
                    # memory usage.
                    possible_label_i = tf.constant(0)
                    sensitivity = tf.while_loop(
                        cond,
                        body,
                        [possible_label_i, sensitivity],
                        parallel_iterations=1,
                    )[1]

                    if i == len(self.jacobian_devices) - 1 and end_pad > 0:
                        # The last device's features may have been padded.
                        prediction = prediction[:-end_pad]
                    predictions_list.append(prediction)
                    max_two_norms_list.append(sensitivity)

        with tf.device(self.features_party_dev):
            predictions = tf.concat(predictions_list, axis=0)
            max_two_norm = tf.reduce_max(max_two_norms_list)

            if isinstance(self.loss, tf.keras.losses.CategoricalCrossentropy):
                if type(enc_labels) is tf.Tensor:
                    tf.debugging.assert_equal(
                        tf.shape(predictions),
                        tf.shape(enc_labels),
                        message="Predictions and labels must have the same shape.",
                    )
                # The base class ensures that when the loss is CCE, the last
                # layer's activation is softmax. The derivative of these two
                # functions is simple subtraction.
                dJ_dz = enc_labels.__rsub__(predictions)
                # ^  batch_size x num output classes.

                # In classification, the maximum difference between the
                # prediction and the label is 1.0, thus the max_two_norm
                # does not need to be scaled.
            elif isinstance(self.loss, tf.keras.losses.MeanSquaredError):
                if type(enc_labels) is tf.Tensor:
                    tf.debugging.assert_equal(
                        tf.shape(predictions),
                        tf.shape(enc_labels),
                        message="Predictions and labels must have the same shape.",
                    )
                dJ_dz = enc_labels.__rsub__(predictions)

                # Note: dJ_dz is unbounded. It must either be clipped (which
                # is not implemented) or the noise scale must be accounted for
                # by the caller. A warning is printed in the base class.
            else:
                raise ValueError(
                    "Unsupported loss function. Only CCE and MSE are supported."
                )

            # Backward pass.
            grads = self._backward(dJ_dz)

        return grads, max_two_norm, predictions
