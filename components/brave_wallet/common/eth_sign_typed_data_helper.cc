/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/eth_sign_typed_data_helper.h"

#include <optional>
#include <string_view>
#include <utility>

#include "base/containers/extend.h"
#include "base/containers/span.h"
#include "base/logging.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
#include "brave/components/brave_wallet/common/hash_utils.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/string_utils.h"

namespace brave_wallet {

// static
std::unique_ptr<EthSignTypedDataHelper> EthSignTypedDataHelper::Create(
    base::Value::Dict types,
    Version version) {
  return std::unique_ptr<EthSignTypedDataHelper>(
      new EthSignTypedDataHelper(std::move(types), version));
}

EthSignTypedDataHelper::EthSignTypedDataHelper(base::Value::Dict types,
                                               Version version)
    : types_(std::move(types)), version_(version) {}

EthSignTypedDataHelper::~EthSignTypedDataHelper() = default;

void EthSignTypedDataHelper::SetTypes(base::Value::Dict types) {
  types_ = std::move(types);
}

void EthSignTypedDataHelper::SetVersion(Version version) {
  version_ = version;
}

void EthSignTypedDataHelper::FindAllDependencyTypes(
    base::flat_map<std::string, base::Value>* known_types,
    const std::string_view anchor_type_name) const {
  DCHECK(!anchor_type_name.empty());
  DCHECK(known_types);

  const auto* anchor_type = types_.FindList(anchor_type_name);
  if (!anchor_type) {
    return;
  }
  known_types->emplace(anchor_type_name, anchor_type->Clone());

  for (const auto& field : *anchor_type) {
    if (!field.is_dict()) {
      // EncodeType will fail for this anchor type, there is no need to continue
      // finding.
      return;
    }
    const std::string* type = field.GetDict().FindString("type");
    if (type) {
      const auto type_split = base::SplitStringPiece(
          *type, "[", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);
      std::string_view lookup_type = *type;
      if (type_split.size() == 2) {
        lookup_type = type_split[0];
      }

      if (!known_types->contains(lookup_type)) {
        FindAllDependencyTypes(known_types, lookup_type);
      }
    }
  }
}

std::string EthSignTypedDataHelper::EncodeType(
    const base::Value& type,
    const std::string_view type_name) const {
  if (!type.is_list()) {
    return std::string();
  }
  std::string result = base::StrCat({type_name, "("});

  for (size_t i = 0; i < type.GetList().size(); ++i) {
    if (!type.GetList()[i].is_dict()) {
      return std::string();
    }
    const base::Value::Dict& root = type.GetList()[i].GetDict();
    const std::string* type_str = root.FindString("type");
    const std::string* name_str = root.FindString("name");
    if (!type_str || !name_str) {
      return std::string();
    }
    base::StrAppend(&result, {*type_str, " ", *name_str});
    if (i != type.GetList().size() - 1) {
      base::StrAppend(&result, {","});
    }
  }
  base::StrAppend(&result, {")"});
  return result;
}

std::string EthSignTypedDataHelper::EncodeTypes(
    const std::string_view primary_type_name) const {
  std::string result;

  base::flat_map<std::string, base::Value> types_map;
  FindAllDependencyTypes(&types_map, primary_type_name);

  auto it = types_map.find(primary_type_name);
  if (it != types_map.end()) {
    base::StrAppend(&result, {EncodeType(it->second, primary_type_name)});
  }
  for (const auto& type : types_map) {
    if (type.first == primary_type_name) {
      continue;
    }
    base::StrAppend(&result, {EncodeType(type.second, type.first)});
  }
  return result;
}

EthSignTypedDataHelper::Eip712HashArray EthSignTypedDataHelper::GetTypeHash(
    const std::string_view primary_type_name) const {
  return KeccakHash(base::as_byte_span(EncodeTypes(primary_type_name)));
}

std::optional<
    std::pair<EthSignTypedDataHelper::Eip712HashArray, base::Value::Dict>>
EthSignTypedDataHelper::HashStruct(const std::string_view primary_type_name,
                                   const base::Value::Dict& data) const {
  auto encoded_data = EncodeData(primary_type_name, data);
  if (!encoded_data) {
    return std::nullopt;
  }
  return std::make_pair(KeccakHash(encoded_data->first),
                        std::move(encoded_data->second));
}

// Encode the json data by the its type defined in json custom types starting
// from primary type. See unittests for some examples.
std::optional<std::pair<std::vector<uint8_t>, base::Value::Dict>>
EthSignTypedDataHelper::EncodeData(const std::string_view primary_type_name,
                                   const base::Value::Dict& data) const {
  const auto* primary_type = types_.FindList(primary_type_name);
  if (!primary_type) {
    return std::nullopt;
  }
  std::vector<uint8_t> result;
  // 32 bytes for type hash and for each item in schema.
  result.reserve(Eip712HashArray().size() * (1 + primary_type->size()));

  base::Extend(result, GetTypeHash(primary_type_name));

  base::Value::Dict sanitized_data;

  for (const auto& item : *primary_type) {
    const auto& field = item.GetDict();
    const std::string* type_str = field.FindString("type");
    const std::string* name_str = field.FindString("name");
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
      VLOG(0) << "version has to be v4 to support array";
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
    base::ranges::reverse(result);

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
    base::ranges::reverse(result);

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
    std::pair<EthSignTypedDataHelper::Eip712HashArray, base::Value::Dict>>
EthSignTypedDataHelper::GetTypedDataDomainHash(
    const base::Value::Dict& domain) const {
  return HashStruct("EIP712Domain", domain);
}

std::optional<
    std::pair<EthSignTypedDataHelper::Eip712HashArray, base::Value::Dict>>
EthSignTypedDataHelper::GetTypedDataPrimaryHash(
    const std::string& primary_type_name,
    const base::Value::Dict& message) const {
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
