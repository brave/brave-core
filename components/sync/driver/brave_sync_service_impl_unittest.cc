/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/base64.h"
#include "base/logging.h"
#include "base/test/gtest_util.h"
#include "base/test/task_environment.h"
#include "brave/components/sync/driver/brave_sync_service_impl.h"
#include "brave/components/sync/driver/sync_service_impl_delegate.h"
#include "build/build_config.h"
#include "components/os_crypt/os_crypt.h"
#include "components/os_crypt/os_crypt_mocker.h"
#include "components/sync/driver/data_type_manager_impl.h"
#include "components/sync/engine/nigori/key_derivation_params.h"
#include "components/sync/engine/nigori/nigori.h"
#include "components/sync/test/fake_data_type_controller.h"
#include "components/sync/test/fake_sync_api_component_factory.h"
#include "components/sync/test/fake_sync_engine.h"
#include "components/sync/test/fake_sync_manager.h"
#include "components/sync/test/sync_service_impl_bundle.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;
using testing::ByMove;
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

  std::string nigori_name;
  EXPECT_TRUE(
      nigori->Permute(Nigori::Type::Password, kNigoriKeyName, &nigori_name));

  const std::string unencrypted = "test";
  sync_pb::EncryptedData encrypted;
  encrypted.set_key_name(nigori_name);
  EXPECT_TRUE(nigori->Encrypt(unencrypted, encrypted.mutable_blob()));
  return encrypted;
}

}  // namespace

class SyncServiceImplDelegateMock : public SyncServiceImplDelegate {
 public:
  SyncServiceImplDelegateMock() = default;
  ~SyncServiceImplDelegateMock() override = default;
  void SuspendDeviceObserverForOwnReset() override {}
  void ResumeDeviceObserver() override {}
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
      SyncServiceImpl::StartBehavior start_behavior,
      ModelTypeSet registered_types = ModelTypeSet(BOOKMARKS)) {
    DataTypeController::TypeVector controllers;
    for (ModelType type : registered_types) {
      controllers.push_back(std::make_unique<FakeDataTypeController>(type));
    }

    std::unique_ptr<SyncClientMock> sync_client =
        sync_service_impl_bundle_.CreateSyncClientMock();
    ON_CALL(*sync_client, CreateDataTypeControllers(_))
        .WillByDefault(Return(ByMove(std::move(controllers))));

    sync_service_impl_ = std::make_unique<BraveSyncServiceImpl>(
        sync_service_impl_bundle_.CreateBasicInitParams(start_behavior,
                                                        std::move(sync_client)),
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

  CreateSyncService(SyncServiceImpl::MANUAL_START);

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

  CreateSyncService(SyncServiceImpl::MANUAL_START);

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

  CreateSyncService(SyncServiceImpl::MANUAL_START);

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

// Google test doc strongly recommends to use ``*DeathTest` naming
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

  CreateSyncService(SyncServiceImpl::MANUAL_START);

  brave_sync_service_impl()->Initialize();
  EXPECT_FALSE(engine());

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

  CreateSyncService(SyncServiceImpl::MANUAL_START);

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
  CreateSyncService(SyncServiceImpl::MANUAL_START);

  brave_sync_service_impl()->Initialize();
  EXPECT_FALSE(engine());
  brave_sync_service_impl()->SetSyncCode(kValidSyncCode);
  task_environment_.RunUntilIdle();

  brave_sync_service_impl()->GetUserSettings()->SetFirstSetupComplete(
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

  brave_sync_service_impl()->OnEngineInitialized(true, false);
  EXPECT_FALSE(
      brave_sync_service_impl()->GetUserSettings()->IsPassphraseRequired());

  OSCryptMocker::TearDown();
}

}  // namespace syncer
