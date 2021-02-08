/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>
#include <vector>

#include "base/logging.h"
#include "base/test/task_environment.h"
#include "brave/components/sync/driver/brave_sync_profile_sync_service.h"
#include "brave/components/sync/driver/profile_sync_service_delegate.h"
#include "components/os_crypt/os_crypt_mocker.h"
#include "components/sync/driver/data_type_manager_mock.h"
#include "components/sync/driver/fake_data_type_controller.h"
#include "components/sync/driver/profile_sync_service_bundle.h"
#include "components/sync/driver/sync_api_component_factory_mock.h"
#include "components/sync/driver/sync_user_settings_mock.h"
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

class MockSyncPrefObserver : public SyncPrefObserver {
 public:
  MOCK_METHOD(void, OnSyncManagedPrefChange, (bool), (override));
  MOCK_METHOD(void, OnFirstSetupCompletePrefChange, (bool), (override));
  MOCK_METHOD(void, OnSyncRequestedPrefChange, (bool), (override));
  MOCK_METHOD(void, OnPreferredDataTypesPrefChange, (), (override));
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

  EXPECT_CALL(*sync_engine, OnCookieJarChanged(_, _)).Times(0);
  brave_sync_service()->OnEngineInitialized(
      ModelTypeSet(BOOKMARKS), WeakHandle<JsBackend>(),
      WeakHandle<DataTypeDebugInfoListener>(), "", "", true);

  OSCryptMocker::TearDown();
}

namespace {

SyncCycleSnapshot MakeDefaultCycleSnapshot(const SyncerError& commit_result) {
  ModelNeutralState model_neutral_state;
  model_neutral_state.commit_result = commit_result;

  return SyncCycleSnapshot(
      /*birthday=*/"", /*bag_of_chips=*/"", model_neutral_state,
      ProgressMarkerMap(), /*is_silenced-*/ false,
      /*num_encryption_conflicts=*/0, /*num_hierarchy_conflicts=*/0,
      /*num_server_conflicts=*/0, /*notifications_enabled=*/false,
      /*num_entries=*/0, /*sync_start_time=*/base::Time::Now(),
      /*poll_finish_time=*/base::Time::Now(),
      /*num_entries_by_type=*/std::vector<int>(ModelType::NUM_ENTRIES, 0),
      /*num_to_delete_entries_by_type=*/
      std::vector<int>(ModelType::NUM_ENTRIES, 0),
      /*get_updates_origin=*/sync_pb::SyncEnums::UNKNOWN_ORIGIN,
      /*poll_interval=*/base::TimeDelta::FromMinutes(30),
      /*has_remaining_local_changes=*/false);
}

base::TimeDelta g_overridden_time_delta;
base::Time g_overridden_now;

std::unique_ptr<base::subtle::ScopedTimeClockOverrides> OverrideForTimeDelta(
    base::TimeDelta overridden_time_delta,
    const base::Time& now = base::subtle::TimeNowIgnoringOverride()) {
  g_overridden_time_delta = overridden_time_delta;
  g_overridden_now = now;
  return std::make_unique<base::subtle::ScopedTimeClockOverrides>(
      []() { return g_overridden_now + g_overridden_time_delta; }, nullptr,
      nullptr);
}

base::TimeDelta g_minimal_time_between_reenable;

std::unique_ptr<base::subtle::ScopedTimeClockOverrides>
AdvanceTimeToAllowReenable() {
  DCHECK(!g_minimal_time_between_reenable.is_zero());
  static base::TimeDelta override_total_delta;
  override_total_delta += g_minimal_time_between_reenable;
  return OverrideForTimeDelta(override_total_delta);
}

}  // namespace

TEST_F(BraveProfileSyncServiceTest, ReenableTypes) {
  g_minimal_time_between_reenable =
      BraveProfileSyncService::MinimalTimeBetweenReenableForTests();
  CreateSyncService(ProfileSyncService::MANUAL_START);
  brave_sync_service()->Initialize();

  testing::StrictMock<MockSyncPrefObserver> mock_sync_pref_observer;
  brave_sync_service()->sync_prefs_.AddSyncPrefObserver(
      &mock_sync_pref_observer);

  SyncCycleSnapshot snapshot_unset(
      MakeDefaultCycleSnapshot(SyncerError(SyncerError::UNSET)));

  SyncerError::Value err_codes[] = {SyncerError::SERVER_RETURN_CONFLICT,
                                    SyncerError::SERVER_RETURN_TRANSIENT_ERROR,
                                    SyncerError::UNSET};

  std::unique_ptr<base::subtle::ScopedTimeClockOverrides> time_override;

  for (const auto& err : err_codes) {
    {
      auto time_override = AdvanceTimeToAllowReenable();
      // Cleanup failures counter
      brave_sync_service()->OnSyncCycleCompleted(snapshot_unset);
    }

    SyncCycleSnapshot snapshot_maybe_error(
        MakeDefaultCycleSnapshot(SyncerError(err)));

    for (size_t i = 0;
         i <
         BraveProfileSyncService::GetNumberOfFailedCommitsToReenableForTests() -
             1;
         ++i) {
      auto time_override = AdvanceTimeToAllowReenable();
      brave_sync_service()->OnSyncCycleCompleted(snapshot_maybe_error);
    }

    // Expect re-enables types on some errors
    EXPECT_CALL(mock_sync_pref_observer, OnPreferredDataTypesPrefChange())
        .Times((err == SyncerError::UNSET) ? 0 : 2);

    auto time_override = AdvanceTimeToAllowReenable();
    brave_sync_service()->OnSyncCycleCompleted(snapshot_maybe_error);
  }

  brave_sync_service()->sync_prefs_.RemoveSyncPrefObserver(
      &mock_sync_pref_observer);
}

TEST_F(BraveProfileSyncServiceTest, ReenableTypesMaxPeriod) {
  CreateSyncService(ProfileSyncService::MANUAL_START);
  brave_sync_service()->Initialize();

  testing::StrictMock<MockSyncPrefObserver> mock_sync_pref_observer;
  brave_sync_service()->sync_prefs_.AddSyncPrefObserver(
      &mock_sync_pref_observer);

  SyncCycleSnapshot snapshot_unset(
      MakeDefaultCycleSnapshot(SyncerError(SyncerError::UNSET)));
  SyncCycleSnapshot snapshot_error(MakeDefaultCycleSnapshot(
      SyncerError(SyncerError::SERVER_RETURN_CONFLICT)));

  for (size_t i = 0;
       i <
       BraveProfileSyncService::GetNumberOfFailedCommitsToReenableForTests() -
           1;
       ++i) {
    brave_sync_service()->OnSyncCycleCompleted(snapshot_error);
  }

  // Expect re-enables types happened
  EXPECT_CALL(mock_sync_pref_observer, OnPreferredDataTypesPrefChange())
      .Times(2);
  brave_sync_service()->OnSyncCycleCompleted(snapshot_error);

  // Doing the same, expecting re-enable will not happened because the allowed
  // period not yet passed

  // Cleanup failures counter
  brave_sync_service()->OnSyncCycleCompleted(snapshot_unset);
  for (size_t i = 0;
       i <
       BraveProfileSyncService::GetNumberOfFailedCommitsToReenableForTests() -
           1;
       ++i) {
    brave_sync_service()->OnSyncCycleCompleted(snapshot_error);
  }

  // Expect re-enables types not happened
  EXPECT_CALL(mock_sync_pref_observer, OnPreferredDataTypesPrefChange())
      .Times(0);
  brave_sync_service()->OnSyncCycleCompleted(snapshot_error);

  brave_sync_service()->sync_prefs_.RemoveSyncPrefObserver(
      &mock_sync_pref_observer);
}

}  // namespace syncer
