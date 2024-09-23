/*
 * Copyright 2023 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "shell_encryption/rns/rns_galois_key.h"
#include "tensorflow/core/framework/variant.h"
#include "tensorflow/core/framework/variant_encode_decode.h"
#include "tensorflow/core/framework/variant_op_registry.h"
#include "tensorflow/core/framework/variant_tensor_data.h"

using tensorflow::VariantTensorData;

template <typename T>
class RotationKeyVariant {
  using ModularInt = rlwe::MontgomeryInt<T>;
  using Gadget = rlwe::RnsGadget<ModularInt>;
  using RotationKey = rlwe::RnsGaloisKey<ModularInt>;

 public:
  RotationKeyVariant() {}

  // Create with gadget first, then create and add keys.
  RotationKeyVariant(Gadget gadget) : gadget(gadget) {}

  static inline char const kTypeName[] = "ShellRotationKeyVariant";

  std::string TypeName() const { return kTypeName; }

  // TODO(jchoncholas): implement for networking
  void Encode(VariantTensorData* data) const {};

  // TODO(jchoncholas): implement for networking
  bool Decode(VariantTensorData const& data) { return false; };

  std::string DebugString() const { return "ShellRotationKeyVariant"; }

  Gadget gadget;
  std::vector<RotationKey> keys;
};

template <typename T>
class SingleRotationKeyVariant {
  using ModularInt = rlwe::MontgomeryInt<T>;
  using RotationKey = rlwe::RnsGaloisKey<ModularInt>;

 public:
  SingleRotationKeyVariant() {}

  // Create with gadget first, then create and add keys.
  SingleRotationKeyVariant(RotationKey key) : key(key) {}

  static inline char const kTypeName[] = "SingleRotationKeyVariant";

  std::string TypeName() const { return kTypeName; }

  // Individual keys are never sent over the network.
  void Encode(VariantTensorData* data) const {};

  // Individual keys are never sent over the network.
  bool Decode(VariantTensorData const& data) { return false; };

  std::string DebugString() const { return "SingleRotationKeyVariant"; }

  RotationKey key;
};

template <typename T>
class FastRotationKeyVariant {
  using ModularInt = rlwe::MontgomeryInt<T>;
  using Context = rlwe::RnsContext<ModularInt>;
  using RnsPolynomial = rlwe::RnsPolynomial<ModularInt>;
  using PrimeModulus = rlwe::PrimeModulus<ModularInt>;

 public:
  FastRotationKeyVariant() {}

  // Create with gadget first, then create and add keys.
  FastRotationKeyVariant(std::vector<RnsPolynomial> keys,
                         std::shared_ptr<Context const> ct_context_)
      : keys(std::move(keys)), ct_context(ct_context_) {}

  static inline char const kTypeName[] = "ShellFastRotationKeyVariant";

  std::string TypeName() const { return kTypeName; }

  void Encode(VariantTensorData* data) const {
    data->tensors_.reserve(keys.size());

    for (auto const& key : keys) {
      auto serialized_key_or = key.Serialize(ct_context->MainPrimeModuli());
      if (!serialized_key_or.ok()) {
        std::cout << "ERROR: Failed to serialize key: "
                  << serialized_key_or.status();
        return;
      }
      std::string serialized_key;
      serialized_key_or.value().SerializeToString(&serialized_key);
      data->tensors_.push_back(Tensor(serialized_key));
    }
  };

  bool Decode(VariantTensorData const& data) {
    if (data.tensors_.size() < 1) {
      std::cout << "ERROR: Not enough tensors to deserialize fast rotation key."
                << std::endl;
      return false;
    }

    if (!key_strs.empty()) {
      std::cout << "ERROR: Fast rotation key already decoded." << std::endl;
      return false;
    }

    size_t num_keys = data.tensors_.size();
    key_strs.reserve(num_keys);

    for (size_t i = 0; i < data.tensors_.size(); ++i) {
      std::string const serialized_key(
          data.tensors_[i].scalar<tstring>()().begin(),
          data.tensors_[i].scalar<tstring>()().end());

      key_strs.push_back(std::move(serialized_key));
    }

    return true;
  };

  Status MaybeLazyDecode(std::shared_ptr<Context const> ct_context_) {
    // If the keys have already been fully decoded, nothing to do.
    if (key_strs.empty()) {
      return OkStatus();
    }

    for (auto const& key_str : key_strs) {
      rlwe::SerializedRnsPolynomial serialized_key;
      bool ok = serialized_key.ParseFromString(key_str);
      if (!ok) {
        return InvalidArgument("Failed to parse fast rotation key polynomial.");
      }

      // Using the moduli, reconstruct the key polynomial.
      TF_ASSIGN_OR_RETURN(auto key_polynomial,
                          rlwe::RnsPolynomial<ModularInt>::Deserialize(
                              serialized_key, ct_context_->MainPrimeModuli()));

      keys.push_back(std::move(key_polynomial));
    }

    // Hold a pointer to the context for future encoding.
    ct_context = ct_context_;

    // Clear the key strings.
    key_strs.clear();

    return OkStatus();
  };

  std::string DebugString() const { return "ShellFastRotationKeyVariant"; }

  std::vector<RnsPolynomial> keys;
  std::vector<std::string> key_strs;
  std::shared_ptr<Context const> ct_context;
};