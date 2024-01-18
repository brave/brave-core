/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/base64.h"
#include "base/logging.h"
#include "base/test/gtest_util.h"
#include "base/test/task_environment.h"
#include "brave/components/history/core/browser/sync/brave_history_delete_directives_model_type_controller.h"
#include "brave/components/history/core/browser/sync/brave_history_model_type_controller.h"
#include "brave/components/sync/service/brave_sync_service_impl.h"
#include "brave/components/sync/service/sync_service_impl_delegate.h"
#include "brave/components/sync/test/brave_mock_sync_engine.h"
#include "build/build_config.h"
#include "components/os_crypt/sync/os_crypt.h"
#include "components/os_crypt/sync/os_crypt_mocker.h"
#include "components/sync/engine/nigori/key_derivation_params.h"
#include "components/sync/engine/nigori/nigori.h"
#include "components/sync/service/data_type_manager_impl.h"
#include "components/sync/test/data_type_manager_mock.h"
#include "components/sync/test/fake_data_type_controller.h"
#include "components/sync/test/fake_sync_api_component_factory.h"
#include "components/sync/test/fake_sync_engine.h"
#include "components/sync/test/fake_sync_manager.h"
#include "components/sync/test/sync_service_impl_bundle.h"
#include "components/sync/test/test_model_type_store_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;
using testing::ByMove;
using testing::NiceMock;
using testing::Return;

namespace syncer {

namespace {

const char kValidSyncCode[] =
    "fringe digital begin feed equal output proof cheap "
    "exotic ill sure question trial squirrel glove celery "
    "awkward push jelly logic broccoli almost grocery drift";

// Taken from anonimous namespace from sync_service_crypto_unittest.cc
sync_pb::EncryptedData MakeEncryptedData(
    const std::string& passphrase,
    const KeyDerivationParams& derivation_params) {
  std::unique_ptr<Nigori> nigori =
      Nigori::CreateByDerivation(derivation_params, passphrase);

  std::string nigori_name = nigori->GetKeyName();
  const std::string unencrypted = "test";
  sync_pb::EncryptedData encrypted;
  encrypted.set_key_name(nigori_name);
  *encrypted.mutable_blob() = nigori->Encrypt(unencrypted);
  return encrypted;
}

}  // namespace

class SyncServiceImplDelegateMock : public SyncServiceImplDelegate {
 public:
  SyncServiceImplDelegateMock() = default;
  ~SyncServiceImplDelegateMock() override = default;
  void SuspendDeviceObserverForOwnReset() override {}
  void ResumeDeviceObserver() override {}
  void SetLocalDeviceAppearedCallback(
      base::OnceCallback<void()> local_device_appeared_callback) override {}
  void GetKnownToSyncHistoryCount(
      base::OnceCallback<void(std::pair<bool, int>)> callback) override {}
};

class SyncServiceObserverMock : public SyncServiceObserver {
 public:
  SyncServiceObserverMock() {}
  ~SyncServiceObserverMock() override {}

  MOCK_METHOD(void, OnStateChanged, (SyncService * sync), (override));
  MOCK_METHOD(void, OnSyncCycleCompleted, (SyncService * sync), (override));
  MOCK_METHOD(void,
              OnSyncConfigurationCompleted,
              (SyncService * sync),
              (override));
  MOCK_METHOD(void, OnSyncShutdown, (SyncService * sync), (override));
};

class BraveSyncServiceImplTest : public testing::Test {
 public:
  BraveSyncServiceImplTest()
      : brave_sync_prefs_(sync_service_impl_bundle_.pref_service()),
        sync_prefs_(sync_service_impl_bundle_.pref_service()) {
    sync_service_impl_bundle_.identity_test_env()
        ->SetAutomaticIssueOfAccessTokens(true);
    brave_sync::Prefs::RegisterProfilePrefs(
        sync_service_impl_bundle_.pref_service()->registry());
  }

  ~BraveSyncServiceImplTest() override { sync_service_impl_->Shutdown(); }

  void CreateSyncService(
      ModelTypeSet registered_types = ModelTypeSet({BOOKMARKS})) {
    DataTypeController::TypeVector controllers;
    for (ModelType type : registered_types) {
      controllers.push_back(std::make_unique<FakeDataTypeController>(type));
    }

    std::unique_ptr<SyncClientMock> sync_client =
        sync_service_impl_bundle_.CreateSyncClientMock();
    ON_CALL(*sync_client, CreateDataTypeControllers(_))
        .WillByDefault(Return(ByMove(std::move(controllers))));

    sync_service_impl_ = std::make_unique<BraveSyncServiceImpl>(
        sync_service_impl_bundle_.CreateBasicInitParams(std::move(sync_client)),
        std::make_unique<SyncServiceImplDelegateMock>());
  }

  brave_sync::Prefs* brave_sync_prefs() { return &brave_sync_prefs_; }

  SyncPrefs* sync_prefs() { return &sync_prefs_; }

  PrefService* pref_service() {
    return sync_service_impl_bundle_.pref_service();
  }

  BraveSyncServiceImpl* brave_sync_service_impl() {
    return sync_service_impl_.get();
  }

  FakeSyncApiComponentFactory* component_factory() {
    return sync_service_impl_bundle_.component_factory();
  }

  FakeSyncEngine* engine() {
    return component_factory()->last_created_engine();
  }

  signin::IdentityManager* identity_manager() {
    return sync_service_impl_bundle_.identity_manager();
  }

 protected:
  content::BrowserTaskEnvironment task_environment_;

 private:
  SyncServiceImplBundle sync_service_impl_bundle_;
  brave_sync::Prefs brave_sync_prefs_;
  SyncPrefs sync_prefs_;
  std::unique_ptr<BraveSyncServiceImpl> sync_service_impl_;
};

TEST_F(BraveSyncServiceImplTest, ValidPassphrase) {
  OSCryptMocker::SetUp();

  CreateSyncService();

  brave_sync_service_impl()->Initialize();
  EXPECT_FALSE(engine());

  bool set_code_result = brave_sync_service_impl()->SetSyncCode(kValidSyncCode);
  EXPECT_TRUE(set_code_result);

  bool failed_to_decrypt = false;
  EXPECT_EQ(brave_sync_prefs()->GetSeed(&failed_to_decrypt), kValidSyncCode);
  EXPECT_FALSE(failed_to_decrypt);

  OSCryptMocker::TearDown();
}

TEST_F(BraveSyncServiceImplTest, InvalidPassphrase) {
  OSCryptMocker::SetUp();

  CreateSyncService();

  brave_sync_service_impl()->Initialize();
  EXPECT_FALSE(engine());

  bool set_code_result =
      brave_sync_service_impl()->SetSyncCode("word one and then two");
  EXPECT_FALSE(set_code_result);

  bool failed_to_decrypt = false;
  EXPECT_EQ(brave_sync_prefs()->GetSeed(&failed_to_decrypt), "");
  EXPECT_FALSE(failed_to_decrypt);

  OSCryptMocker::TearDown();
}

TEST_F(BraveSyncServiceImplTest, ValidPassphraseLeadingTrailingWhitespace) {
  OSCryptMocker::SetUp();

  CreateSyncService();

  brave_sync_service_impl()->Initialize();
  EXPECT_FALSE(engine());

  std::string sync_code_extra_whitespace =
      std::string(" \t\n") + kValidSyncCode + std::string(" \t\n");
  bool set_code_result =
      brave_sync_service_impl()->SetSyncCode(sync_code_extra_whitespace);
  EXPECT_TRUE(set_code_result);

  bool failed_to_decrypt = false;
  EXPECT_EQ(brave_sync_prefs()->GetSeed(&failed_to_decrypt), kValidSyncCode);
  EXPECT_FALSE(failed_to_decrypt);

  OSCryptMocker::TearDown();
}

// Google test doc strongly recommends to use `*DeathTest` naming
// for test suite
using BraveSyncServiceImplDeathTest = BraveSyncServiceImplTest;

// Some tests are failing for Windows x86 CI,
// See https://github.com/brave/brave-browser/issues/22767
#if BUILDFLAG(IS_WIN) && defined(ARCH_CPU_X86)
#define MAYBE_EmulateGetOrCreateSyncCodeCHECK \
  DISABLED_EmulateGetOrCreateSyncCodeCHECK
#else
#define MAYBE_EmulateGetOrCreateSyncCodeCHECK EmulateGetOrCreateSyncCodeCHECK
#endif
TEST_F(BraveSyncServiceImplDeathTest, MAYBE_EmulateGetOrCreateSyncCodeCHECK) {
  OSCryptMocker::SetUp();

  CreateSyncService();

  brave_sync_service_impl()->Initialize();
  EXPECT_FALSE(engine());

  // Since Chromium commit 33441a0f3f9a591693157f2fd16852ce072e6f9d
  // we cannot change datatypes if we are not signed to sync, see changes
  // around SyncUserSettingsImpl::SetSelectedTypes.
  // Call pref_service()->SetString() below triggers
  // BraveSyncServiceImpl::OnBraveSyncPrefsChanged which sets kBookmarks type.
  // To workaround this, first set valid sync code to pretend we are signed in.
  bool set_code_result = brave_sync_service_impl()->SetSyncCode(kValidSyncCode);
  EXPECT_TRUE(set_code_result);

  std::string wrong_seed = "123";
  std::string encrypted_wrong_seed;
  EXPECT_TRUE(OSCrypt::EncryptString(wrong_seed, &encrypted_wrong_seed));

  std::string encoded_wrong_seed;
  base::Base64Encode(encrypted_wrong_seed, &encoded_wrong_seed);
  pref_service()->SetString(brave_sync::Prefs::GetSeedPath(),
                            encoded_wrong_seed);

  EXPECT_CHECK_DEATH(brave_sync_service_impl()->GetOrCreateSyncCode());

  OSCryptMocker::TearDown();
}

TEST_F(BraveSyncServiceImplTest, StopAndClearForBraveSeed) {
  OSCryptMocker::SetUp();

  CreateSyncService();

  brave_sync_service_impl()->Initialize();
  EXPECT_FALSE(engine());

  bool set_code_result = brave_sync_service_impl()->SetSyncCode(kValidSyncCode);
  EXPECT_TRUE(set_code_result);

  brave_sync_service_impl()->StopAndClear();

  bool failed_to_decrypt = false;
  EXPECT_EQ(brave_sync_prefs()->GetSeed(&failed_to_decrypt), "");
  EXPECT_FALSE(failed_to_decrypt);

  OSCryptMocker::TearDown();
}

TEST_F(BraveSyncServiceImplTest, ForcedSetDecryptionPassphrase) {
  OSCryptMocker::SetUp();
  CreateSyncService();

  brave_sync_service_impl()->Initialize();
  EXPECT_FALSE(engine());
  brave_sync_service_impl()->SetSyncCode(kValidSyncCode);

  task_environment_.RunUntilIdle();

  brave_sync_service_impl()
      ->GetUserSettings()
      ->SetInitialSyncFeatureSetupComplete(
          syncer::SyncFirstSetupCompleteSource::ADVANCED_FLOW_CONFIRM);

  // Pretend we need the passphrase by triggering OnPassphraseRequired and
  // supplying the encrypted portion of data, as it is done in
  // sync_service_crypto_unittest.cc
  brave_sync_service_impl()->GetCryptoForTests()->OnPassphraseRequired(
      KeyDerivationParams::CreateForPbkdf2(),
      MakeEncryptedData(kValidSyncCode,
                        KeyDerivationParams::CreateForPbkdf2()));

  EXPECT_TRUE(
      brave_sync_service_impl()->GetUserSettings()->IsPassphraseRequired());

  // By default Brave enables Bookmarks datatype when sync is enabled.
  // This caused DCHECK at DataTypeManagerImpl::DataTypeManagerImpl
  // after OnEngineInitialized(true, false) call.
  // Current unit test is intended to verify fix for brave/brave-browser#22898
  // and is about set encryption passphrase later setup after right after
  // enabling sync, for example when internet connection is unstable. Related
  // Chromium commit 3241d114b8036bb6d53931ba34b3bf819258c29d Prior to this
  // commit DataTypeManagerImpl wasn't created for bookmarks at
  // ForcedSetDecryptionPassphrase test.
  // Update Dec 2023: moved this workaround closer to OnEngineInitialized,
  // with Chromium commit 33441a0f3f9a591693157f2fd16852ce072e6f9d the logic of
  // SyncUserSettingsImpl::GetSelectedTypes had been changed and affected this
  // test.
  brave_sync_service_impl()->GetUserSettings()->SetSelectedTypes(
      false, syncer::UserSelectableTypeSet());

  brave_sync_service_impl()->OnEngineInitialized(true, false);
  EXPECT_FALSE(
      brave_sync_service_impl()->GetUserSettings()->IsPassphraseRequired());

  OSCryptMocker::TearDown();
}

TEST_F(BraveSyncServiceImplTest, OnSelfDeviceInfoDeleted) {
  OSCryptMocker::SetUp();
  CreateSyncService();

  brave_sync_service_impl()->Initialize();
  EXPECT_FALSE(engine());

  brave_sync_service_impl()->SetSyncCode(kValidSyncCode);
  task_environment_.RunUntilIdle();

  // Replace DataTypeManager with mock who gives CONFIGURING result on state()
  // call. We need this to force SyncService::GetTransportState() give
  // TransportState::CONFIGURING status to test behavior
  // BraveSyncServiceImpl::OnSelfDeviceInfoDeleted
  NiceMock<DataTypeManagerMock> data_type_manager_mock;
  std::unique_ptr<DataTypeManager> data_type_manager_mock_ptr =
      std::unique_ptr<DataTypeManager>(&data_type_manager_mock);

  brave_sync_service_impl()->data_type_manager_ =
      std::move(data_type_manager_mock_ptr);

  ON_CALL(data_type_manager_mock, state())
      .WillByDefault(Return(DataTypeManager::CONFIGURING));

  EXPECT_EQ(brave_sync_service_impl()->GetTransportState(),
            SyncServiceImpl::TransportState::CONFIGURING);

  NiceMock<SyncServiceObserverMock> observer_mock;
  brave_sync_service_impl()->AddObserver(&observer_mock);

  // When OnSelfDeviceInfoDeleted arrived, but transport state is CONFIGURING,
  // we must not stop and clear service.
  EXPECT_CALL(observer_mock, OnStateChanged).Times(0);
  brave_sync_service_impl()->OnSelfDeviceInfoDeleted(base::DoNothing());

  brave_sync_service_impl()->RemoveObserver(&observer_mock);

  // brave_sync_service_impl()->data_type_manager_ is owned by local var
  // |data_type_manager_mock|, so release ownership for the correct destruction
  brave_sync_service_impl()->data_type_manager_.release();

  OSCryptMocker::TearDown();
}

TEST_F(BraveSyncServiceImplTest, PermanentlyDeleteAccount) {
  OSCryptMocker::SetUp();
  CreateSyncService();

  brave_sync_service_impl()->Initialize();
  EXPECT_FALSE(engine());
  brave_sync_service_impl()->SetSyncCode(kValidSyncCode);
  task_environment_.RunUntilIdle();

  brave_sync_service_impl()
      ->GetUserSettings()
      ->SetInitialSyncFeatureSetupComplete(
          syncer::SyncFirstSetupCompleteSource::ADVANCED_FLOW_CONFIRM);
  EXPECT_TRUE(engine());

  std::unique_ptr<testing::NiceMock<BraveMockSyncEngine>> mock_sync_engine =
      std::make_unique<testing::NiceMock<BraveMockSyncEngine>>();
  EXPECT_CALL(*mock_sync_engine, PermanentlyDeleteAccount).Times(1);
  std::unique_ptr<SyncEngine> engine_orig =
      std::move(brave_sync_service_impl()->engine_);
  brave_sync_service_impl()->engine_ = std::move(mock_sync_engine);
  brave_sync_service_impl()->PermanentlyDeleteAccount(base::BindOnce(
      [](const syncer::SyncProtocolError& sync_protocol_error) {}));
  brave_sync_service_impl()->engine_ = std::move(engine_orig);
  OSCryptMocker::TearDown();
}

TEST_F(BraveSyncServiceImplTest, OnAccountDeleted_Success) {
  OSCryptMocker::SetUp();
  CreateSyncService();

  brave_sync_service_impl()->Initialize();
  EXPECT_FALSE(engine());

  brave_sync_service_impl()->initiated_delete_account_ = true;
  SyncProtocolError sync_protocol_error;
  sync_protocol_error.error_type = SYNC_SUCCESS;

  brave_sync_service_impl()->OnAccountDeleted(
      1, base::BindOnce([](const syncer::SyncProtocolError& spe) {
        EXPECT_EQ(spe.error_type, SYNC_SUCCESS);
      }),
      sync_protocol_error);

  OSCryptMocker::TearDown();
}

TEST_F(BraveSyncServiceImplTest, OnAccountDeleted_FailureAndRetry) {
  OSCryptMocker::SetUp();
  CreateSyncService();

  brave_sync_service_impl()->Initialize();
  EXPECT_FALSE(engine());

  brave_sync_service_impl()->initiated_delete_account_ = true;
  SyncProtocolError sync_protocol_error;
  sync_protocol_error.error_type = TRANSIENT_ERROR;

  // Five unsuccessful attempts, the callback must be fired once, for the last
  // one
  bool was_callback_invoked[] = {false, false, false, false, true};

  for (size_t i = 0; i < std::size(was_callback_invoked); ++i) {
    bool on_account_deleted_invoked = false;
    brave_sync_service_impl()->OnAccountDeleted(
        i + 1,
        base::BindOnce(
            [](bool* on_account_deleted_invoked,
               const syncer::SyncProtocolError& spe) {
              *on_account_deleted_invoked = true;
              EXPECT_EQ(spe.error_type, TRANSIENT_ERROR);
            },
            &on_account_deleted_invoked),
        sync_protocol_error);

    EXPECT_EQ(on_account_deleted_invoked, was_callback_invoked[i]);
  }

  OSCryptMocker::TearDown();
}

TEST_F(BraveSyncServiceImplTest, JoinActiveOrNewChain) {
  OSCryptMocker::SetUp();
  CreateSyncService();

  brave_sync_service_impl()->Initialize();
  EXPECT_FALSE(engine());

  EXPECT_FALSE(brave_sync_service_impl()->join_chain_result_callback_);

  bool join_chain_callback_invoked = false;

  brave_sync_service_impl()->SetJoinChainResultCallback(base::BindOnce(
      [](bool* join_chain_callback_invoked, bool join_succeeded) {
        *join_chain_callback_invoked = true;
        EXPECT_TRUE(join_succeeded);
      },
      &join_chain_callback_invoked));

  EXPECT_TRUE(brave_sync_service_impl()->join_chain_result_callback_);
  EXPECT_FALSE(join_chain_callback_invoked);
  brave_sync_service_impl()->LocalDeviceAppeared();
  EXPECT_TRUE(join_chain_callback_invoked);

  OSCryptMocker::TearDown();
}

TEST_F(BraveSyncServiceImplTest, JoinDeletedChain) {
  OSCryptMocker::SetUp();
  CreateSyncService();

  brave_sync_service_impl()->Initialize();
  EXPECT_FALSE(engine());

  EXPECT_FALSE(brave_sync_service_impl()->join_chain_result_callback_);

  bool join_chain_callback_invoked = false;

  brave_sync_service_impl()->SetJoinChainResultCallback(base::BindOnce(
      [](bool* join_chain_callback_invoked, bool join_succeeded) {
        *join_chain_callback_invoked = true;
        EXPECT_FALSE(join_succeeded);
      },
      &join_chain_callback_invoked));

  EXPECT_TRUE(brave_sync_service_impl()->join_chain_result_callback_);
  EXPECT_FALSE(join_chain_callback_invoked);
  EXPECT_FALSE(brave_sync_service_impl()->initiated_self_device_info_deleted_);

  EXPECT_FALSE(brave_sync_service_impl()->initiated_join_chain_);
  brave_sync_service_impl()->SetSyncCode(kValidSyncCode);
  EXPECT_TRUE(brave_sync_service_impl()->initiated_join_chain_);

  // Normally sync_disabled_by_admin_ is set at
  // SyncServiceImpl::OnActionableError, but we can't invoke it, so set it
  // directly for test
  brave_sync_service_impl()->sync_disabled_by_admin_ = true;
  brave_sync_service_impl()->ResetEngine(
      ShutdownReason::DISABLE_SYNC_AND_CLEAR_DATA,
      SyncServiceImpl::ResetEngineReason::kDisabledAccount);

  EXPECT_TRUE(join_chain_callback_invoked);

  OSCryptMocker::TearDown();
}

TEST_F(BraveSyncServiceImplTest, HistoryPreconditions) {
  // This test ensures that BraveHistoryModelTypeController and
  // BraveHistoryDeleteDirectivesModelTypeController allow to run if
  // IsEncryptEverythingEnabled is set to true; upstream doesn't allow
  // History sync when encrypt everything is set to true.
  // The test is placed here alongside BraveSyncServiceImplTest because
  // here is the infrastructure which allows implement it.
  // Otherwise TestSyncUserSettings would not allow to override
  // IsEncryptEverythingEnabled.

  OSCryptMocker::SetUp();
  CreateSyncService();

  brave_sync_service_impl()->Initialize();
  EXPECT_FALSE(engine());
  brave_sync_service_impl()->SetSyncCode(kValidSyncCode);
  task_environment_.RunUntilIdle();

  brave_sync_service_impl()
      ->GetUserSettings()
      ->SetInitialSyncFeatureSetupComplete(
          syncer::SyncFirstSetupCompleteSource::ADVANCED_FLOW_CONFIRM);
  EXPECT_TRUE(engine());

  // Code below turns on encrypt everything
  brave_sync_service_impl()->GetCryptoForTests()->OnEncryptedTypesChanged(
      AlwaysEncryptedUserTypes(), true);

  // Ensure encrypt everything was actually enabled
  EXPECT_TRUE(brave_sync_service_impl()
                  ->GetUserSettings()
                  ->IsEncryptEverythingEnabled());

  auto history_model_type_controller =
      std::make_unique<history::BraveHistoryModelTypeController>(
          brave_sync_service_impl(), identity_manager(), nullptr,
          pref_service());

  auto history_precondition_state =
      history_model_type_controller->GetPreconditionState();
  EXPECT_EQ(history_precondition_state,
            DataTypeController::PreconditionState::kPreconditionsMet);

  auto test_model_type_store_service =
      std::make_unique<TestModelTypeStoreService>();
  auto history_delete_directives_model_type_controller = std::make_unique<
      history::BraveHistoryDeleteDirectivesModelTypeController>(
      base::DoNothing(), brave_sync_service_impl(),
      test_model_type_store_service.get(), nullptr, pref_service());

  auto history_delete_directives_precondition_state =
      history_delete_directives_model_type_controller->GetPreconditionState();
  EXPECT_EQ(history_delete_directives_precondition_state,
            DataTypeController::PreconditionState::kPreconditionsMet);

  OSCryptMocker::TearDown();
}

TEST_F(BraveSyncServiceImplTest, OnlyBookmarksAfterSetup) {
  OSCryptMocker::SetUp();
  CreateSyncService();

  brave_sync_service_impl()->Initialize();
  EXPECT_FALSE(engine());
  brave_sync_service_impl()->SetSyncCode(kValidSyncCode);
  task_environment_.RunUntilIdle();

  brave_sync_service_impl()
      ->GetUserSettings()
      ->SetInitialSyncFeatureSetupComplete(
          syncer::SyncFirstSetupCompleteSource::ADVANCED_FLOW_CONFIRM);
  EXPECT_TRUE(engine());

  EXPECT_FALSE(
      brave_sync_service_impl()->GetUserSettings()->IsSyncEverythingEnabled());
  auto selected_types =
      brave_sync_service_impl()->GetUserSettings()->GetSelectedTypes();
  EXPECT_EQ(selected_types.Size(), 1u);
  EXPECT_TRUE(selected_types.Has(UserSelectableType::kBookmarks));

  OSCryptMocker::TearDown();
}

}  // namespace syncer
