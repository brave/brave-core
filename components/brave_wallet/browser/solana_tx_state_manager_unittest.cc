/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_tx_state_manager.h"

#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "base/files/scoped_temp_dir.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/solana_account_meta.h"
#include "brave/components/brave_wallet/browser/solana_instruction.h"
#include "brave/components/brave_wallet/browser/solana_transaction.h"
#include "brave/components/brave_wallet/browser/solana_tx_meta.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/browser/tx_storage_delegate_impl.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

class SolanaTxStateManagerUnitTest : public testing::Test {
 public:
  SolanaTxStateManagerUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}
  ~SolanaTxStateManagerUnitTest() override = default;

 protected:
  void SetUp() override {
    RegisterProfilePrefs(prefs_.registry());
    RegisterProfilePrefsForMigration(prefs_.registry());
    factory_ = GetTestValueStoreFactory(temp_dir_);
    delegate_ = GetTxStorageDelegateForTest(GetPrefs(), factory_);
    account_resolver_delegate_ =
        std::make_unique<AccountResolverDelegateForTest>();
    solana_tx_state_manager_ = std::make_unique<SolanaTxStateManager>(
        *delegate_, *account_resolver_delegate_);
  }

  PrefService* GetPrefs() { return &prefs_; }

  base::test::TaskEnvironment task_environment_;
  base::ScopedTempDir temp_dir_;
  scoped_refptr<value_store::TestValueStoreFactory> factory_;
  std::unique_ptr<TxStorageDelegateImpl> delegate_;
  std::unique_ptr<AccountResolverDelegateForTest> account_resolver_delegate_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  std::unique_ptr<SolanaTxStateManager> solana_tx_state_manager_;
};

TEST_F(SolanaTxStateManagerUnitTest, SolanaTxMetaAndValue) {
  std::string from_account = "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8";
  auto sol_account = account_resolver_delegate_->RegisterAccount(
      MakeAccountId(mojom::CoinType::SOL, mojom::KeyringId::kSolana,
                    mojom::AccountKind::kDerived, from_account));
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

  SolanaTxMeta meta(sol_account, std::move(tx));
  meta.set_signature_status(SolanaSignatureStatus(82, 10, "", "confirmed"));
  meta.set_id(TxMeta::GenerateMetaID());
  meta.set_status(mojom::TransactionStatus::Submitted);
  meta.set_created_time(base::Time::Now());
  meta.set_submitted_time(base::Time::Now());
  meta.set_confirmed_time(base::Time::Now());
  meta.set_tx_hash(
      "5VERv8NMvzbJMEkV8xnrLkEaWRtSz9CosKDYjCJjBRnbJLgp8uirBgmQpjKhoR4tjF3ZpRzr"
      "FmBV6UjKdiSZkQUW");
  meta.set_origin(url::Origin::Create(GURL("https://test.brave.com/")));
  meta.set_chain_id(mojom::kSolanaMainnet);

  base::Value::Dict meta_value = meta.ToValue();
  auto meta_from_value =
      solana_tx_state_manager_->ValueToSolanaTxMeta(meta_value);
  ASSERT_TRUE(meta_from_value);
  EXPECT_EQ(*meta_from_value, meta);
}

}  // namespace brave_wallet
