/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_tx_state_manager.h"

#include <utility>
#include <vector>

#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/solana_account_meta.h"
#include "brave/components/brave_wallet/browser/solana_instruction.h"
#include "brave/components/brave_wallet/browser/solana_transaction.h"
#include "brave/components/brave_wallet/browser/solana_tx_meta.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

class SolanaTxStateManagerUnitTest : public testing::Test {
 public:
  SolanaTxStateManagerUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME),
        shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {}
  ~SolanaTxStateManagerUnitTest() override {}

 protected:
  void SetUp() override {
    brave_wallet::RegisterProfilePrefs(prefs_.registry());
    json_rpc_service_.reset(
        new JsonRpcService(shared_url_loader_factory_, GetPrefs()));
    solana_tx_state_manager_.reset(
        new SolanaTxStateManager(GetPrefs(), json_rpc_service_.get()));
  }

  void SetNetwork(const std::string& chain_id) {
    ASSERT_TRUE(json_rpc_service_->SetNetwork(chain_id, mojom::CoinType::SOL));
  }

  PrefService* GetPrefs() { return &prefs_; }

  base::test::TaskEnvironment task_environment_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  std::unique_ptr<JsonRpcService> json_rpc_service_;
  std::unique_ptr<SolanaTxStateManager> solana_tx_state_manager_;
};

TEST_F(SolanaTxStateManagerUnitTest, SolanaTxMetaAndValue) {
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
  meta.set_signature_status(SolanaSignatureStatus(82, 10, "", "confirmed"));
  meta.set_id(TxMeta::GenerateMetaID());
  meta.set_status(mojom::TransactionStatus::Submitted);
  meta.set_from(from_account);
  meta.set_created_time(base::Time::Now());
  meta.set_submitted_time(base::Time::Now());
  meta.set_confirmed_time(base::Time::Now());
  meta.set_tx_hash(
      "5VERv8NMvzbJMEkV8xnrLkEaWRtSz9CosKDYjCJjBRnbJLgp8uirBgmQpjKhoR4tjF3ZpRzr"
      "FmBV6UjKdiSZkQUW");

  base::Value meta_value = meta.ToValue();
  auto meta_from_value =
      solana_tx_state_manager_->ValueToSolanaTxMeta(meta_value);
  ASSERT_TRUE(meta_from_value);
  EXPECT_EQ(*meta_from_value, meta);
}

TEST_F(SolanaTxStateManagerUnitTest, GetTxPrefPathPrefix) {
  EXPECT_EQ("solana.mainnet", solana_tx_state_manager_->GetTxPrefPathPrefix());
  SetNetwork("0x66");
  EXPECT_EQ("solana.testnet", solana_tx_state_manager_->GetTxPrefPathPrefix());
  SetNetwork("0x67");
  EXPECT_EQ("solana.devnet", solana_tx_state_manager_->GetTxPrefPathPrefix());
  SetNetwork(brave_wallet::mojom::kLocalhostChainId);
  EXPECT_EQ("solana.http://localhost:8899/",
            solana_tx_state_manager_->GetTxPrefPathPrefix());
}

}  // namespace brave_wallet
