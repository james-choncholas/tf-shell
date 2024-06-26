// Copyright 2023 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "shape_inference.h"
#include "tensorflow/core/framework/common_shape_fns.h"
#include "tensorflow/core/framework/op.h"
#include "tensorflow/core/framework/shape_inference.h"

using tensorflow::OkStatus;
using tensorflow::errors::InvalidArgument;
using tensorflow::shape_inference::DimensionHandle;
using tensorflow::shape_inference::InferenceContext;
using tensorflow::shape_inference::ScalarShape;
using tensorflow::shape_inference::ShapeHandle;
using tensorflow::shape_inference::UnchangedShape;

// Tensorflow does not have size_t but Shell Context parameters require it.
// Code below must assume size_t is a unit64 because of this.
static_assert(sizeof(size_t) == sizeof(uint64_t), "Fatal size mismatch");

REGISTER_OP("ContextImport64")
    .Input("log_n: uint64")
    .Input("main_moduli: uint64")
    .Input("aux_moduli: uint64")
    .Input("plaintext_modulus: uint64")
    .Input("noise_variance: uint64")
    .Input("seed: string")
    .Output("shell_context: variant")
    .SetShapeFn(ScalarShape);

REGISTER_OP("PolynomialImport64")
    .Attr(
        "Dtype: {uint8, int8, int16, uint16, int32, uint32, int64, uint64, "
        "float, double}")
    .Input("shell_context: variant")
    .Input("in: Dtype")
    .Output("val: variant")
    .SetShapeFn(ImportAndRemoveBatchingDimShape);

REGISTER_OP("PolynomialExport64")
    .Attr("dtype: {uint8, int8, uint16, int16, uint32, int32, uint64, int64}")
    .Attr("batching_dim: int")
    .Input("shell_context: variant")
    .Input("in: variant")
    .Output("val: dtype")
    .SetShapeFn(ExportAndAddBatchingDimShape<1>);

REGISTER_OP("KeyGen64")
    .Input("context: variant")
    .Output("key: variant")
    .SetShapeFn(ScalarShape);

REGISTER_OP("Encrypt64")
    .Input("context: variant")
    .Input("key: variant")
    .Input("val: variant")
    .Output("out: variant")
    .SetShapeFn(UnchangedArgShape<2>);

REGISTER_OP("Decrypt64")
    .Attr("dtype: {uint8, int8, uint16, int16, uint32, int32, uint64, int64}")
    .Attr("batching_dim: int")
    .Input("context: variant")
    .Input("key: variant")
    .Input("val: variant")
    .Output("out: dtype")
    .SetShapeFn(ExportAndAddBatchingDimShape<2>);

// Add and subtract.
REGISTER_OP("AddCtCt64")
    .Input("context: variant")
    .Input("a: variant")
    .Input("b: variant")
    .Output("c: variant")
    .SetShapeFn(ShellBroadcastingOpShape);

REGISTER_OP("AddCtPt64")
    .Input("context: variant")
    .Input("a: variant")
    .Input("b: variant")
    .Output("c: variant")
    .SetShapeFn(ShellBroadcastingOpShape);

REGISTER_OP("AddPtPt64")
    .Input("context: variant")
    .Input("a: variant")
    .Input("b: variant")
    .Output("c: variant")
    .SetShapeFn(ShellBroadcastingOpShape);

REGISTER_OP("SubCtCt64")
    .Input("context: variant")
    .Input("a: variant")
    .Input("b: variant")
    .Output("c: variant")
    .SetShapeFn(ShellBroadcastingOpShape);

REGISTER_OP("SubCtPt64")
    .Input("context: variant")
    .Input("a: variant")
    .Input("b: variant")
    .Output("c: variant")
    .SetShapeFn(ShellBroadcastingOpShape);

REGISTER_OP("SubPtPt64")
    .Input("context: variant")
    .Input("a: variant")
    .Input("b: variant")
    .Output("c: variant")
    .SetShapeFn(ShellBroadcastingOpShape);

REGISTER_OP("NegCt64")
    .Input("context: variant")
    .Input("value: variant")
    .Output("negated_value: variant")
    .SetShapeFn(UnchangedArgShape<1>);

REGISTER_OP("NegPt64")
    .Input("context: variant")
    .Input("value: variant")
    .Output("negated_value: variant")
    .SetShapeFn(UnchangedArgShape<1>);

// Multiply.
REGISTER_OP("MulCtCt64")
    .Input("context: variant")
    .Input("a: variant")
    .Input("b: variant")
    .Output("c: variant")
    .SetShapeFn(ShellBroadcastingOpShape);

REGISTER_OP("MulCtPt64")
    .Input("context: variant")
    .Input("a: variant")
    .Input("b: variant")
    .Output("c: variant")
    .SetShapeFn(ShellBroadcastingOpShape);

REGISTER_OP("MulCtTfScalar64")
    .Attr("Dtype: {uint8, int8, uint16, int16, uint32, int32, uint64, int64}")
    .Input("context: variant")
    .Input("a: variant")
    .Input("b: Dtype")
    .Output("c: variant")
    .SetShapeFn(UnchangedArgShape<1>);

REGISTER_OP("MulPtTfScalar64")
    .Attr("Dtype: {uint8, int8, uint16, int16, uint32, int32, uint64, int64}")
    .Input("context: variant")
    .Input("a: variant")
    .Input("b: Dtype")
    .Output("c: variant")
    .SetShapeFn(UnchangedArgShape<1>);

REGISTER_OP("MulPtPt64")
    .Input("context: variant")
    .Input("a: variant")
    .Input("b: variant")
    .Output("c: variant")
    .SetShapeFn(ShellBroadcastingOpShape);

REGISTER_OP("MatMulCtPt64")
    .Attr("Dtype: {uint8, int8, uint16, int16, uint32, int32, uint64, int64}")
    .Input("context: variant")
    .Input("a: variant")
    .Input("b: Dtype")
    .Output("c: variant")
    .SetShapeFn(ShellMatMulCtPtShape);

REGISTER_OP("MatMulPtCt64")
    .Attr("Dtype: {uint8, int8, uint16, int16, uint32, int32, uint64, int64}")
    .Input("context: variant")
    .Input("rotation_key: variant")
    .Input("a: Dtype")
    .Input("b: variant")
    .Output("c: variant")
    .SetShapeFn(ShellMatMulPtCtShape);

// Rotate.
REGISTER_OP("RotationKeyGen64")
    .Input("context: variant")
    .Input("key: variant")
    .Output("rotation_key: variant")
    .SetShapeFn(ScalarShape);

REGISTER_OP("Roll64")
    .Input("rotation_key: variant")
    .Input("value: variant")
    .Input("shift: int64")
    .Output("rotated_value: variant")
    .SetShapeFn(UnchangedArgShape<1>);

REGISTER_OP("ReduceSumByRotation64")
    .Input("value: variant")
    .Input("rotation_key: variant")
    .Output("repeated_reduce_sum: variant")
    .SetShapeFn(UnchangedShape);

REGISTER_OP("ReduceSum64")
    .Input("value: variant")
    .Attr("axis: int")
    .Output("repeated_reduce_sum: variant")
    .SetShapeFn([](InferenceContext* c) {
      tsl::int32 rank = c->Rank(c->input(0));

      tsl::int32 axis;
      TF_RETURN_IF_ERROR(c->GetAttr("axis", &axis));

      // Check that axis is in the correct range.
      if (axis == 0) {
        return InvalidArgument(
            "axis may not be zero. See ReduceSumByRotation()");
      }

      // Recall first dimension of a shell variant tensor is the packing
      // dimension.
      int clamped_axis = axis;
      if (clamped_axis < 0) {
        clamped_axis += rank + 1;
      } else if (clamped_axis > 0) {
        clamped_axis -= 1;
      }

      if (clamped_axis < 0 || clamped_axis > rank) {
        return InvalidArgument("axis must be in the range [0, rank], got ",
                               clamped_axis);
      }

      ShapeHandle output;

      // If this op ever supports keepdim=True, use the following shape.
      //  DimensionHandle reduced_dim = c->MakeDim({1});
      //  TF_RETURN_IF_ERROR(c->ReplaceDim(c->input(0), clamped_axis,
      //  reduced_dim, &output));

      // This op currently only supports keepdim=False whose shape is computed
      // via the following.
      ShapeHandle prefix;
      TF_RETURN_IF_ERROR(c->Subshape(c->input(0), 0, clamped_axis, &prefix));

      ShapeHandle postfix;
      TF_RETURN_IF_ERROR(
          c->Subshape(c->input(0), clamped_axis + 1, rank, &postfix));

      if (clamped_axis == 0) {
        output = postfix;
      } else if (clamped_axis == rank - 1) {
        output = prefix;
      } else {
        TF_RETURN_IF_ERROR(c->Concatenate(prefix, postfix, &output));
      }

      c->set_output(0, output);
      return OkStatus();
    });

// Modulus switching.
REGISTER_OP("ModulusReduceContext64")
    .Input("context: variant")
    .Output("reduced_context: variant")
    .SetShapeFn(ScalarShape);

REGISTER_OP("ModulusReduceKey64")
    .Input("key: variant")
    .Output("reduced_key: variant")
    .SetShapeFn(ScalarShape);

REGISTER_OP("ModulusReduceCt64")
    .Input("context: variant")
    .Input("value: variant")
    .Output("reduced_value: variant")
    .SetShapeFn(UnchangedArgShape<1>);

REGISTER_OP("ModulusReducePt64")
    .Input("context: variant")
    .Input("value: variant")
    .Output("reduced_value: variant")
    .SetShapeFn(UnchangedArgShape<1>);

// Shape kernels.
REGISTER_OP("ExpandDimsVariant")
    .Input("value: variant")
    .Attr("axis: int")
    .Output("expanded_value: variant")
    .SetShapeFn([](InferenceContext* c) {
      tsl::int32 rank = c->Rank(c->input(0));

      tsl::int32 axis;
      TF_RETURN_IF_ERROR(c->GetAttr("axis", &axis));

      // Check that axis is in the correct range.
      if (axis < -rank || axis > rank) {
        return InvalidArgument("axis must be in the range [-rank, rank], got ",
                               axis);
      }

      if (axis < 0) {
        axis += rank;
      }

      ShapeHandle prefix;
      TF_RETURN_IF_ERROR(c->Subshape(c->input(0), 0, axis, &prefix));

      ShapeHandle postfix;
      TF_RETURN_IF_ERROR(c->Subshape(c->input(0), axis, rank - 1, &postfix));

      ShapeHandle output;
      ShapeHandle axis_dim = c->MakeShape({1});
      TF_RETURN_IF_ERROR(c->Concatenate(prefix, axis_dim, &output));
      TF_RETURN_IF_ERROR(c->Concatenate(output, postfix, &output));

      c->set_output(0, output);
      return OkStatus();
    });

// Segment sum where the segment_ids are plaintexts.
// Based on :
// https://github.com/tensorflow/tensorflow/blob/dfdba938a0048611319ce192d8f17639e058ad00/tensorflow/core/ops/math_ops.cc#L1293
REGISTER_OP("UnsortedCtSegmentSum")
    .Input("shell_context: variant")
    .Input("data: variant")
    .Input("segment_ids: Tindices")
    .Input("num_segments: Tnumsegments")
    .Input("rotation_key: variant")
    .Output("output: variant")
    .Attr("Tindices: {int32,int64}")
    .Attr("Tnumsegments: {int32,int64} = DT_INT32")
    .SetShapeFn(ShellSegmentReductionWithNumSegmentsShape);
