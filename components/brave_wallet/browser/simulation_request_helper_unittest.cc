/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>
#include <vector>

#include "base/test/values_test_util.h"
#include "brave/components/brave_wallet/browser/eip1559_transaction.h"
#include "brave/components/brave_wallet/browser/eth_transaction.h"
#include "brave/components/brave_wallet/browser/eth_tx_meta.h"
#include "brave/components/brave_wallet/browser/json_rpc_requests_helper.h"
#include "brave/components/brave_wallet/browser/simulation_request_helper.h"
#include "brave/components/brave_wallet/browser/solana_transaction.h"
#include "brave/components/brave_wallet/browser/solana_tx_meta.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/origin.h"

using base::test::ParseJson;

namespace brave_wallet {

namespace {

mojom::TransactionInfoPtr GetCannedScanEVMTransactionParams(
    bool eip1559,
    bool is_eth_send,
    absl::optional<std::string> origin) {
  auto base_tx_data = mojom::TxData::New(
      "0x09", "0x4a817c800", "0x5208",
      "0x3535353535353535353535353535353535353535", "0xde0b6b3a7640000",
      is_eth_send ? std::vector<uint8_t>() : std::vector<uint8_t>({10u}), false,
      absl::nullopt);

  auto tx =
      eip1559
          ? std::make_unique<Eip1559Transaction>(
                *Eip1559Transaction::FromTxData(mojom::TxData1559::New(
                    std::move(base_tx_data), "0x3", "0x1E", "0x32", nullptr)))
          : std::make_unique<EthTransaction>(
                *EthTransaction::FromTxData(std::move(base_tx_data)));

  auto eth_account =
      MakeAccountId(mojom::CoinType::ETH, mojom::KeyringId::kDefault,
                    mojom::AccountKind::kDerived,
                    "0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4");
  EthTxMeta meta(eth_account, std::move(tx));

  if (origin) {
    meta.set_origin(url::Origin::Create(GURL(*origin)));
  }

  return meta.ToTransactionInfo();
}

mojom::TransactionInfoPtr GetCannedScanSolanaTransactionParams(
    absl::optional<std::string> origin) {
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
      {SolanaAccountMeta(from_account, absl::nullopt, true, true),
       SolanaAccountMeta(to_account, absl::nullopt, false, true)},
      data);

  auto msg = SolanaMessage::CreateLegacyMessage(
      recent_blockhash, last_valid_block_height, from_account,
      std::vector<SolanaInstruction>({instruction}));
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

  if (origin) {
    meta.set_origin(url::Origin::Create(GURL(*origin)));
  }

  return meta.ToTransactionInfo();
}

mojom::SignTransactionRequestPtr MakeSolanaSignTransactionRequest(
    mojom::TransactionInfoPtr tx_info) {
  auto request = mojom::SignTransactionRequest::New();
  request->origin_info = std::move(tx_info->origin_info);
  request->id = 1;
  request->from_account_id = tx_info->from_account_id->Clone();
  request->from_address = *tx_info->from_address;
  request->tx_data = tx_info->tx_data_union.Clone();
  request->coin = mojom::CoinType::SOL;

  auto tx = SolanaTransaction::FromSolanaTxData(
      std::move(tx_info->tx_data_union->get_solana_tx_data()));
  const auto& tx_b64 = tx->GetBase64EncodedMessage();
  request->raw_message = mojom::ByteArrayStringUnion::NewStr(tx_b64);
  return request;
}

mojom::SignAllTransactionsRequestPtr MakeSolanaSignAllTransactionsRequest(
    mojom::TransactionInfoPtr tx_info) {
  auto request = mojom::SignAllTransactionsRequest::New();
  request->origin_info = std::move(tx_info->origin_info);
  request->id = 1;
  request->from_account_id = tx_info->from_account_id->Clone();
  request->from_address = *tx_info->from_address;

  request->tx_datas.push_back(tx_info->tx_data_union.Clone());
  request->coin = mojom::CoinType::SOL;

  auto tx = SolanaTransaction::FromSolanaTxData(
      std::move(tx_info->tx_data_union->get_solana_tx_data()));
  const auto& tx_b64 = tx->GetBase64EncodedMessage();

  request->raw_messages.push_back(mojom::ByteArrayStringUnion::NewStr(tx_b64));
  return request;
}

}  // namespace

TEST(SimulationRequestHelperUnitTest,
     EncodeScanTransactionParamsEVMType0DefaultOrigin) {
  // OK: Params for legacy (type-0) EVM native asset transfer is encoded
  // correctly.
  auto tx_info = GetCannedScanEVMTransactionParams(false, true, absl::nullopt);
  auto encoded_params = evm::EncodeScanTransactionParams(tx_info);
  std::string expected_params(R"(
    {
      "metadata":{
        "origin":"https://brave.com"
      },
      "txObject":{
        "data":"0x",
        "from":"0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4",
        "to":"0x3535353535353535353535353535353535353535",
        "value":"0xde0b6b3a7640000"
      },
      "userAccount":"0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4"
    }
  )");
  ASSERT_TRUE(encoded_params);
  EXPECT_EQ(*encoded_params, GetJSON(ParseJson(expected_params)));

  // OK: Params for legacy (type-0) EVM contract interaction is encoded
  // properly.
  tx_info = GetCannedScanEVMTransactionParams(false, false, absl::nullopt);
  encoded_params = evm::EncodeScanTransactionParams(tx_info);
  expected_params = R"(
    {
      "metadata":{
        "origin":"https://brave.com"
      },
      "txObject":{
        "data":"0x0a",
        "from":"0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4",
        "to":"0x3535353535353535353535353535353535353535",
        "value":"0xde0b6b3a7640000"
      },
      "userAccount":"0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4"
    }
  )";
  ASSERT_TRUE(encoded_params);
  EXPECT_EQ(*encoded_params, GetJSON(ParseJson(expected_params)));
}

TEST(SimulationRequestHelperUnitTest,
     EncodeScanTransactionParamsEVMType2DefaultOrigin) {
  // OK: Params for EIP-1559 (type-2) EVM ETH transfer is encoded correctly.
  auto tx_info = GetCannedScanEVMTransactionParams(true, true, absl::nullopt);
  auto encoded_params = evm::EncodeScanTransactionParams(tx_info);
  std::string expected_params(R"(
    {
      "metadata":{
        "origin":"https://brave.com"
      },
      "txObject":{
        "data":"0x",
        "from":"0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4",
        "to":"0x3535353535353535353535353535353535353535",
        "value":"0xde0b6b3a7640000"
      },
      "userAccount":"0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4"
    }
  )");
  ASSERT_TRUE(encoded_params);
  EXPECT_EQ(*encoded_params, GetJSON(ParseJson(expected_params)));

  // OK: Params for EIP-1559 (type-2) EVM contract interaction is encoded
  // correctly.
  tx_info = GetCannedScanEVMTransactionParams(true, false, absl::nullopt);
  encoded_params = evm::EncodeScanTransactionParams(tx_info);
  expected_params = R"(
    {
      "metadata":{
        "origin":"https://brave.com"
      },
      "txObject":{
        "data":"0x0a",
        "from":"0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4",
        "to":"0x3535353535353535353535353535353535353535",
        "value":"0xde0b6b3a7640000"
      },
      "userAccount":"0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4"
    }
  )";
  ASSERT_TRUE(encoded_params);
  EXPECT_EQ(*encoded_params, GetJSON(ParseJson(expected_params)));
}

TEST(SimulationRequestHelperUnitTest,
     EncodeScanTransactionParamsEVMType0CustomOrigin) {
  // OK: Custom origin for ETH transfer is encoded in params metadata
  // correctly.
  auto tx_info =
      GetCannedScanEVMTransactionParams(false, true, "https://example.com");
  auto encoded_params = evm::EncodeScanTransactionParams(tx_info);
  std::string expected_params(R"(
    {
      "metadata":{
        "origin":"https://example.com"
      },
      "txObject":{
        "data":"0x",
        "from":"0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4",
        "to":"0x3535353535353535353535353535353535353535",
        "value":"0xde0b6b3a7640000"
      },
      "userAccount":"0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4"
    }
  )");
  ASSERT_TRUE(encoded_params);
  EXPECT_EQ(*encoded_params, GetJSON(ParseJson(expected_params)));

  // OK: Custom origin for contract interaction is encoded in params metadata
  // correctly.
  tx_info =
      GetCannedScanEVMTransactionParams(false, false, "https://example.com");
  encoded_params = evm::EncodeScanTransactionParams(tx_info);
  expected_params = R"(
    {
      "metadata":{
        "origin":"https://example.com"
      },
      "txObject":{
        "data":"0x0a",
        "from":"0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4",
        "to":"0x3535353535353535353535353535353535353535",
        "value":"0xde0b6b3a7640000"
      },
      "userAccount":"0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4"
    }
  )";
  ASSERT_TRUE(encoded_params);
  EXPECT_EQ(*encoded_params, GetJSON(ParseJson(expected_params)));
}

TEST(SimulationRequestHelperUnitTest,
     EncodeScanTransactionParamsEVMType2CustomOrigin) {
  // OK: Custom origin for ETH transfer is encoded in params metadata
  // correctly.
  auto tx_info =
      GetCannedScanEVMTransactionParams(true, true, "https://example.com");
  auto encoded_params = evm::EncodeScanTransactionParams(tx_info);
  std::string expected_params(R"(
    {
      "metadata":{
        "origin":"https://example.com"
      },
      "txObject":{
        "data":"0x",
        "from":"0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4",
        "to":"0x3535353535353535353535353535353535353535",
        "value":"0xde0b6b3a7640000"
      },
      "userAccount":"0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4"
    }
  )");
  ASSERT_TRUE(encoded_params);
  EXPECT_EQ(*encoded_params, GetJSON(ParseJson(expected_params)));

  // OK: Custom origin for contract interaction is encoded in params metadata
  // correctly.
  tx_info =
      GetCannedScanEVMTransactionParams(true, false, "https://example.com");
  encoded_params = evm::EncodeScanTransactionParams(tx_info);
  expected_params = R"(
    {
      "metadata":{
        "origin":"https://example.com"
      },
      "txObject":{
        "data":"0x0a",
        "from":"0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4",
        "to":"0x3535353535353535353535353535353535353535",
        "value":"0xde0b6b3a7640000"
      },
      "userAccount":"0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4"
    }
  )";
  ASSERT_TRUE(encoded_params);
  EXPECT_EQ(*encoded_params, GetJSON(ParseJson(expected_params)));
}

TEST(SimulationRequestHelperUnitTest,
     EncodeScanTransactionParamsEVMInvalidTxData) {
  auto tx_info = GetCannedScanSolanaTransactionParams(absl::nullopt);
  auto encoded_params = evm::EncodeScanTransactionParams(tx_info);

  // KO: Invalid tx data is not encoded.
  EXPECT_FALSE(encoded_params);
}

TEST(SimulationRequestHelperUnitTest,
     EncodeScanTransactionParamsEVMNullParams) {
  EXPECT_EQ(evm::EncodeScanTransactionParams(nullptr), absl::nullopt);
}

TEST(SimulationRequestHelperUnitTest,
     EncodeScanTransactionParamsSolanaTransactionInfoDefaultOrigin) {
  auto tx_info = GetCannedScanSolanaTransactionParams(absl::nullopt);
  auto request = mojom::SolanaTransactionRequestUnion::NewTransactionInfo(
      std::move(tx_info));
  auto encoded_params = solana::EncodeScanTransactionParams(request);

  std::string expected_params(R"(
    {
      "metadata":{
        "origin":"https://brave.com"
      },
      "transactions":[
        "AQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABAAEDoTNZW3PS2dRMn6vIKJadRsVHGCzRbI8EOvvXPsmsn8X/4OT1Xu4XhM4oUvnby2eebttd+Y+Gz6yzTEMGqaSVJgAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAg79TyWzB3v+wQ4jR2yoGqfCJjrmpBhFXewYqN6JAeFsBAgIAAQwCAAAAgJaYAAAAAAA="
      ],
      "userAccount":"BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8"
    }
  )");

  // OK: Params for Solana TransactionInfo is encoded correctly.
  ASSERT_TRUE(encoded_params);
  EXPECT_EQ(*encoded_params, GetJSON(ParseJson(expected_params)));
}

TEST(SimulationRequestHelperUnitTest,
     EncodeScanTransactionParamsSolanaTransactionInfoCustomOrigin) {
  auto tx_info = GetCannedScanSolanaTransactionParams("https://example.com");
  auto request = mojom::SolanaTransactionRequestUnion::NewTransactionInfo(
      std::move(tx_info));
  auto encoded_params = solana::EncodeScanTransactionParams(request);

  std::string expected_params(R"(
    {
      "metadata":{
        "origin":"https://example.com"
      },
      "transactions":[
        "AQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABAAEDoTNZW3PS2dRMn6vIKJadRsVHGCzRbI8EOvvXPsmsn8X/4OT1Xu4XhM4oUvnby2eebttd+Y+Gz6yzTEMGqaSVJgAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAg79TyWzB3v+wQ4jR2yoGqfCJjrmpBhFXewYqN6JAeFsBAgIAAQwCAAAAgJaYAAAAAAA="
      ],
      "userAccount":"BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8"
    }
  )");

  // OK: Params for Solana TransactionInfo is encoded correctly.
  ASSERT_TRUE(encoded_params);
  EXPECT_EQ(*encoded_params, GetJSON(ParseJson(expected_params)));
}

TEST(SimulationRequestHelperUnitTest,
     EncodeScanTransactionParamsSolanaSignTransactionRequestDefaultOrigin) {
  auto tx_info = GetCannedScanSolanaTransactionParams(absl::nullopt);

  auto parsed = MakeSolanaSignTransactionRequest(std::move(tx_info));
  auto request =
      mojom::SolanaTransactionRequestUnion::NewSignTransactionRequest(
          std::move(parsed));
  auto encoded_params = solana::EncodeScanTransactionParams(request);

  std::string expected_params(R"(
    {
      "metadata":{
        "origin":"https://brave.com"
      },
      "transactions":[
        "AQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABAAEDoTNZW3PS2dRMn6vIKJadRsVHGCzRbI8EOvvXPsmsn8X/4OT1Xu4XhM4oUvnby2eebttd+Y+Gz6yzTEMGqaSVJgAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAg79TyWzB3v+wQ4jR2yoGqfCJjrmpBhFXewYqN6JAeFsBAgIAAQwCAAAAgJaYAAAAAAA="
      ],
      "userAccount":"BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8"
    }
  )");

  // OK: Params for Solana TransactionInfo is encoded correctly.
  ASSERT_TRUE(encoded_params);
  EXPECT_EQ(*encoded_params, GetJSON(ParseJson(expected_params)));
}

TEST(SimulationRequestHelperUnitTest,
     EncodeScanTransactionParamsSolanaSignTransactionRequestCustomOrigin) {
  auto tx_info = GetCannedScanSolanaTransactionParams("https://example.com");

  auto parsed = MakeSolanaSignTransactionRequest(std::move(tx_info));
  auto request =
      mojom::SolanaTransactionRequestUnion::NewSignTransactionRequest(
          std::move(parsed));
  auto encoded_params = solana::EncodeScanTransactionParams(request);

  std::string expected_params(R"(
    {
      "metadata":{
        "origin":"https://example.com"
      },
      "transactions":[
        "AQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABAAEDoTNZW3PS2dRMn6vIKJadRsVHGCzRbI8EOvvXPsmsn8X/4OT1Xu4XhM4oUvnby2eebttd+Y+Gz6yzTEMGqaSVJgAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAg79TyWzB3v+wQ4jR2yoGqfCJjrmpBhFXewYqN6JAeFsBAgIAAQwCAAAAgJaYAAAAAAA="
      ],
      "userAccount":"BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8"
    }
  )");

  // OK: Params for Solana TransactionInfo is encoded correctly.
  ASSERT_TRUE(encoded_params);
  EXPECT_EQ(*encoded_params, GetJSON(ParseJson(expected_params)));
}

TEST(SimulationRequestHelperUnitTest,
     EncodeScanTransactionParamsSolanaSignAllTransactionsRequestDefaultOrigin) {
  auto tx_info = GetCannedScanSolanaTransactionParams(absl::nullopt);

  auto parsed = MakeSolanaSignAllTransactionsRequest(std::move(tx_info));
  auto request =
      mojom::SolanaTransactionRequestUnion::NewSignAllTransactionsRequest(
          std::move(parsed));
  auto encoded_params = solana::EncodeScanTransactionParams(request);

  std::string expected_params(R"(
    {
      "metadata":{
        "origin":"https://brave.com"
      },
      "transactions":[
        "AQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABAAEDoTNZW3PS2dRMn6vIKJadRsVHGCzRbI8EOvvXPsmsn8X/4OT1Xu4XhM4oUvnby2eebttd+Y+Gz6yzTEMGqaSVJgAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAg79TyWzB3v+wQ4jR2yoGqfCJjrmpBhFXewYqN6JAeFsBAgIAAQwCAAAAgJaYAAAAAAA="
      ],
      "userAccount":"BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8"
    }
  )");

  // OK: Params for Solana TransactionInfo is encoded correctly.
  ASSERT_TRUE(encoded_params);
  EXPECT_EQ(*encoded_params, GetJSON(ParseJson(expected_params)));
}

TEST(SimulationRequestHelperUnitTest,
     EncodeScanTransactionParamsSolanaSignAllTransactionsRequestCustomOrigin) {
  auto tx_info = GetCannedScanSolanaTransactionParams("https://example.com");

  auto parsed = MakeSolanaSignAllTransactionsRequest(std::move(tx_info));
  auto request =
      mojom::SolanaTransactionRequestUnion::NewSignAllTransactionsRequest(
          std::move(parsed));
  auto encoded_params = solana::EncodeScanTransactionParams(request);

  std::string expected_params(R"(
    {
      "metadata":{
        "origin":"https://example.com"
      },
      "transactions":[
        "AQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABAAEDoTNZW3PS2dRMn6vIKJadRsVHGCzRbI8EOvvXPsmsn8X/4OT1Xu4XhM4oUvnby2eebttd+Y+Gz6yzTEMGqaSVJgAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAg79TyWzB3v+wQ4jR2yoGqfCJjrmpBhFXewYqN6JAeFsBAgIAAQwCAAAAgJaYAAAAAAA="
      ],
      "userAccount":"BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8"
    }
  )");

  // OK: Params for Solana TransactionInfo is encoded correctly.
  ASSERT_TRUE(encoded_params);
  EXPECT_EQ(*encoded_params, GetJSON(ParseJson(expected_params)));
}

TEST(SimulationRequestHelperUnitTest,
     EncodeScanTransactionParamsSolanaInvalidTxData) {
  auto tx_info = GetCannedScanEVMTransactionParams(false, true, absl::nullopt);
  auto request = mojom::SolanaTransactionRequestUnion::NewTransactionInfo(
      std::move(tx_info));
  auto encoded_params = solana::EncodeScanTransactionParams(request);

  // KO: Invalid tx data is not encoded.
  EXPECT_FALSE(encoded_params);
}

TEST(SimulationRequestHelperUnitTest,
     EncodeScanTransactionParamsSolanaNullParams) {
  EXPECT_EQ(solana::EncodeScanTransactionParams(nullptr), absl::nullopt);
}

}  // namespace brave_wallet
