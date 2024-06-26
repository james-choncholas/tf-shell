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
import tf_shell.python.ops.shell_ops as shell_ops
from tf_shell.python.shell_context import ShellContext64
from tf_shell.python.shell_context import mod_reduce_context64
import tensorflow as tf
import typing


class ShellKey64(tf.experimental.ExtensionType):
    _raw_key: tf.Tensor
    level: int


def create_key64(context):
    if not isinstance(context, ShellContext64):
        raise ValueError("context must be a ShellContext64")

    return ShellKey64(
        _raw_key=shell_ops.key_gen64(context._raw_context), level=context.level
    )


def mod_reduce_key64(key):
    if not isinstance(key, ShellKey64):
        raise ValueError("key must be a ShellKey64")

    smaller_raw_key = shell_ops.modulus_reduce_key64(key._raw_key)
    mod_reduced = ShellKey64(smaller_raw_key, key.level - 1)
    return mod_reduced


class ShellRotationKey64(tf.experimental.ExtensionType):
    _raw_rot_keys_at_level: typing.Mapping[int, tf.Tensor]
    context: ShellContext64

    def _get_key_at_level(self, level):
        if level not in self._raw_rot_keys_at_level:
            raise ValueError(f"No rotation key at level {level}.")
        return self._raw_rot_keys_at_level[level]


def create_rotation_key64(context, key, skip_at_mul_depth=[]):
    """Create rotation keys for any multiplicative depth of the given context.
    Rotation key contains keys to perform an arbitrary number of slot rotations.
    Since rotation key generation is expensive, the caller can choose to skip
    generating keys at levels (particular number of moduli) at which no
    rotations are required."""
    if not isinstance(context, ShellContext64):
        raise ValueError("context must be a ShellContext64.")

    if context.level != key.level:
        raise ValueError("Context and key levels must match.")

    raw_rot_keys_at_level = {}
    while context.mul_depth_supported >= 0:
        if context.mul_depth_supported not in skip_at_mul_depth:
            raw_rot_keys_at_level[context.level] = shell_ops.rotation_key_gen64(
                context._raw_context, key._raw_key
            )

        if context.mul_depth_supported == 0 or context.level == 1:
            break
        context = mod_reduce_context64(context)
        key = mod_reduce_key64(key)

    return ShellRotationKey64(
        _raw_rot_keys_at_level=raw_rot_keys_at_level, context=context
    )
