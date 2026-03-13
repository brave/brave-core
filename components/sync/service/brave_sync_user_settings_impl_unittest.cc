/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/test/task_environment.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "components/saved_tab_groups/public/pref_names.h"
#include "components/signin/public/base/signin_prefs.h"
#include "components/signin/public/identity_manager/identity_manager.h"
#include "components/sync/base/features.h"
#include "components/sync/service/glue/sync_transport_data_prefs.h"
#include "components/sync/service/sync_service_crypto.h"
#include "components/sync/service/sync_user_settings_impl.h"
#include "components/sync/test/sync_service_crypto_test_utils.h"
#include "components/trusted_vault/test/fake_trusted_vault_client.h"
#include "google_apis/gaia/gaia_id.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// This is re-worked from SyncUserSettingsImplTest

namespace syncer {

namespace {

using testing::ContainerEq;
using testing::Return;

constexpr GaiaId::Literal kTestGaiaId("1111");

DataTypeSet GetUserTypes() {
  DataTypeSet user_types = UserTypes();
  // Ignore all Chrome OS types on non-Chrome OS platforms.
  user_types.RemoveAll({APP_LIST, ARC_PACKAGE, OS_PREFERENCES,
                        OS_PRIORITY_PREFERENCES, PRINTERS,
                        PRINTERS_AUTHORIZATION_SERVERS, WIFI_CONFIGURATIONS});
  return user_types;
}

class MockSyncServiceCryptoDelegate : public SyncServiceCrypto::Delegate {
 public:
  MockSyncServiceCryptoDelegate() = default;
  ~MockSyncServiceCryptoDelegate() override = default;

  MOCK_METHOD(void, CryptoStateChanged, (), (override));
  MOCK_METHOD(void, CryptoRequiredUserActionChanged, (), (override));
  MOCK_METHOD(void, ReconfigureDataTypesDueToCrypto, (), (override));
  MOCK_METHOD(void, PassphraseTypeChanged, (PassphraseType), (override));
  MOCK_METHOD(std::optional<PassphraseType>,
              GetPassphraseType,
              (),
              (const override));
  MOCK_METHOD(void,
              SetEncryptionBootstrapToken,
              (const std::string&),
              (override));
  MOCK_METHOD(std::string, GetEncryptionBootstrapToken, (), (const override));
};

class MockDelegate : public SyncUserSettingsImpl::Delegate {
 public:
  MockDelegate() = default;
  ~MockDelegate() override = default;

  MOCK_METHOD(bool, IsCustomPassphraseAllowed, (), (const override));
  MOCK_METHOD(SyncPrefs::SyncAccountState,
              GetSyncAccountStateForPrefs,
              (),
              (const override));
  MOCK_METHOD(CoreAccountInfo,
              GetSyncAccountInfoForPrefs,
              (),
              (const override));
  MOCK_METHOD(void, OnSyncClientDisabledByPolicyChanged, (), (override));
  MOCK_METHOD(void, OnSelectedTypesChanged, (), (override));
  MOCK_METHOD(void, OnInitialSyncFeatureSetupCompleted, (), (override));
};

class BraveSyncUserSettingsImplTest : public testing::Test {
 protected:
  BraveSyncUserSettingsImplTest() {
    SyncPrefs::RegisterProfilePrefs(pref_service_.registry());
    SigninPrefs::RegisterProfilePrefs(pref_service_.registry());
    SyncTransportDataPrefs::RegisterProfilePrefs(pref_service_.registry());
    signin::IdentityManager::RegisterProfilePrefs(pref_service_.registry());
    // TODO(crbug.com/368409110): Necessary for a workaround in
    // SyncPrefs::KeepAccountSettingsPrefsOnlyForUsers(); see TODO there.
    pref_service_.registry()->RegisterDictionaryPref(
        tab_groups::prefs::kLocallyClosedRemoteTabGroupIds, base::DictValue());
    sync_prefs_ = std::make_unique<SyncPrefs>(&pref_service_);

    sync_service_crypto_ = std::make_unique<SyncServiceCrypto>(
        &sync_service_crypto_delegate_, &trusted_vault_client_);
    sync_service_crypto_->SetEncryptor(
        std::make_unique<os_crypt_async::Encryptor>(GetEncryptorForTest()));

    ON_CALL(delegate_, IsCustomPassphraseAllowed).WillByDefault(Return(true));
    ON_CALL(delegate_, GetSyncAccountStateForPrefs)
        .WillByDefault(Return(SyncPrefs::SyncAccountState::kSyncing));
    ON_CALL(delegate_, GetSyncAccountInfoForPrefs).WillByDefault([]() {
      CoreAccountInfo account;
      account.email = "name@account.com";
      account.gaia = kTestGaiaId;
      account.account_id = CoreAccountId::FromGaiaId(account.gaia);
      return account;
    });
  }

  std::unique_ptr<SyncUserSettingsImpl> MakeSyncUserSettings(
      DataTypeSet registered_types) {
    return std::make_unique<SyncUserSettingsImpl>(
        &delegate_, sync_service_crypto_.get(), sync_prefs_.get(),
        registered_types);
  }

  base::test::SingleThreadTaskEnvironment task_environment_;
  // The order of fields matters because it determines destruction order and
  // fields are dependent.
  TestingPrefServiceSimple pref_service_;
  std::unique_ptr<SyncPrefs> sync_prefs_;
  testing::NiceMock<MockSyncServiceCryptoDelegate>
      sync_service_crypto_delegate_;
  testing::NiceMock<MockDelegate> delegate_;
  trusted_vault::FakeTrustedVaultClient trusted_vault_client_;
  std::unique_ptr<SyncServiceCrypto> sync_service_crypto_;
};

TEST_F(BraveSyncUserSettingsImplTest, PreferredTypesSyncEverything) {
  std::unique_ptr<SyncUserSettingsImpl> sync_user_settings =
      MakeSyncUserSettings(GetUserTypes());
  DataTypeSet preferred_user_types =
      sync_user_settings->GetPreferredDataTypes();
  EXPECT_FALSE(preferred_user_types.Has(AUTOFILL_VALUABLE));
  EXPECT_FALSE(preferred_user_types.Has(AUTOFILL_VALUABLE_METADATA));
}

}  // namespace

}  // namespace syncer
