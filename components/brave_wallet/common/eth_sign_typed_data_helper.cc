/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/eth_sign_typed_data_helper.h"

#include <limits>

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
    const base::Value& types,
    Version version) {
  if (!types.is_dict())
    return nullptr;
  return std::unique_ptr<EthSignTypedDataHelper>(
      new EthSignTypedDataHelper(types, version));
}

EthSignTypedDataHelper::EthSignTypedDataHelper(const base::Value& types,
                                               Version version)
    : types_(types.Clone()), version_(version) {
  CHECK(types_.is_dict());
}

EthSignTypedDataHelper::~EthSignTypedDataHelper() = default;

bool EthSignTypedDataHelper::SetTypes(const base::Value& types) {
  if (!types_.is_dict())
    return false;
  types_ = types.Clone();
  return true;
}

void EthSignTypedDataHelper::SetVersion(Version version) {
  version_ = version;
}

void EthSignTypedDataHelper::FindAllDependencyTypes(
    base::flat_map<std::string, base::Value>* known_types,
    const std::string& anchor_type_name) const {
  DCHECK(!anchor_type_name.empty());
  DCHECK(known_types);

  const base::Value* anchor_type =
      types_.FindKeyOfType(anchor_type_name, base::Value::Type::LIST);
  if (!anchor_type)
    return;
  known_types->emplace(anchor_type_name, anchor_type->Clone());

  for (const auto& field : anchor_type->GetListDeprecated()) {
    const std::string* type = field.FindStringKey("type");
    if (type && !known_types->contains(*type)) {
      FindAllDependencyTypes(known_types, *type);
    }
  }
}

std::string EthSignTypedDataHelper::EncodeType(
    const base::Value& type,
    const std::string& type_name) const {
  if (!type.is_list())
    return std::string();
  std::string result = base::StrCat({type_name, "("});

  for (size_t i = 0; i < type.GetListDeprecated().size(); ++i) {
    const std::string* type_str =
        type.GetListDeprecated()[i].FindStringKey("type");
    const std::string* name_str =
        type.GetListDeprecated()[i].FindStringKey("name");
    DCHECK(type_str && name_str);
    base::StrAppend(&result, {*type_str, " ", *name_str});
    if (i != type.GetListDeprecated().size() - 1)
      base::StrAppend(&result, {","});
  }
  base::StrAppend(&result, {")"});
  return result;
}

std::string EthSignTypedDataHelper::EncodeTypes(
    const std::string& primary_type_name) const {
  std::string result;

  base::flat_map<std::string, base::Value> types_map;
  FindAllDependencyTypes(&types_map, primary_type_name);

  auto it = types_map.find(primary_type_name);
  if (it != types_map.end()) {
    base::StrAppend(&result, {EncodeType(it->second, primary_type_name)});
  }
  for (const auto& type : types_map) {
    if (type.first == primary_type_name)
      continue;
    base::StrAppend(&result, {EncodeType(type.second, type.first)});
  }
  return result;
}

std::vector<uint8_t> EthSignTypedDataHelper::GetTypeHash(
    const std::string primary_type_name) const {
  const std::string type_hash =
      KeccakHash(EncodeTypes(primary_type_name), false);
  return std::vector<uint8_t>(type_hash.begin(), type_hash.end());
}

absl::optional<std::vector<uint8_t>> EthSignTypedDataHelper::HashStruct(
    const std::string primary_type_name,
    const base::Value& data) const {
  auto encoded_data = EncodeData(primary_type_name, data);
  if (!encoded_data)
    return absl::nullopt;
  return KeccakHash(*encoded_data);
}

// Encode the json data by the its type defined in json custom types starting
// from primary type. See unittests for some examples.
absl::optional<std::vector<uint8_t>> EthSignTypedDataHelper::EncodeData(
    const std::string& primary_type_name,
    const base::Value& data) const {
  DCHECK(data.is_dict());
  const base::Value* primary_type = types_.FindKey(primary_type_name);
  DCHECK(primary_type);
  DCHECK(primary_type->is_list());
  std::vector<uint8_t> result;

  const std::vector<uint8_t> type_hash = GetTypeHash(primary_type_name);
  result.insert(result.end(), type_hash.begin(), type_hash.end());

  for (const auto& field : primary_type->GetListDeprecated()) {
    const std::string* type_str = field.FindStringKey("type");
    const std::string* name_str = field.FindStringKey("name");
    DCHECK(type_str && name_str);
    const base::Value* value = data.FindKey(*name_str);
    if (value) {
      auto encoded_field = EncodeField(*type_str, *value);
      if (!encoded_field)
        return absl::nullopt;
      result.insert(result.end(), encoded_field->begin(), encoded_field->end());
    } else {
      if (version_ == Version::kV4) {
        for (size_t i = 0; i < 32; ++i)
          result.push_back(0);
      }
    }
  }
  return result;
}

// Encode each field of a custom type, if a field is also a custom type it
// will call EncodeData recursively until it reaches an atomic type
absl::optional<std::vector<uint8_t>> EthSignTypedDataHelper::EncodeField(
    const std::string& type,
    const base::Value& value) const {
  // ES6 section 20.1.2.6 Number.MAX_SAFE_INTEGER
  constexpr double kMaxSafeInteger = static_cast<double>(kMaxSafeIntegerUint64);
  std::vector<uint8_t> result;

  if (base::EndsWith(type, "]")) {
    if (version_ != Version::kV4) {
      VLOG(0) << "version has to be v4 to support array";
      return absl::nullopt;
    }
    if (!value.is_list())
      return absl::nullopt;
    auto type_split = base::SplitString(type, "[", base::KEEP_WHITESPACE,
                                        base::SPLIT_WANT_ALL);
    if (type_split.size() != 2)
      return absl::nullopt;
    const std::string array_type = type_split[0];
    std::vector<uint8_t> array_result;
    for (const auto& item : value.GetListDeprecated()) {
      auto encoded_item = EncodeField(array_type, item);
      if (!encoded_item)
        return absl::nullopt;
      array_result.insert(array_result.end(), encoded_item->begin(),
                          encoded_item->end());
    }
    auto array_hash = KeccakHash(array_result);
    result.insert(result.end(), array_hash.begin(), array_hash.end());
  } else if (type == "string") {
    const std::string* value_str = value.GetIfString();
    if (!value_str)
      return absl::nullopt;
    const std::string encoded_value = KeccakHash(*value_str, false);
    const std::vector<uint8_t> encoded_value_bytes(encoded_value.begin(),
                                                   encoded_value.end());
    result.insert(result.end(), encoded_value_bytes.begin(),
                  encoded_value_bytes.end());
  } else if (type == "bytes") {
    const std::string* value_str = value.GetIfString();
    if (!value_str || (!value_str->empty() && !IsValidHexString(*value_str)))
      return absl::nullopt;
    std::vector<uint8_t> bytes;
    if (!value_str->empty())
      CHECK(PrefixedHexStringToBytes(*value_str, &bytes));
    const std::vector<uint8_t> encoded_value = KeccakHash(bytes);
    result.insert(result.end(), encoded_value.begin(), encoded_value.end());
  } else if (type == "bool") {
    absl::optional<bool> value_bool = value.GetIfBool();
    if (!value_bool)
      return absl::nullopt;
    uint256_t encoded_value = (uint256_t)*value_bool;
    // Append the encoded value to byte array result in big endian order
    for (int i = 256 - 8; i >= 0; i -= 8) {
      result.push_back(static_cast<uint8_t>((encoded_value >> i) & 0xFF));
    }
  } else if (type == "address") {
    const std::string* value_str = value.GetIfString();
    if (!value_str || !IsValidHexString(*value_str))
      return absl::nullopt;
    std::vector<uint8_t> address;
    CHECK(PrefixedHexStringToBytes(*value_str, &address));
    DCHECK_EQ(address.size(), 20u);
    for (size_t i = 0; i < 256 - 160; i += 8)
      result.push_back(0);
    result.insert(result.end(), address.begin(), address.end());
  } else if (base::StartsWith(type, "bytes", base::CompareCase::SENSITIVE)) {
    unsigned type_check;
    if (!base::StringToUint(type.data() + 5, &type_check) || type_check > 32)
      return absl::nullopt;
    const std::string* value_str = value.GetIfString();
    if (!value_str || !IsValidHexString(*value_str))
      return absl::nullopt;
    std::vector<uint8_t> bytes;
    CHECK(PrefixedHexStringToBytes(*value_str, &bytes));
    if (bytes.size() > 32)
      return absl::nullopt;
    result.insert(result.end(), bytes.begin(), bytes.end());
    for (size_t i = 0; i < 32u - bytes.size(); ++i) {
      result.push_back(0);
    }
  } else if (base::StartsWith(type, "uint", base::CompareCase::SENSITIVE)) {
    // uint8 to uint256 in steps of 8
    unsigned type_check;
    if (!base::StringToUint(type.data() + 4, &type_check) || type_check % 8 ||
        type_check > 256)
      return absl::nullopt;

    absl::optional<double> value_double = value.GetIfDouble();
    const std::string* value_str = value.GetIfString();
    uint256_t encoded_value = 0;
    if (value_double) {
      encoded_value = (uint256_t)(uint64_t)*value_double;
      if (encoded_value > (uint256_t)kMaxSafeInteger)
        return absl::nullopt;
    } else if (value_str) {
      if (!value_str->empty() &&
          !HexValueToUint256(*value_str, &encoded_value) &&
          !Base10ValueToUint256(*value_str, &encoded_value))
        return absl::nullopt;
    } else {
      return absl::nullopt;
    }

    // check if value excceeds type bound
    switch (type_check) {
      case 8:
        if (encoded_value > std::numeric_limits<uint8_t>::max())
          return absl::nullopt;
        break;
      case 16:
        if (encoded_value > std::numeric_limits<uint16_t>::max())
          return absl::nullopt;
        break;
      case 32:
        if (encoded_value > std::numeric_limits<uint32_t>::max())
          return absl::nullopt;
        break;
      case 64:
        if (encoded_value > std::numeric_limits<uint64_t>::max())
          return absl::nullopt;
        break;
      case 128:
        if (encoded_value > std::numeric_limits<uint128_t>::max())
          return absl::nullopt;
        break;
      case 256:
        if (encoded_value > std::numeric_limits<uint256_t>::max())
          return absl::nullopt;
        break;
      default:
        return absl::nullopt;
    }
    // Append the encoded value to byte array result in big endian order
    for (int i = 256 - 8; i >= 0; i -= 8) {
      result.push_back(static_cast<uint8_t>((encoded_value >> i) & 0xFF));
    }
  } else if (base::StartsWith(type, "int", base::CompareCase::SENSITIVE)) {
    // int8 to int256 in steps of 8
    unsigned type_check;
    if (!base::StringToUint(type.data() + 3, &type_check) || type_check % 8 ||
        type_check > 256)
      return absl::nullopt;
    absl::optional<double> value_double = value.GetIfDouble();
    const std::string* value_str = value.GetIfString();
    int256_t encoded_value = 0;
    if (value_double) {
      encoded_value = (int256_t)(int64_t)*value_double;
      if (encoded_value > (int256_t)kMaxSafeInteger)
        return absl::nullopt;
    } else if (value_str) {
      if (!value_str->empty() &&
          !HexValueToInt256(*value_str, &encoded_value) &&
          !Base10ValueToInt256(*value_str, &encoded_value))
        return absl::nullopt;
    } else {
      return absl::nullopt;
    }

    // check if value excceeds type bound
    switch (type_check) {
      case 8:
        if (encoded_value > std::numeric_limits<int8_t>::max() ||
            encoded_value < std::numeric_limits<int8_t>::min())
          return absl::nullopt;
        break;
      case 16:
        if (encoded_value > std::numeric_limits<int16_t>::max() ||
            encoded_value < std::numeric_limits<int16_t>::min())
          return absl::nullopt;
        break;
      case 32:
        if (encoded_value > std::numeric_limits<int32_t>::max() ||
            encoded_value < std::numeric_limits<int32_t>::min())
          return absl::nullopt;
        break;
      case 64:
        if (encoded_value > std::numeric_limits<int64_t>::max() ||
            encoded_value < std::numeric_limits<int64_t>::min())
          return absl::nullopt;
        break;
      case 128:
        if (encoded_value > kMax128BitInt || encoded_value < kMin128BitInt)
          return absl::nullopt;
        break;
      case 256:
        if (encoded_value > kMax256BitInt || encoded_value < kMin256BitInt)
          return absl::nullopt;
        break;
      default:
        return absl::nullopt;
    }
    // Append the encoded value to byte array result in big endian order
    for (int i = 256 - 8; i >= 0; i -= 8) {
      result.push_back(static_cast<uint8_t>((encoded_value >> i) & 0xFF));
    }
  } else {
    auto encoded_data = EncodeData(type, value);
    if (!encoded_data)
      return absl::nullopt;
    std::vector<uint8_t> encoded_value = KeccakHash(*encoded_data);

    result.insert(result.end(), encoded_value.begin(), encoded_value.end());
  }
  return result;
}

absl::optional<std::vector<uint8_t>>
EthSignTypedDataHelper::GetTypedDataDomainHash(
    const base::Value& domain_separator) const {
  return HashStruct("EIP712Domain", domain_separator);
}

absl::optional<std::vector<uint8_t>>
EthSignTypedDataHelper::GetTypedDataPrimaryHash(
    const std::string& primary_type_name,
    const base::Value& message) const {
  return HashStruct(primary_type_name, message);
}

// static
absl::optional<std::vector<uint8_t>>
EthSignTypedDataHelper::GetTypedDataMessageToSign(
    const std::vector<uint8_t>& domain_hash,
    const std::vector<uint8_t>& primary_hash) {
  if (domain_hash.empty() || primary_hash.empty())
    return absl::nullopt;
  std::vector<uint8_t> encoded_data({0x19, 0x01});
  encoded_data.insert(encoded_data.end(), domain_hash.begin(),
                      domain_hash.end());
  encoded_data.insert(encoded_data.end(), primary_hash.begin(),
                      primary_hash.end());
  return KeccakHash(encoded_data);
}

}  // namespace brave_wallet
