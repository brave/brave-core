/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_instruction_decoded_data.h"

#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "base/test/gtest_util.h"
#include "base/test/values_test_util.h"
#include "brave/components/brave_wallet/browser/solana_instruction_data_decoder.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(SolanaInstructionDecodedDataUnitTest, FromToMojom) {
  std::vector<mojom::SolanaInstructionParamPtr> mojom_params;
  mojom_params.emplace_back(mojom::SolanaInstructionParam::New(
      "test1", "Test1", "TEST1", mojom::SolanaInstructionParamType::kUnknown));
  mojom_params.emplace_back(mojom::SolanaInstructionParam::New(
      "test2", "Test2", "TEST2",
      mojom::SolanaInstructionParamType::kPublicKey));
  auto mojom_decoded_data = mojom::DecodedSolanaInstructionData::New(
      static_cast<uint32_t>(mojom::SolanaSystemInstruction::kTransfer),
      solana_ins_data_decoder::GetMojomAccountParamsForTesting(
          mojom::SolanaSystemInstruction::kTransfer, std::nullopt),
      std::move(mojom_params));

  auto decoded_data = SolanaInstructionDecodedData::FromMojom(
      mojom::kSolanaSystemProgramId, mojom_decoded_data.Clone());
  ASSERT_TRUE(decoded_data);
  EXPECT_EQ(decoded_data->sys_ins_type,
            mojom::SolanaSystemInstruction::kTransfer);
  EXPECT_FALSE(decoded_data->token_ins_type);
  std::vector<std::tuple<std::string, std::string, std::string,
                         mojom::SolanaInstructionParamType>>
      expect_params({{"test1", "Test1", "TEST1",
                      mojom::SolanaInstructionParamType::kUnknown},
                     {"test2", "Test2", "TEST2",
                      mojom::SolanaInstructionParamType::kPublicKey}});
  auto expect_account_params =
      solana_ins_data_decoder::GetAccountParamsForTesting(
          mojom::SolanaSystemInstruction::kTransfer, std::nullopt);
  EXPECT_EQ(decoded_data->params, expect_params);
  EXPECT_EQ(decoded_data->account_params, expect_account_params);
  EXPECT_EQ(decoded_data->ToMojom(), mojom_decoded_data);

  mojom_decoded_data->instruction_type =
      static_cast<uint32_t>(mojom::SolanaTokenInstruction::kApprove);
  mojom_decoded_data->account_params =
      solana_ins_data_decoder::GetMojomAccountParamsForTesting(
          std::nullopt, mojom::SolanaTokenInstruction::kApprove);
  expect_account_params = solana_ins_data_decoder::GetAccountParamsForTesting(
      std::nullopt, mojom::SolanaTokenInstruction::kApprove);
  decoded_data = SolanaInstructionDecodedData::FromMojom(
      mojom::kSolanaTokenProgramId, mojom_decoded_data.Clone());
  ASSERT_TRUE(decoded_data);
  EXPECT_FALSE(decoded_data->sys_ins_type);
  EXPECT_EQ(decoded_data->token_ins_type,
            mojom::SolanaTokenInstruction::kApprove);
  EXPECT_EQ(decoded_data->params, expect_params);
  EXPECT_EQ(decoded_data->account_params, expect_account_params);
  EXPECT_EQ(decoded_data->ToMojom(), mojom_decoded_data);

  // Test invalid cases for FromMojom.
  EXPECT_FALSE(SolanaInstructionDecodedData::FromMojom(
      mojom::kSolanaTokenProgramId, nullptr));
  EXPECT_FALSE(SolanaInstructionDecodedData::FromMojom(
      mojom::kSolanaAssociatedTokenProgramId, mojom_decoded_data.Clone()));

  mojom_decoded_data->instruction_type =
      static_cast<uint32_t>(mojom::SolanaSystemInstruction::kMaxValue) + 1;
  EXPECT_FALSE(SolanaInstructionDecodedData::FromMojom(
      mojom::kSolanaSystemProgramId, mojom_decoded_data.Clone()));

  mojom_decoded_data->instruction_type =
      static_cast<uint32_t>(mojom::SolanaTokenInstruction::kMaxValue) + 1;
  EXPECT_FALSE(SolanaInstructionDecodedData::FromMojom(
      mojom::kSolanaTokenProgramId, mojom_decoded_data.Clone()));

  // Test invalid cases for ToMojom.
  decoded_data = SolanaInstructionDecodedData();
  EXPECT_FALSE(decoded_data->ToMojom());
  decoded_data->sys_ins_type = mojom::SolanaSystemInstruction::kTransfer;
  decoded_data->token_ins_type = mojom::SolanaTokenInstruction::kApprove;
  EXPECT_FALSE(decoded_data->ToMojom());
}

TEST(SolanaInstructionDecodedDataUnitTest, FromToValue) {
  SolanaInstructionDecodedData decoded_data;
  decoded_data.sys_ins_type = mojom::SolanaSystemInstruction::kTransfer;
  decoded_data.params = {
      {"test1", "Test1", "TEST1",
       mojom::SolanaInstructionParamType::kPublicKey},
      {"test2", "Test2", "TEST2", mojom::SolanaInstructionParamType::kUint8}};
  decoded_data.account_params = {{"param1", "Param1"}, {"param2", "Param2"}};

  auto decoded_data_dict = decoded_data.ToValue();
  ASSERT_TRUE(decoded_data_dict);
  auto expect_decoded_data_value = base::test::ParseJson(R"(
    {
      "account_params": [
        {
          "name": "param1",
          "localized_name": "Param1"
        },
        {
          "name": "param2",
          "localized_name": "Param2"
        }
      ],
      "params": [
        {
          "name": "test1",
          "localized_name": "Test1",
          "value": "TEST1",
          "type": 3
        },
        {
          "name": "test2",
          "localized_name": "Test2",
          "value": "TEST2",
          "type": 0
        }
      ],
      "sys_ins_type": "2"
    }
  )");
  EXPECT_EQ(*decoded_data_dict, expect_decoded_data_value.GetDict());

  auto decoded_data_from_value =
      SolanaInstructionDecodedData::FromValue(*decoded_data_dict);
  EXPECT_EQ(decoded_data, decoded_data_from_value);

  // Test DecodedData objects converted from old values without type info
  // should have unknown type info.
  SolanaInstructionDecodedData expected_decoded_data;
  expected_decoded_data.sys_ins_type =
      mojom::SolanaSystemInstruction::kTransfer;
  expected_decoded_data.params = {
      {"test1", "Test1", "TEST1", mojom::SolanaInstructionParamType::kUnknown},
      {"test2", "Test2", "TEST2", mojom::SolanaInstructionParamType::kUnknown}};
  expected_decoded_data.account_params = {{"param1", "Param1"},
                                          {"param2", "Param2"}};

  auto decoded_data_without_type_info = base::test::ParseJson(R"(
    {
      "account_params": [
        {
          "name": "param1",
          "localized_name": "Param1"
        },
        {
          "name": "param2",
          "localized_name": "Param2"
        }
      ],
      "params": [
        {
          "name": "test1",
          "localized_name": "Test1",
          "value": "TEST1"
        },
        {
          "name": "test2",
          "localized_name": "Test2",
          "value": "TEST2"
        }
      ],
      "sys_ins_type": "2"
    }
  )");
  EXPECT_EQ(expected_decoded_data, SolanaInstructionDecodedData::FromValue(
                                       decoded_data_without_type_info.GetDict())
                                       .value());

  // Test invalid FromValue cases.
  std::vector<std::string> invalid_value_strings = {
      "{}",
      R"({"sys_ins_type": "2"})",
      R"({"params": []})",
      R"({"params": [], "sys_ins_type": "2", "token_ins_type": "3"})",
      R"({"params": {}, "sys_ins_type": "2"})",
      R"({"params": ["test"], "sys_ins_type": "2"})",
      R"({"params": [{"test": "test"}], "sys_ins_type": "2"})",
      R"({"params": [{"name": "test"}], "sys_ins_type": "2"})",
      R"({"params": [{"value": "test"}], "sys_ins_type": "2"})"};

  for (const auto& invalid_value_string : invalid_value_strings) {
    auto invalid_value = base::test::ParseJson(invalid_value_string);
    EXPECT_FALSE(
        SolanaInstructionDecodedData::FromValue(invalid_value.GetDict()))
        << ":" << invalid_value_string;
  }

  // Test invalid ToValue cases.
  EXPECT_FALSE(SolanaInstructionDecodedData().ToValue());

  decoded_data.token_ins_type = mojom::SolanaTokenInstruction::kApprove;
  EXPECT_FALSE(decoded_data.ToValue());
}

}  // namespace brave_wallet
