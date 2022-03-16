/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_tx_meta.h"

#include <memory>
#include <string>
#include <vector>

#include "base/json/json_reader.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(SolanaTxMetaUnitTest, ToTransactionInfo) {
  std::string from_account = "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8";
  std::string to_account = "JDqrvDz8d8tFCADashbUKQDKfJZFobNy13ugN65t1wvV";
  std::string recent_blockhash = "9sHcv6xwn9YkB8nxTUGKDwPwNnmqVp5oAXxU8Fdkm4J6";
  const std::vector<uint8_t> data = {2, 0, 0, 0, 128, 150, 152, 0, 0, 0, 0, 0};

  SolanaInstruction instruction(
      // Program ID
      kSolanaSystemProgramId,
      // Accounts
      {SolanaAccountMeta(from_account, true, true),
       SolanaAccountMeta(to_account, false, true)},
      data);
  auto tx = std::make_unique<SolanaTransaction>(
      recent_blockhash, from_account,
      std::vector<SolanaInstruction>({instruction}));

  SolanaTxMeta meta(std::move(tx));
  SolanaSignatureStatus status(82, 10, "", "confirmed");
  meta.set_signature_status(status);

  meta.set_id("meta_id");
  meta.set_status(mojom::TransactionStatus::Confirmed);
  meta.set_from("BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8");
  base::Time::Exploded x{1981, 3, 0, 1, 2};
  base::Time confirmed_time = meta.confirmed_time();
  EXPECT_TRUE(base::Time::FromUTCExploded(x, &confirmed_time));
  meta.set_confirmed_time(confirmed_time);
  meta.set_submitted_time(confirmed_time - base::Seconds(3));
  meta.set_created_time(confirmed_time - base::Minutes(1));
  meta.set_tx_hash(
      "5VERv8NMvzbJMEkV8xnrLkEaWRtSz9CosKDYjCJjBRnbJLgp8uirBgmQpjKhoR4tjF3ZpRzr"
      "FmBV6UjKdiSZkQUW");

  mojom::TransactionInfoPtr ti = meta.ToTransactionInfo();
  EXPECT_EQ(ti->id, meta.id());
  EXPECT_EQ(ti->tx_status, meta.status());
  EXPECT_EQ(ti->from_address, meta.from());
  EXPECT_EQ(ti->tx_hash, meta.tx_hash());

  EXPECT_EQ(meta.created_time().ToJavaTime(),
            ti->created_time.InMilliseconds());
  EXPECT_EQ(meta.submitted_time().ToJavaTime(),
            ti->submitted_time.InMilliseconds());
  EXPECT_EQ(meta.confirmed_time().ToJavaTime(),
            ti->confirmed_time.InMilliseconds());

  EXPECT_EQ(mojom::TransactionType::Other, ti->tx_type);
  EXPECT_TRUE(ti->tx_params.empty());
  EXPECT_TRUE(ti->tx_args.empty());

  auto solana_account_meta1 =
      mojom::SolanaAccountMeta::New(from_account, true, true);
  auto solana_account_meta2 =
      mojom::SolanaAccountMeta::New(to_account, false, true);
  std::vector<mojom::SolanaAccountMetaPtr> account_metas;
  account_metas.push_back(std::move(solana_account_meta1));
  account_metas.push_back(std::move(solana_account_meta2));
  auto mojom_instruction = mojom::SolanaInstruction::New(
      kSolanaSystemProgramId, std::move(account_metas), data);

  std::vector<mojom::SolanaInstructionPtr> instructions;
  instructions.push_back(std::move(mojom_instruction));

  ASSERT_TRUE(ti->tx_data_union->is_solana_tx_data());
  EXPECT_EQ(ti->tx_data_union->get_solana_tx_data(),
            mojom::SolanaTxData::New(recent_blockhash, from_account,
                                     std::move(instructions)));
}

TEST(SolanaTxMetaUnitTest, ToValue) {
  std::string from_account = "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8";
  std::string to_account = "JDqrvDz8d8tFCADashbUKQDKfJZFobNy13ugN65t1wvV";
  std::string recent_blockhash = "9sHcv6xwn9YkB8nxTUGKDwPwNnmqVp5oAXxU8Fdkm4J6";
  const std::vector<uint8_t> data = {2, 0, 0, 0, 128, 150, 152, 0, 0, 0, 0, 0};

  SolanaInstruction instruction(
      // Program ID
      kSolanaSystemProgramId,
      // Accounts
      {SolanaAccountMeta(from_account, true, true),
       SolanaAccountMeta(to_account, false, true)},
      data);
  auto tx = std::make_unique<SolanaTransaction>(
      recent_blockhash, from_account,
      std::vector<SolanaInstruction>({instruction}));

  SolanaTxMeta meta(std::move(tx));
  SolanaSignatureStatus status(82, 10, "", "confirmed");
  meta.set_signature_status(status);

  meta.set_id("meta_id");
  meta.set_status(mojom::TransactionStatus::Confirmed);
  meta.set_from("BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8");
  base::Time::Exploded x{1981, 3, 0, 1, 2};
  base::Time confirmed_time = meta.confirmed_time();
  EXPECT_TRUE(base::Time::FromUTCExploded(x, &confirmed_time));
  meta.set_confirmed_time(confirmed_time);
  meta.set_submitted_time(confirmed_time - base::Seconds(3));
  meta.set_created_time(confirmed_time - base::Minutes(1));
  meta.set_tx_hash(
      "5VERv8NMvzbJMEkV8xnrLkEaWRtSz9CosKDYjCJjBRnbJLgp8uirBgmQpjKhoR4tjF3ZpRzr"
      "FmBV6UjKdiSZkQUW");

  base::Value value = meta.ToValue();
  auto expect_value = base::JSONReader::Read(R"(
    {
      "id": "meta_id",
      "status": 4,
      "from": "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8",
      "tx_hash": "5VERv8NMvzbJMEkV8xnrLkEaWRtSz9CosKDYjCJjBRnbJLgp8uirBgmQpjKhoR4tjF3ZpRzrFmBV6UjKdiSZkQUW",
      "confirmed_time": "11996733600000000",
      "created_time": "11996733540000000",
      "submitted_time": "11996733597000000",
      "tx": {
        "message": {
          "recent_blockhash": "9sHcv6xwn9YkB8nxTUGKDwPwNnmqVp5oAXxU8Fdkm4J6",
          "fee_payer": "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8",
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
              "data": "AgAAAICWmAAAAAAA"
            }
          ]
        }
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
