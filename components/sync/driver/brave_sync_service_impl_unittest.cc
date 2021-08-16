/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/logging.h"
#include "base/test/task_environment.h"
#include "brave/components/sync/driver/brave_sync_service_impl.h"
#include "brave/components/sync/driver/sync_service_impl_delegate.h"
#include "components/os_crypt/os_crypt_mocker.h"
#include "components/sync/driver/data_type_manager_impl.h"
#include "components/sync/driver/fake_data_type_controller.h"
#include "components/sync/driver/fake_sync_api_component_factory.h"
#include "components/sync/driver/sync_service_impl_bundle.h"
#include "components/sync/test/engine/fake_sync_engine.h"
#include "components/sync/test/engine/fake_sync_manager.h"
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

}  // namespace

class SyncServiceImplDelegateMock : public SyncServiceImplDelegate {
 public:
  SyncServiceImplDelegateMock() {}
  ~SyncServiceImplDelegateMock() override {}
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

  BraveSyncServiceImpl* brave_sync_service_impl() {
    return sync_service_impl_.get();
  }

  FakeSyncApiComponentFactory* component_factory() {
    return sync_service_impl_bundle_.component_factory();
  }

  FakeSyncEngine* engine() {
    return component_factory()->last_created_engine();
  }

 private:
  content::BrowserTaskEnvironment task_environment_;
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

  EXPECT_EQ(brave_sync_prefs()->GetSeed(), kValidSyncCode);

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

  EXPECT_EQ(brave_sync_prefs()->GetSeed(), "");

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

  EXPECT_EQ(brave_sync_prefs()->GetSeed(), kValidSyncCode);

  OSCryptMocker::TearDown();
}

}  // namespace syncer
