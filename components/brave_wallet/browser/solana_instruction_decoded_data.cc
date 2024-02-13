/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_instruction_decoded_data.h"

#include <optional>

namespace brave_wallet {

SolanaInstructionDecodedData::SolanaInstructionDecodedData() = default;
SolanaInstructionDecodedData::SolanaInstructionDecodedData(
    const SolanaInstructionDecodedData&) = default;
SolanaInstructionDecodedData& SolanaInstructionDecodedData::operator=(
    const SolanaInstructionDecodedData&) = default;
SolanaInstructionDecodedData::SolanaInstructionDecodedData(
    SolanaInstructionDecodedData&&) = default;
SolanaInstructionDecodedData& SolanaInstructionDecodedData::operator=(
    SolanaInstructionDecodedData&&) = default;
SolanaInstructionDecodedData::~SolanaInstructionDecodedData() = default;

bool SolanaInstructionDecodedData::operator==(
    const SolanaInstructionDecodedData& other) const {
  return sys_ins_type == other.sys_ins_type &&
         token_ins_type == other.token_ins_type && params == other.params &&
         account_params == other.account_params;
}

// static
std::optional<SolanaInstructionDecodedData>
SolanaInstructionDecodedData::FromMojom(
    const std::string& program_id,
    const mojom::DecodedSolanaInstructionDataPtr& mojom_decoded_data) {
  SolanaInstructionDecodedData decoded_data;
  if (!mojom_decoded_data) {
    return std::nullopt;
  }

  if (program_id != mojom::kSolanaSystemProgramId &&
      program_id != mojom::kSolanaTokenProgramId) {
    return std::nullopt;
  }

  if (program_id == mojom::kSolanaSystemProgramId) {
    if (mojom_decoded_data->instruction_type >
        static_cast<uint32_t>(mojom::SolanaSystemInstruction::kMaxValue)) {
      return std::nullopt;
    }
    decoded_data.sys_ins_type = static_cast<mojom::SolanaSystemInstruction>(
        mojom_decoded_data->instruction_type);
  } else {  // token program
    if (mojom_decoded_data->instruction_type >
        static_cast<uint32_t>(mojom::SolanaTokenInstruction::kMaxValue)) {
      return std::nullopt;
    }
    decoded_data.token_ins_type = static_cast<mojom::SolanaTokenInstruction>(
        mojom_decoded_data->instruction_type);
  }

  for (const auto& mojom_param : mojom_decoded_data->params) {
    decoded_data.params.emplace_back(
        std::make_tuple(mojom_param->name, mojom_param->localized_name,
                        mojom_param->value, mojom_param->type));
  }

  for (const auto& mojom_account_param : mojom_decoded_data->account_params) {
    decoded_data.account_params.emplace_back(std::make_pair(
        mojom_account_param->name, mojom_account_param->localized_name));
  }

  return decoded_data;
}

mojom::DecodedSolanaInstructionDataPtr SolanaInstructionDecodedData::ToMojom()
    const {
  if (!IsValid()) {
    return nullptr;
  }

  std::vector<mojom::SolanaInstructionParamPtr> mojom_params;
  uint32_t ins_type;
  ins_type = sys_ins_type ? static_cast<uint32_t>(*sys_ins_type)
                          : static_cast<uint32_t>(*token_ins_type);
  for (const auto& param : params) {
    mojom_params.emplace_back(mojom::SolanaInstructionParam::New(
        std::get<0>(param), std::get<1>(param), std::get<2>(param),
        std::get<3>(param)));
  }

  std::vector<mojom::SolanaInstructionAccountParamPtr> mojom_account_params;
  for (const auto& param : account_params) {
    mojom_account_params.emplace_back(
        mojom::SolanaInstructionAccountParam::New(param.first, param.second));
  }

  return mojom::DecodedSolanaInstructionData::New(
      ins_type, std::move(mojom_account_params), std::move(mojom_params));
}

// static
std::optional<SolanaInstructionDecodedData>
SolanaInstructionDecodedData::FromValue(const base::Value::Dict& value) {
  const std::string* sys_ins_type_str = value.FindString("sys_ins_type");
  const std::string* token_ins_type_str = value.FindString("token_ins_type");
  if (!sys_ins_type_str == !token_ins_type_str) {  // Both true or both false.
    return std::nullopt;
  }

  SolanaInstructionDecodedData decoded_data;
  if (sys_ins_type_str) {
    uint32_t sys_ins_type;
    if (!base::StringToUint(*sys_ins_type_str, &sys_ins_type) ||
        sys_ins_type >
            static_cast<uint32_t>(mojom::SolanaSystemInstruction::kMaxValue)) {
      return std::nullopt;
    }
    decoded_data.sys_ins_type =
        static_cast<mojom::SolanaSystemInstruction>(sys_ins_type);
  } else {
    uint32_t token_ins_type;
    if (!base::StringToUint(*token_ins_type_str, &token_ins_type) ||
        token_ins_type >
            static_cast<uint32_t>(mojom::SolanaTokenInstruction::kMaxValue)) {
      return std::nullopt;
    }
    decoded_data.token_ins_type =
        static_cast<mojom::SolanaTokenInstruction>(token_ins_type);
  }

  const base::Value::List* param_list = value.FindList("params");
  if (!param_list) {
    return std::nullopt;
  }
  for (const auto& param_value : *param_list) {
    if (!param_value.is_dict()) {
      return std::nullopt;
    }
    const auto* name = param_value.GetDict().FindString("name");
    if (!name) {
      return std::nullopt;
    }
    const auto* localized_name =
        param_value.GetDict().FindString("localized_name");
    if (!localized_name) {
      return std::nullopt;
    }
    const auto* value_local = param_value.GetDict().FindString("value");
    if (!value_local) {
      return std::nullopt;
    }

    mojom::SolanaInstructionParamType type =
        mojom::SolanaInstructionParamType::kUnknown;
    auto type_int = param_value.GetDict().FindInt("type");
    if (type_int &&
        mojom::IsKnownEnumValue(
            static_cast<mojom::SolanaInstructionParamType>(*type_int))) {
      type = static_cast<mojom::SolanaInstructionParamType>(*type_int);
    }

    decoded_data.params.emplace_back(
        std::make_tuple(*name, *localized_name, *value_local, type));
  }

  const base::Value::List* account_param_list =
      value.FindList("account_params");
  for (const auto& param_value : *account_param_list) {
    if (!param_value.is_dict()) {
      return std::nullopt;
    }
    const auto* name = param_value.GetDict().FindString("name");
    if (!name) {
      return std::nullopt;
    }
    const auto* localized_name =
        param_value.GetDict().FindString("localized_name");
    if (!localized_name) {
      return std::nullopt;
    }
    decoded_data.account_params.emplace_back(
        std::make_pair(*name, *localized_name));
  }

  return decoded_data;
}

std::optional<base::Value::Dict> SolanaInstructionDecodedData::ToValue() const {
  if (!IsValid()) {
    return std::nullopt;
  }

  base::Value::Dict dict;
  if (sys_ins_type) {
    dict.Set("sys_ins_type",
             base::NumberToString(static_cast<uint32_t>(*sys_ins_type)));
  }

  if (token_ins_type) {
    dict.Set("token_ins_type",
             base::NumberToString(static_cast<uint32_t>(*token_ins_type)));
  }

  base::Value::List param_list;
  for (const auto& param : params) {
    base::Value::Dict param_dict;
    param_dict.Set("name", std::get<0>(param));
    param_dict.Set("localized_name", std::get<1>(param));
    param_dict.Set("value", std::get<2>(param));
    param_dict.Set("type", static_cast<int>(std::get<3>(param)));
    param_list.Append(std::move(param_dict));
  }
  dict.Set("params", std::move(param_list));

  base::Value::List account_param_list;
  for (const auto& param : account_params) {
    base::Value::Dict param_dict;
    param_dict.Set("name", std::get<0>(param));
    param_dict.Set("localized_name", std::get<1>(param));
    account_param_list.Append(std::move(param_dict));
  }
  dict.Set("account_params", std::move(account_param_list));

  return dict;
}

}  // namespace brave_wallet
