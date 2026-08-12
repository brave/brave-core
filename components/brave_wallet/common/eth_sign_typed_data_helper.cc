/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/eth_sign_typed_data_helper.h"

#include <stddef.h>

#include <algorithm>
#include <optional>
#include <string_view>
#include <utility>

#include "base/check.h"
#include "base/containers/extend.h"
#include "base/containers/queue.h"
#include "base/containers/span.h"
#include "base/numerics/safe_conversions.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
#include "brave/components/brave_wallet/common/hash_utils.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/string_utils.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_set.h"

namespace brave_wallet {

namespace {

constexpr char kTypeField[] = "type";
constexpr char kNameField[] = "name";

std::optional<std::vector<std::pair<std::string_view, const base::ListValue*>>>
FindAndValidateAllDependencyTypes(const base::DictValue& types,
                                  const std::string_view primary_type_name) {
  std::vector<std::pair<std::string_view, const base::ListValue*>> result;

  absl::flat_hash_set<std::string_view> visited_types;

  base::queue<std::string_view> types_queue;
  types_queue.push(primary_type_name);

  while (!types_queue.empty()) {
    auto type_name = types_queue.front();
    types_queue.pop();

    if (visited_types.contains(type_name)) {
      continue;
    }

    const auto* member_list = types.FindList(type_name);
    if (!member_list) {
      // Reference implementation just ignores unknown types and continues to
      // encode the rest of the data.
      // https://github.com/MetaMask/eth-sig-util/blob/0832d49b7c2f6d48d22a4496faee3e393081d1ec/src/sign-typed-data.ts#L436
      continue;
    }
    visited_types.emplace(type_name);
    result.emplace_back(type_name, member_list);

    for (const auto& member : *member_list) {
      if (!member.is_dict()) {
        return std::nullopt;
      }
      const std::string* type_str = member.GetDict().FindString(kTypeField);
      const std::string* name_str = member.GetDict().FindString(kNameField);
      if (!type_str || !name_str) {
        return std::nullopt;
      }

      std::string_view lookup_type = *type_str;
      if (auto split_array_part = base::SplitStringOnce(*type_str, "[")) {
        lookup_type = split_array_part->first;
      }

      if (!visited_types.contains(lookup_type)) {
        types_queue.push(lookup_type);
      }
    }
  }

  return result;
}

// https://eips.ethereum.org/EIPS/eip-712#definition-of-encodetype
void EncodeType(std::string& encode_to,
                const std::string_view type_name,
                const base::ListValue& member_list) {
  base::StrAppend(&encode_to, {type_name, "("});

  for (auto& member : member_list) {
    CHECK(member.is_dict());

    const std::string* type_str = member.GetDict().FindString(kTypeField);
    const std::string* name_str = member.GetDict().FindString(kNameField);
    CHECK(type_str);
    CHECK(name_str);

    base::StrAppend(&encode_to, {*type_str, " ", *name_str, ","});
  }
  if (!member_list.empty()) {
    CHECK_EQ(encode_to.back(), ',');
    encode_to.pop_back();  // Trim last comma.
  }
  base::StrAppend(&encode_to, {")"});
}

}  // namespace

// static
std::unique_ptr<EthSignTypedDataHelper> EthSignTypedDataHelper::Create(
    base::DictValue types,
    Version version) {
  return std::unique_ptr<EthSignTypedDataHelper>(
      new EthSignTypedDataHelper(std::move(types), version));
}

EthSignTypedDataHelper::EthSignTypedDataHelper(base::DictValue types,
                                               Version version)
    : types_(std::move(types)), version_(version) {}

EthSignTypedDataHelper::~EthSignTypedDataHelper() = default;

void EthSignTypedDataHelper::SetTypes(base::DictValue types) {
  types_ = std::move(types);
}

void EthSignTypedDataHelper::SetVersion(Version version) {
  version_ = version;
}

// https://eips.ethereum.org/EIPS/eip-712#definition-of-encodetype
std::optional<std::string> EthSignTypedDataHelper::EncodeTypes(
    const std::string_view primary_type_name) const {
  std::string result;

  auto types_map = FindAndValidateAllDependencyTypes(types_, primary_type_name);
  if (!types_map) {
    return std::nullopt;
  }

  // Primary type comes first, then the rest of the types in alphabetical order.
  std::ranges::sort(*types_map, [=](const auto& a, const auto& b) {
    if (a.first == primary_type_name) {
      return true;
    }
    if (b.first == primary_type_name) {
      return false;
    }
    return a.first < b.first;
  });
  for (const auto& type : *types_map) {
    EncodeType(result, type.first, *type.second);
  }
  return result;
}

std::optional<EthSignTypedDataHelper::Eip712HashArray>
EthSignTypedDataHelper::GetTypeHash(
    const std::string_view primary_type_name) const {
  auto encode_types = EncodeTypes(primary_type_name);
  if (!encode_types) {
    return std::nullopt;
  }
  return KeccakHash(base::as_byte_span(*encode_types));
}

std::optional<
    std::pair<EthSignTypedDataHelper::Eip712HashArray, base::DictValue>>
EthSignTypedDataHelper::HashStruct(const std::string_view primary_type_name,
                                   const base::DictValue& data) const {
  auto encoded_data = EncodeData(primary_type_name, data);
  if (!encoded_data) {
    return std::nullopt;
  }
  return std::make_pair(KeccakHash(encoded_data->first),
                        std::move(encoded_data->second));
}

// Encode the json data by the its type defined in json custom types starting
// from primary type. See unittests for some examples.
std::optional<std::pair<std::vector<uint8_t>, base::DictValue>>
EthSignTypedDataHelper::EncodeData(const std::string_view primary_type_name,
                                   const base::DictValue& data) const {
  const auto* primary_type = types_.FindList(primary_type_name);
  if (!primary_type) {
    return std::nullopt;
  }
  std::vector<uint8_t> result;
  // 32 bytes for type hash and for each item in schema.
  result.reserve(Eip712HashArray().size() * (1 + primary_type->size()));

  auto type_hash = GetTypeHash(primary_type_name);
  if (!type_hash) {
    return std::nullopt;
  }

  base::Extend(result, *type_hash);

  base::DictValue sanitized_data;

  for (const auto& item : *primary_type) {
    const auto& field = item.GetDict();
    const std::string* type_str = field.FindString(kTypeField);
    const std::string* name_str = field.FindString(kNameField);
    if (!type_str || !name_str) {
      return std::nullopt;
    }
    const base::Value* value = data.Find(*name_str);
    if (value) {
      auto encoded_field = EncodeField(*type_str, *value);
      if (!encoded_field) {
        return std::nullopt;
      }
      base::Extend(result, *encoded_field);
      sanitized_data.Set(*name_str, value->Clone());
    } else {
      if (version_ == Version::kV4) {
        // https://github.com/MetaMask/eth-sig-util/blob/66a8c0935c14d6ef80b583148d0c758c198a9c4a/src/sign-typed-data.ts#L248
        // Insert null line in case of a missing field.
        result.insert(result.end(), 32, 0);
      }
    }
  }

  return std::make_pair(result, std::move(sanitized_data));
}

// Encode each field of a custom type, if a field is also a custom type it
// will call EncodeData recursively until it reaches an atomic type
std::optional<EthSignTypedDataHelper::Eip712HashArray>
EthSignTypedDataHelper::EncodeField(const std::string_view type,
                                    const base::Value& value) const {
  // ES6 section 20.1.2.6 Number.MAX_SAFE_INTEGER
  constexpr double kMaxSafeInteger = static_cast<double>(kMaxSafeIntegerUint64);

  if (type.ends_with(']')) {
    if (version_ != Version::kV4) {
      return std::nullopt;
    }
    if (!value.is_list()) {
      return std::nullopt;
    }
    const auto type_split = base::SplitStringPiece(
        type, "[", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);
    if (type_split.size() != 2) {
      return std::nullopt;
    }
    std::vector<uint8_t> array_result;
    for (const auto& item : value.GetList()) {
      auto encoded_item = EncodeField(type_split[0], item);
      if (!encoded_item) {
        return std::nullopt;
      }
      base::Extend(array_result, *encoded_item);
    }
    return KeccakHash(array_result);
  }

  if (type == "string") {
    const std::string* value_str = value.GetIfString();
    if (!value_str) {
      return std::nullopt;
    }
    return KeccakHash(base::as_byte_span(*value_str));
  }

  if (type == "bytes") {
    const std::string* value_str = value.GetIfString();
    if (!value_str || (!value_str->empty() && !IsValidHexString(*value_str))) {
      return std::nullopt;
    }
    std::vector<uint8_t> bytes;
    if (!value_str->empty()) {
      CHECK(PrefixedHexStringToBytes(*value_str, &bytes));
    }
    return KeccakHash(bytes);
  }

  if (type == "bool") {
    std::optional<bool> value_bool = value.GetIfBool();
    if (!value_bool) {
      return std::nullopt;
    }

    Eip712HashArray result = {};
    result.back() = value_bool.value() ? 1 : 0;
    return result;
  }

  if (type == "address") {
    const std::string* value_str = value.GetIfString();
    if (!value_str || !IsValidHexString(*value_str)) {
      return std::nullopt;
    }
    std::vector<uint8_t> address;
    CHECK(PrefixedHexStringToBytes(*value_str, &address));
    if (address.size() != 20u) {
      return std::nullopt;
    }

    Eip712HashArray result = {};
    base::as_writable_byte_span(result).last(20u).copy_from(address);
    return result;
  }

  if (type.starts_with("bytes")) {
    unsigned num_bits;
    if (!base::StringToUint(type.substr(5), &num_bits) || num_bits > 32) {
      return std::nullopt;
    }
    const std::string* value_str = value.GetIfString();
    if (!value_str || !IsValidHexString(*value_str)) {
      return std::nullopt;
    }
    std::vector<uint8_t> bytes;
    CHECK(PrefixedHexStringToBytes(*value_str, &bytes));
    if (bytes.size() > 32) {
      return std::nullopt;
    }
    Eip712HashArray result = {};
    base::as_writable_byte_span(result).copy_prefix_from(bytes);
    return result;
  }

  if (type.starts_with("uint")) {
    // uint8 to uint256 in steps of 8
    unsigned num_bits;
    if (!base::StringToUint(type.substr(4), &num_bits) ||
        !ValidSolidityBits(num_bits)) {
      return std::nullopt;
    }

    std::optional<double> value_double = value.GetIfDouble();
    const std::string* value_str = value.GetIfString();
    uint256_t encoded_value = 0;
    if (value_double) {
      encoded_value = (uint256_t)(uint64_t)*value_double;
      if (encoded_value > (uint256_t)kMaxSafeInteger) {
        return std::nullopt;
      }
    } else if (value_str) {
      if (!value_str->empty()) {
        if (!HexValueToUint256(*value_str, &encoded_value)) {
          if (auto v = Base10ValueToUint256(*value_str)) {
            encoded_value = *v;
          } else {
            return std::nullopt;
          }
        }
      }
    } else {
      return std::nullopt;
    }

    std::optional<uint256_t> max_value = MaxSolidityUint(num_bits);
    if (max_value == std::nullopt || encoded_value > *max_value) {
      return std::nullopt;
    }

    Eip712HashArray result = {};
    base::span(result).copy_from(base::byte_span_from_ref(encoded_value));
    std::ranges::reverse(result);

    return result;
  }

  if (type.starts_with("int")) {
    // int8 to int256 in steps of 8
    unsigned num_bits;
    if (!base::StringToUint(type.substr(3), &num_bits) ||
        !ValidSolidityBits(num_bits)) {
      return std::nullopt;
    }
    std::optional<double> value_double = value.GetIfDouble();
    const std::string* value_str = value.GetIfString();
    int256_t encoded_value = 0;
    if (value_double) {
      encoded_value = (int256_t)(int64_t)*value_double;
      if (encoded_value > (int256_t)kMaxSafeInteger) {
        return std::nullopt;
      }
    } else if (value_str) {
      if (!value_str->empty()) {
        if (!HexValueToInt256(*value_str, &encoded_value)) {
          if (auto v = Base10ValueToInt256(*value_str)) {
            encoded_value = *v;
          } else {
            return std::nullopt;
          }
        }
      }
    } else {
      return std::nullopt;
    }

    std::optional<int256_t> min_value = MinSolidityInt(num_bits);
    std::optional<int256_t> max_value = MaxSolidityInt(num_bits);
    if (min_value == std::nullopt || max_value == std::nullopt ||
        encoded_value > *max_value || encoded_value < *min_value) {
      return std::nullopt;
    }

    Eip712HashArray result = {};
    base::span(result).copy_from(base::byte_span_from_ref(encoded_value));
    std::ranges::reverse(result);

    return result;
  }

  if (!value.is_dict()) {
    return std::nullopt;
  }
  auto encoded_data = EncodeData(type, value.GetDict());
  if (!encoded_data) {
    return std::nullopt;
  }
  return KeccakHash(encoded_data->first);
}

std::optional<
    std::pair<EthSignTypedDataHelper::Eip712HashArray, base::DictValue>>
EthSignTypedDataHelper::GetTypedDataDomainHash(
    const base::DictValue& domain) const {
  return HashStruct("EIP712Domain", domain);
}

std::optional<
    std::pair<EthSignTypedDataHelper::Eip712HashArray, base::DictValue>>
EthSignTypedDataHelper::GetTypedDataPrimaryHash(
    const std::string& primary_type_name,
    const base::DictValue& message) const {
  return HashStruct(primary_type_name, message);
}

// static
EthSignTypedDataHelper::Eip712HashArray
EthSignTypedDataHelper::GetTypedDataMessageToSign(
    base::span<const uint8_t> domain_hash,
    base::span<const uint8_t> primary_hash) {
  DCHECK(!domain_hash.empty());
  DCHECK(!primary_hash.empty());

  std::vector<uint8_t> encoded_data({0x19, 0x01});
  base::Extend(encoded_data, domain_hash);
  base::Extend(encoded_data, primary_hash);

  return KeccakHash(encoded_data);
}

}  // namespace brave_wallet
