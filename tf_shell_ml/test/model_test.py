#!/usr/bin/python
#
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
import unittest
import tensorflow as tf
import keras
import numpy as np
import tf_shell
import tf_shell_ml
import tf_shell.python.shell_optimizers as shell_optimizers

# Prepare the dataset.
batch_size = 2**10
(x_train, y_train), (x_test, y_test) = keras.datasets.mnist.load_data()
x_train, x_test = np.reshape(x_train, (-1, 784)), np.reshape(x_test, (-1, 784))
x_train, x_test = x_train / np.float32(255.0), x_test / np.float32(255.0)
y_train, y_test = tf.one_hot(y_train, 10), tf.one_hot(y_test, 10)

train_dataset = tf.data.Dataset.from_tensor_slices((x_train, y_train))
train_dataset = train_dataset.shuffle(buffer_size=2**14).batch(
    batch_size, drop_remainder=True
)

val_dataset = tf.data.Dataset.from_tensor_slices((x_test, y_test))
val_dataset = val_dataset.batch(batch_size, drop_remainder=True)


class TestModel(tf.test.TestCase):
    def test_model(self):
        # Turn on the shell optimizer to use autocontext.
        shell_optimizers.enable_tf_shell_optimizer()

        m = tf_shell_ml.TfShellSequential(
            [
                tf_shell_ml.ShellDense(
                    64,
                    activation=tf_shell_ml.relu,
                    activation_deriv=tf_shell_ml.relu_deriv,
                    use_fast_reduce_sum=True,
                ),
                tf_shell_ml.ShellDense(
                    10,
                    activation=tf.nn.softmax,
                    use_fast_reduce_sum=True,
                ),
            ],
            lambda: tf_shell.create_autocontext64(
                log2_cleartext_sz=32,
                scaling_factor=3,
                noise_offset_log2=64,
            ),
            None,
            None,
            True,
        )

        m.compile(
            shell_loss=tf_shell_ml.CategoricalCrossentropy(),
            optimizer=tf.keras.optimizers.Adam(0.1),
            loss=tf.keras.losses.CategoricalCrossentropy(),
            metrics=[tf.keras.metrics.CategoricalAccuracy()],
        )

        m.build([None, 784])
        # m(train_dataset)
        m.summary()
        history = m.fit(train_dataset, epochs=1, validation_data=val_dataset)


if __name__ == "__main__":
    unittest.main()
