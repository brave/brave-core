/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/keyring_service_prefs.h"

#include "base/strings/string_number_conversions.h"
#include "base/test/values_test_util.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::ElementsAre;

namespace brave_wallet {

class KeyringServicePrefsUnitTest : public testing::Test {
 public:
  PrefService* GetPrefs() { return &profile_prefs_; }

  void SetUp() override {
    testing::Test::SetUp();
    RegisterProfilePrefs(profile_prefs_.registry());
  }

 private:
  sync_preferences::TestingPrefServiceSyncable profile_prefs_;
};

TEST_F(KeyringServicePrefsUnitTest, GenerateNextAccountIndex) {
  const mojom::KeyringId keyrings[] = {
      mojom::KeyringId::kBitcoinImport, mojom::KeyringId::kBitcoinImportTestnet,
      mojom::KeyringId::kBitcoinHardware,
      mojom::KeyringId::kBitcoinHardwareTestnet};

  for (auto keyring_id : keyrings) {
    EXPECT_EQ(0u, GenerateNextAccountIndex(GetPrefs(), keyring_id));
    EXPECT_EQ(1u, GenerateNextAccountIndex(GetPrefs(), keyring_id));
    EXPECT_EQ(2u, GenerateNextAccountIndex(GetPrefs(), keyring_id));
  }

  EXPECT_EQ(
      3, *GetPrefs()
              ->GetDict(kBraveWalletKeyrings)
              .FindIntByDottedPath("bitcoin_import_test.next_account_index"));

  EXPECT_EQ(3u, GenerateNextAccountIndex(GetPrefs(),
                                         mojom::KeyringId::kBitcoinHardware));
  EXPECT_EQ(4,
            *GetPrefs()
                 ->GetDict(kBraveWalletKeyrings)
                 .FindIntByDottedPath("bitcoin_hardware.next_account_index"));
}

TEST_F(KeyringServicePrefsUnitTest, HardwareAccountsForKeyring) {
  const mojom::KeyringId keyrings[] = {
      mojom::KeyringId::kBitcoinHardware,
      mojom::KeyringId::kBitcoinHardwareTestnet};
  for (auto keyring_id : keyrings) {
    EXPECT_EQ(0u, GetHardwareAccountsForKeyring(GetPrefs(), keyring_id).size());
    HardwareAccountInfo acc_0(keyring_id, 0, "acc 0",
                              mojom::HardwareVendor::kLedger, "derivation path",
                              "device id");
    acc_0.bitcoin_next_receive_address_index = 2;
    acc_0.bitcoin_next_change_address_index = 1;
    acc_0.bitcoin_xpub = "xpub0";
    AddHardwareAccountToPrefs(GetPrefs(), acc_0);
    EXPECT_THAT(GetHardwareAccountsForKeyring(GetPrefs(), keyring_id),
                ElementsAre(acc_0));

    HardwareAccountInfo acc_1(keyring_id, 1, "acc 1",
                              mojom::HardwareVendor::kLedger, "derivation path",
                              "device id");
    acc_1.bitcoin_next_receive_address_index = 3;
    acc_1.bitcoin_next_change_address_index = 4;
    acc_1.bitcoin_xpub = "xpub1";
    AddHardwareAccountToPrefs(GetPrefs(), acc_1);
    EXPECT_THAT(GetHardwareAccountsForKeyring(GetPrefs(), keyring_id),
                ElementsAre(acc_0, acc_1));

    RemoveHardwareAccountFromPrefs(GetPrefs(), *acc_0.GetAccountId());
    EXPECT_THAT(GetHardwareAccountsForKeyring(GetPrefs(), keyring_id),
                ElementsAre(acc_1));
  }

  auto accounts = GetHardwareAccountsForKeyring(
      GetPrefs(), mojom::KeyringId::kBitcoinHardware);
  accounts[0].account_name = "Some new name";
  SetHardwareAccountsForKeyring(GetPrefs(), mojom::KeyringId::kBitcoinHardware,
                                accounts);

  EXPECT_EQ(*GetPrefs()
                 ->GetDict(kBraveWalletKeyrings)
                 .FindDictByDottedPath("bitcoin_hardware"),
            base::test::ParseJson(R"(
  {
    "account_metas": [
      {
        "account_index": 1,
        "account_name": "Some new name",
        "bitcoin": {
            "next_change": "4",
            "next_receive": "3",
            "xpub": "xpub1"
        },
        "hardware": {
            "derivation_path": "derivation path",
            "device_id": "device id",
            "vendor": "Ledger"
        }
      }
    ]
  }
  )"));
}

}  // namespace brave_wallet
