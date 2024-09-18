/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_tx_meta.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/json/json_reader.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/solana_instruction_data_decoder.h"
#include "brave/components/brave_wallet/browser/solana_test_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_constants.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/origin.h"

namespace brave_wallet {

TEST(SolanaTxMetaUnitTest, IsRetriable) {
  auto tx = std::make_unique<SolanaTransaction>(GetTestLegacyMessage());
  auto sol_account =
      MakeAccountId(mojom::CoinType::SOL, mojom::KeyringId::kSolana,
                    mojom::AccountKind::kDerived, kFromAccount);
  SolanaTxMeta meta(sol_account, std::move(tx));

  meta.set_status(mojom::TransactionStatus::Error);
  meta.tx()->set_tx_type(mojom::TransactionType::SolanaSystemTransfer);
  auto param = mojom::SolanaSignTransactionParam::New(
      "test", std::vector<mojom::SignaturePubkeyPairPtr>());
  meta.tx()->set_sign_tx_param(param.Clone());

  EXPECT_TRUE(meta.IsRetriable());

  // Test non-retriable status.
  meta.set_status(mojom::TransactionStatus::Confirmed);
  EXPECT_FALSE(meta.IsRetriable());

  // Reset transaction status.
  meta.set_status(mojom::TransactionStatus::Error);
  EXPECT_TRUE(meta.IsRetriable());

  // Test non-retriable transaction type.
  meta.tx()->set_tx_type(mojom::TransactionType::SolanaSwap);
  EXPECT_FALSE(meta.IsRetriable());

  // Reset transaction type.
  meta.tx()->set_tx_type(mojom::TransactionType::SolanaSystemTransfer);
  EXPECT_TRUE(meta.IsRetriable());

  // Test partial signed transaction with normal blockhash.
  auto partial_signed_param = param.Clone();
  partial_signed_param->signatures.emplace_back(mojom::SignaturePubkeyPair::New(
      mojom::SolanaSignature::New(
          std::vector<uint8_t>(kSolanaSignatureSize, 1)),
      kFromAccount));
  meta.tx()->set_sign_tx_param(partial_signed_param.Clone());
  EXPECT_FALSE(meta.IsRetriable());

  // Test partial signed transaction with durable nonce.
  SolanaInstruction instruction = GetAdvanceNonceAccountInstruction();
  std::vector<SolanaInstruction> vec;
  vec.emplace_back(instruction);
  vec.emplace_back(meta.tx()->message()->instructions()[0]);
  meta.tx()->message()->SetInstructionsForTesting(vec);
  EXPECT_TRUE(meta.IsRetriable());
}

TEST(SolanaTxMetaUnitTest, ToTransactionInfo) {
  std::string from_account = "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8";
  auto sol_account =
      MakeAccountId(mojom::CoinType::SOL, mojom::KeyringId::kSolana,
                    mojom::AccountKind::kDerived, from_account);

  std::string to_account = "JDqrvDz8d8tFCADashbUKQDKfJZFobNy13ugN65t1wvV";
  std::string recent_blockhash = "9sHcv6xwn9YkB8nxTUGKDwPwNnmqVp5oAXxU8Fdkm4J6";
  uint64_t last_valid_block_height = 3090;
  const std::vector<uint8_t> data = {2, 0, 0, 0, 128, 150, 152, 0, 0, 0, 0, 0};

  SolanaInstruction instruction(
      // Program ID
      mojom::kSolanaSystemProgramId,
      // Accounts
      {SolanaAccountMeta(from_account, std::nullopt, true, true),
       SolanaAccountMeta(to_account, std::nullopt, false, true)},
      data);

  auto msg = SolanaMessage::CreateLegacyMessage(
      recent_blockhash, last_valid_block_height, from_account,
      std::vector<SolanaInstruction>({instruction}));
  ASSERT_TRUE(msg);
  auto tx = std::make_unique<SolanaTransaction>(std::move(*msg));
  tx->set_to_wallet_address(to_account);
  tx->set_lamports(10000000u);
  tx->set_tx_type(mojom::TransactionType::SolanaSystemTransfer);

  SolanaTxMeta meta(sol_account, std::move(tx));
  SolanaSignatureStatus status(82, 10, "", "confirmed");
  meta.set_signature_status(status);

  meta.set_id("meta_id");
  meta.set_status(mojom::TransactionStatus::Confirmed);
  base::Time::Exploded x{1981, 3, 0, 1, 2};
  base::Time confirmed_time = meta.confirmed_time();
  EXPECT_TRUE(base::Time::FromUTCExploded(x, &confirmed_time));
  meta.set_confirmed_time(confirmed_time);
  meta.set_submitted_time(confirmed_time - base::Seconds(3));
  meta.set_created_time(confirmed_time - base::Minutes(1));
  meta.set_tx_hash(
      "5VERv8NMvzbJMEkV8xnrLkEaWRtSz9CosKDYjCJjBRnbJLgp8uirBgmQpjKhoR4tjF3ZpRzr"
      "FmBV6UjKdiSZkQUW");
  meta.set_origin(url::Origin::Create(GURL("https://test.brave.com/")));

  mojom::TransactionInfoPtr ti = meta.ToTransactionInfo();
  EXPECT_EQ(ti->id, meta.id());
  EXPECT_EQ(ti->tx_status, meta.status());
  EXPECT_EQ(ti->from_address, from_account);
  EXPECT_EQ(ti->from_account_id, meta.from());
  EXPECT_EQ(ti->tx_hash, meta.tx_hash());
  EXPECT_EQ(
      ti->origin_info,
      MakeOriginInfo(url::Origin::Create(GURL("https://test.brave.com/"))));

  EXPECT_EQ(meta.created_time().InMillisecondsSinceUnixEpoch(),
            ti->created_time.InMilliseconds());
  EXPECT_EQ(meta.submitted_time().InMillisecondsSinceUnixEpoch(),
            ti->submitted_time.InMilliseconds());
  EXPECT_EQ(meta.confirmed_time().InMillisecondsSinceUnixEpoch(),
            ti->confirmed_time.InMilliseconds());

  EXPECT_EQ(meta.tx()->tx_type(), ti->tx_type);
  EXPECT_TRUE(ti->tx_params.empty());
  EXPECT_TRUE(ti->tx_args.empty());

  auto solana_account_meta1 =
      mojom::SolanaAccountMeta::New(from_account, nullptr, true, true);
  auto solana_account_meta2 =
      mojom::SolanaAccountMeta::New(to_account, nullptr, false, true);
  std::vector<mojom::SolanaAccountMetaPtr> account_metas;
  account_metas.push_back(std::move(solana_account_meta1));
  account_metas.push_back(std::move(solana_account_meta2));
  auto mojom_param = mojom::SolanaInstructionParam::New(
      "lamports", "Lamports", "10000000",
      mojom::SolanaInstructionParamType::kUint64);
  std::vector<mojom::SolanaInstructionParamPtr> mojom_params;
  mojom_params.emplace_back(std::move(mojom_param));
  auto mojom_decoded_data = mojom::DecodedSolanaInstructionData::New(
      static_cast<uint32_t>(mojom::SolanaSystemInstruction::kTransfer),
      solana_ins_data_decoder::GetMojomAccountParamsForTesting(
          mojom::SolanaSystemInstruction::kTransfer, std::nullopt),
      std::move(mojom_params));
  auto mojom_instruction = mojom::SolanaInstruction::New(
      mojom::kSolanaSystemProgramId, std::move(account_metas), data,
      std::move(mojom_decoded_data));

  std::vector<mojom::SolanaInstructionPtr> instructions;
  instructions.push_back(std::move(mojom_instruction));

  ASSERT_TRUE(ti->tx_data_union->is_solana_tx_data());
  EXPECT_EQ(
      ti->tx_data_union->get_solana_tx_data(),
      mojom::SolanaTxData::New(
          recent_blockhash, last_valid_block_height, from_account, to_account,
          "", 10000000, 0, mojom::TransactionType::SolanaSystemTransfer,
          std::move(instructions), mojom::SolanaMessageVersion::kLegacy,
          mojom::SolanaMessageHeader::New(1, 0, 1),
          std::vector<std::string>(
              {from_account, to_account, mojom::kSolanaSystemProgramId}),
          std::vector<mojom::SolanaMessageAddressTableLookupPtr>(), nullptr,
          nullptr, nullptr));
}

TEST(SolanaTxMetaUnitTest, ToValue) {
  std::string from_account = "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8";
  auto sol_account =
      MakeAccountId(mojom::CoinType::SOL, mojom::KeyringId::kSolana,
                    mojom::AccountKind::kDerived, from_account);

  std::string to_account = "JDqrvDz8d8tFCADashbUKQDKfJZFobNy13ugN65t1wvV";
  std::string recent_blockhash = "9sHcv6xwn9YkB8nxTUGKDwPwNnmqVp5oAXxU8Fdkm4J6";
  const std::vector<uint8_t> data = {2, 0, 0, 0, 128, 150, 152, 0, 0, 0, 0, 0};
  uint64_t last_valid_block_height = 3090;

  SolanaInstruction instruction(
      // Program ID
      mojom::kSolanaSystemProgramId,
      // Accounts
      {SolanaAccountMeta(from_account, std::nullopt, true, true),
       SolanaAccountMeta(to_account, std::nullopt, false, true)},
      data);
  auto msg = SolanaMessage::CreateLegacyMessage(
      recent_blockhash, last_valid_block_height, from_account,
      std::vector<SolanaInstruction>({instruction}));
  ASSERT_TRUE(msg);
  auto tx = std::make_unique<SolanaTransaction>(std::move(*msg));
  tx->set_to_wallet_address(to_account);
  tx->set_lamports(10000000u);
  tx->set_tx_type(mojom::TransactionType::SolanaSystemTransfer);

  SolanaTxMeta meta(sol_account, std::move(tx));
  SolanaSignatureStatus status(82, 10, "", "confirmed");
  meta.set_signature_status(status);

  meta.set_id("meta_id");
  meta.set_status(mojom::TransactionStatus::Confirmed);
  base::Time::Exploded x{1981, 3, 0, 1, 2};
  base::Time confirmed_time = meta.confirmed_time();
  EXPECT_TRUE(base::Time::FromUTCExploded(x, &confirmed_time));
  meta.set_confirmed_time(confirmed_time);
  meta.set_submitted_time(confirmed_time - base::Seconds(3));
  meta.set_created_time(confirmed_time - base::Minutes(1));
  meta.set_tx_hash(
      "5VERv8NMvzbJMEkV8xnrLkEaWRtSz9CosKDYjCJjBRnbJLgp8uirBgmQpjKhoR4tjF3ZpRzr"
      "FmBV6UjKdiSZkQUW");
  meta.set_origin(url::Origin::Create(GURL("https://test.brave.com/")));
  meta.set_chain_id("0x66");

  base::Value::Dict value = meta.ToValue();
  auto expect_value = base::JSONReader::Read(R"(
    {
      "coin": 501,
      "chain_id" : "0x66",
      "id": "meta_id",
      "status": 4,
      "from_account_id": "501_1_0_BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8",
      "tx_hash": "5VERv8NMvzbJMEkV8xnrLkEaWRtSz9CosKDYjCJjBRnbJLgp8uirBgmQpjKhoR4tjF3ZpRzrFmBV6UjKdiSZkQUW",
      "origin": "https://test.brave.com/",
      "confirmed_time": "11996733600000000",
      "created_time": "11996733540000000",
      "submitted_time": "11996733597000000",
      "tx": {
        "message": {
          "version": 0,
          "recent_blockhash": "9sHcv6xwn9YkB8nxTUGKDwPwNnmqVp5oAXxU8Fdkm4J6",
          "last_valid_block_height": "3090",
          "fee_payer": "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8",
          "message_header": {
            "num_required_signatures": "1",
            "num_readonly_signed_accounts": "0",
            "num_readonly_unsigned_accounts": "1"
          },
          "static_account_keys": [
            "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8",
            "JDqrvDz8d8tFCADashbUKQDKfJZFobNy13ugN65t1wvV",
            "11111111111111111111111111111111"
          ],
          "instructions": [
            {
              "program_id": "11111111111111111111111111111111",
              "accounts": [
                {
                  "pubkey": "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8",
                  "is_signer": true,
                  "is_writable": true
                },
                {
                  "pubkey": "JDqrvDz8d8tFCADashbUKQDKfJZFobNy13ugN65t1wvV",
                  "is_signer": false,
                  "is_writable": true
                }
              ],
              "data": "AgAAAICWmAAAAAAA",
              "decoded_data": {
                "account_params": [
                  {
                    "name": "from_account",
                    "localized_name": "From Account"
                  },
                  {
                    "name": "to_account",
                    "localized_name": "To Account"
                  }
                ],
                "params": [
                  {
                    "name": "lamports",
                    "localized_name": "Lamports",
                    "value": "10000000",
                    "type": 2
                  }
                ],
                "sys_ins_type": "2"
              }
            }
          ],
          "address_table_lookups": []
        },
        "to_wallet_address": "JDqrvDz8d8tFCADashbUKQDKfJZFobNy13ugN65t1wvV",
        "spl_token_mint_address": "",
        "lamports": "10000000",
        "amount": "0",
        "wired_tx": "",
        "tx_type": 6
      },
      "signature_status": {
        "slot": "82",
        "confirmations": "10",
        "err": "",
        "confirmation_status": "confirmed"
      }
    }
  )");
  ASSERT_TRUE(expect_value);
  EXPECT_EQ(*expect_value, value);
}

}  // namespace brave_wallet
