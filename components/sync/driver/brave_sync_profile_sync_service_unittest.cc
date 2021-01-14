/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/logging.h"
#include "base/test/task_environment.h"
#include "brave/components/sync/driver/brave_sync_profile_sync_service.h"
#include "brave/components/sync/driver/profile_sync_service_delegate.h"
#include "components/os_crypt/os_crypt_mocker.h"
#include "components/sync/driver/data_type_manager_mock.h"
#include "components/sync/driver/fake_data_type_controller.h"
#include "components/sync/driver/profile_sync_service_bundle.h"
#include "components/sync/driver/sync_api_component_factory_mock.h"
#include "components/sync/test/engine/fake_sync_engine.h"
#include "components/sync/test/engine/mock_sync_engine.h"
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

}  // namespace

class ProfileSyncServiceDelegateMock : public ProfileSyncServiceDelegate {
 public:
  ProfileSyncServiceDelegateMock() {}
  ~ProfileSyncServiceDelegateMock() override {}
  void SuspendDeviceObserverForOwnReset() override {}
  void ResumeDeviceObserver() override {}
};

class BraveProfileSyncServiceTest : public testing::Test {
 public:
  BraveProfileSyncServiceTest()
      : brave_sync_prefs_(profile_sync_service_bundle_.pref_service()),
        sync_prefs_(profile_sync_service_bundle_.pref_service()) {
    profile_sync_service_bundle_.identity_test_env()
        ->SetAutomaticIssueOfAccessTokens(true);
    brave_sync::Prefs::RegisterProfilePrefs(
        profile_sync_service_bundle_.pref_service()->registry());
  }

  ~BraveProfileSyncServiceTest() override { sync_service_->Shutdown(); }

  void CreateSyncService(
      ProfileSyncService::StartBehavior start_behavior,
      ModelTypeSet registered_types = ModelTypeSet(BOOKMARKS)) {
    DataTypeController::TypeVector controllers;
    for (ModelType type : registered_types) {
      controllers.push_back(std::make_unique<FakeDataTypeController>(type));
    }

    std::unique_ptr<SyncClientMock> sync_client =
        profile_sync_service_bundle_.CreateSyncClientMock();
    ON_CALL(*sync_client, CreateDataTypeControllers(_))
        .WillByDefault(Return(ByMove(std::move(controllers))));

    sync_service_ = std::make_unique<BraveProfileSyncService>(
        profile_sync_service_bundle_.CreateBasicInitParams(
            start_behavior, std::move(sync_client)),
        std::make_unique<ProfileSyncServiceDelegateMock>());
  }

  MockSyncEngine* SetUpMockSyncEngine() {
    auto sync_engine = std::make_unique<NiceMock<MockSyncEngine>>();
    MockSyncEngine* sync_engine_raw = sync_engine.get();
    ON_CALL(*component_factory(), CreateSyncEngine(_, _, _, _))
        .WillByDefault(Return(ByMove(std::move(sync_engine))));
    return sync_engine_raw;
  }

  FakeSyncEngine* SetUpFakeSyncEngine() {
    auto sync_engine = std::make_unique<FakeSyncEngine>();
    FakeSyncEngine* sync_engine_raw = sync_engine.get();
    ON_CALL(*component_factory(), CreateSyncEngine(_, _, _, _))
        .WillByDefault(Return(ByMove(std::move(sync_engine))));
    return sync_engine_raw;
  }

  brave_sync::Prefs* brave_sync_prefs() { return &brave_sync_prefs_; }

  SyncPrefs* sync_prefs() { return &sync_prefs_; }

  BraveProfileSyncService* brave_sync_service() { return sync_service_.get(); }

  SyncApiComponentFactoryMock* component_factory() {
    return profile_sync_service_bundle_.component_factory();
  }

 private:
  content::BrowserTaskEnvironment task_environment_;
  ProfileSyncServiceBundle profile_sync_service_bundle_;
  brave_sync::Prefs brave_sync_prefs_;
  SyncPrefs sync_prefs_;
  std::unique_ptr<BraveProfileSyncService> sync_service_;
};

TEST_F(BraveProfileSyncServiceTest, ValidPassphrase) {
  OSCryptMocker::SetUp();

  CreateSyncService(ProfileSyncService::MANUAL_START);
  SetUpFakeSyncEngine();

  brave_sync_service()->Initialize();

  bool set_code_result = brave_sync_service()->SetSyncCode(kValidSyncCode);
  EXPECT_TRUE(set_code_result);

  EXPECT_EQ(brave_sync_prefs()->GetSeed(), kValidSyncCode);

  OSCryptMocker::TearDown();
}

TEST_F(BraveProfileSyncServiceTest, InvalidPassphrase) {
  OSCryptMocker::SetUp();

  CreateSyncService(ProfileSyncService::MANUAL_START);
  SetUpFakeSyncEngine();

  brave_sync_service()->Initialize();

  bool set_code_result =
      brave_sync_service()->SetSyncCode("word one and then two");
  EXPECT_FALSE(set_code_result);

  EXPECT_EQ(brave_sync_prefs()->GetSeed(), "");

  OSCryptMocker::TearDown();
}

TEST_F(BraveProfileSyncServiceTest, ValidPassphraseLeadingTrailingWhitespace) {
  OSCryptMocker::SetUp();

  CreateSyncService(ProfileSyncService::MANUAL_START);
  SetUpFakeSyncEngine();

  brave_sync_service()->Initialize();

  std::string sync_code_extra_whitespace =
      std::string(" \t\n") + kValidSyncCode + std::string(" \t\n");
  bool set_code_result =
      brave_sync_service()->SetSyncCode(sync_code_extra_whitespace);
  EXPECT_TRUE(set_code_result);

  EXPECT_EQ(brave_sync_prefs()->GetSeed(), kValidSyncCode);

  OSCryptMocker::TearDown();
}

TEST_F(BraveProfileSyncServiceTest, NoIdentityManagerCalls) {
  OSCryptMocker::SetUp();

  CreateSyncService(ProfileSyncService::MANUAL_START);
  MockSyncEngine* sync_engine = SetUpMockSyncEngine();

  brave_sync_service()->Initialize();

  bool set_code_result = brave_sync_service()->SetSyncCode(kValidSyncCode);
  EXPECT_TRUE(set_code_result);

  EXPECT_EQ(brave_sync_prefs()->GetSeed(), kValidSyncCode);

  ON_CALL(*sync_engine, IsInitialized()).WillByDefault(Return(true));

  // We need to test that during `ProfileSyncService::OnEngineInitialized`
  // the stubbed call identity_manager_->GetAccountsInCookieJar() is invoked.
  // We can do it indirectly. The stubbed method returns result where
  // `accounts_in_cookie_jar_info.accounts_are_fresh` is set to `false`,
  // this makes following sequence of calls:
  //   `ProfileSyncService::OnAccountsInCookieUpdated`,
  //   `ProfileSyncService::OnAccountsInCookieUpdatedWithCallback`,
  //   `engine_->OnCookieJarChanged`
  // will not be invoked.
  // So the indirect way to ensure is to see there is no call of
  // `SyncEngine::OnCookieJarChanged`

  EXPECT_CALL(*sync_engine, OnCookieJarChanged(_, _, _)).Times(0);
  brave_sync_service()->OnEngineInitialized(
      ModelTypeSet(BOOKMARKS), WeakHandle<JsBackend>(),
      WeakHandle<DataTypeDebugInfoListener>(), "", "", true);

  OSCryptMocker::TearDown();
}

}  // namespace syncer
