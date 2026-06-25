/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/sync/ai_chat_data_type_controller.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/files/file_path.h"
#include "base/files/scoped_temp_dir.h"
#include "base/functional/callback.h"
#include "base/functional/callback_helpers.h"
#include "base/memory/scoped_refptr.h"
#include "base/test/bind.h"
#include "base/test/metrics/histogram_tester.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/ai_chat_credential_manager.h"
#include "brave/components/ai_chat/core/browser/ai_chat_database.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/browser/model_service.h"
#include "brave/components/ai_chat/core/browser/tab_tracker_service.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "components/os_crypt/async/browser/os_crypt_async.h"
#include "components/os_crypt/async/browser/test_utils.h"
#include "components/os_crypt/async/common/encryptor.h"
#include "components/sync/base/data_type.h"
#include "components/sync/base/sync_stop_metadata_fate.h"
#include "components/sync/protocol/data_type_state.pb.h"
#include "components/sync/service/data_type_controller.h"
#include "components/sync/test/test_sync_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/network/public/cpp/network_context_getter.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

namespace {

using PreconditionState = syncer::DataTypeController::PreconditionState;

// AIChatService's database file, in its profile directory.
constexpr base::FilePath::CharType kDatabaseFileName[] =
    FILE_PATH_LITERAL("AIChat");

class MockAIChatCredentialManager : public AIChatCredentialManager {
 public:
  using AIChatCredentialManager::AIChatCredentialManager;

  MOCK_METHOD(void,
              GetPremiumStatus,
              (mojom::Service::GetPremiumStatusCallback callback),
              (override));
};

// Counts DataTypePreconditionChanged() calls for AI_CHAT_CONVERSATION, which
// is how the controller asks sync to check its precondition again. Once given
// a way to read the precondition, it also reads it on each call, as sync does.
class PreconditionRecordingSyncService : public syncer::TestSyncService {
 public:
  void DataTypePreconditionChanged(syncer::DataType type) override {
    if (type != syncer::AI_CHAT_CONVERSATION) {
      return;
    }
    ++precondition_changed_count_;
    if (read_precondition_) {
      read_preconditions_.push_back(read_precondition_.Run());
    }
  }

  void ReadPreconditionOnChange(
      base::RepeatingCallback<PreconditionState()> read_precondition) {
    read_precondition_ = std::move(read_precondition);
  }

  int precondition_changed_count() const { return precondition_changed_count_; }

  // The preconditions read when DataTypePreconditionChanged() was called.
  const std::vector<PreconditionState>& read_preconditions() const {
    return read_preconditions_;
  }

 private:
  int precondition_changed_count_ = 0;
  base::RepeatingCallback<PreconditionState()> read_precondition_;
  std::vector<PreconditionState> read_preconditions_;
};

}  // namespace

class AIChatDataTypeControllerTest : public testing::Test {
 public:
  explicit AIChatDataTypeControllerTest(bool os_crypt_is_sync = false)
      : os_crypt_is_sync_(os_crypt_is_sync) {
    features_.InitWithFeatures(
        {features::kAIChatHistory, features::kBraveSyncAIChat}, {});
  }

  void SetUp() override {
    ASSERT_TRUE(temp_directory_.CreateUniqueTempDir());
    prefs::RegisterProfilePrefs(prefs_.registry());
    prefs::RegisterLocalStatePrefs(local_state_.registry());
    ModelService::RegisterProfilePrefs(prefs_.registry());
    prefs_.SetBoolean(prefs::kBraveChatStorageEnabled, true);

    // Asynchronous by default, as at browser startup, so there is a window
    // between storage turning on and the database being attached to the sync
    // bridge.
    os_crypt_ = os_crypt_async::GetTestOSCryptAsyncForTesting(
        /*is_sync_for_unittests=*/os_crypt_is_sync_);
    shared_url_loader_factory_ =
        base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
            &url_loader_factory_);
    model_service_ = std::make_unique<ModelService>(
        &prefs_, os_crypt_.get(), network::NetworkContextGetter(),
        /*url_loader_factory=*/nullptr, base::FilePath());
    tab_tracker_service_ = std::make_unique<TabTrackerService>();
  }

 protected:
  void CreateService() {
    auto credential_manager =
        std::make_unique<testing::NiceMock<MockAIChatCredentialManager>>(
            base::NullCallback(), &local_state_);
    ON_CALL(*credential_manager, GetPremiumStatus(testing::_))
        .WillByDefault([](mojom::Service::GetPremiumStatusCallback callback) {
          std::move(callback).Run(mojom::PremiumStatus::Inactive,
                                  mojom::PremiumInfo::New());
        });
    ai_chat_service_ = std::make_unique<AIChatService>(
        model_service_.get(), tab_tracker_service_.get(),
        std::move(credential_manager), &prefs_, /*ai_chat_metrics=*/nullptr,
        os_crypt_.get(), shared_url_loader_factory_, /*channel_string=*/"",
        temp_directory_.GetPath());
  }

  // The controller gets the service from `ai_chat_service_` when it asks, as
  // SyncServiceFactory's getter gets it from AIChatServiceFactory.
  void CreateController() {
    controller_ = std::make_unique<AIChatDataTypeController>(
        &sync_service_, &prefs_,
        base::BindLambdaForTesting([this]() -> AIChatService* {
          ++ai_chat_service_lookups_;
          return ai_chat_service_.get();
        }));
  }

  PreconditionState GetPreconditionState() {
    // The controller doesn't read the context.
    return controller_->GetPreconditionState(
        syncer::DataTypeController::PreconditionContext(
            signin::AccountManagedStatusFinderOutcome::kConsumerGmail));
  }

  // Waits for the connect that the controller's constructor posts, which looks
  // up AIChatService.
  void WaitForPostedConnect() {
    ASSERT_TRUE(
        base::test::RunUntil([this] { return ai_chat_service_lookups_ > 0; }));
  }

  void WaitForSyncDatabaseReady() {
    ASSERT_TRUE(base::test::RunUntil(
        [this] { return ai_chat_service_->IsSyncDatabaseReady(); }));
  }

  // Waits until the sync processor has handled the controller's earlier calls.
  // This call reaches it behind them, through the same tasks, and replies.
  void WaitForSyncProcessor() {
    base::test::TestFuture<size_t> unsynced_count;
    controller_->GetUnsyncedDataCount(unsynced_count.GetCallback());
    ASSERT_TRUE(unsynced_count.Wait());
  }

  // Saves sync metadata saying AI Chat's first sync is done, in the database
  // file AIChatService opens, so its sync processor starts out tracking
  // metadata. Only then does clearing the metadata reach the processor's
  // "Sync.ClearMetadataWhileStopped" histogram.
  void SaveFirstSyncDoneMetadata() {
    base::test::TestFuture<scoped_refptr<os_crypt_async::Encryptor>> encryptor;
    os_crypt_->GetInstance(encryptor.GetCallback());
    AIChatDatabase database(temp_directory_.GetPath().Append(kDatabaseFileName),
                            encryptor.Take());
    sync_pb::DataTypeState state;
    state.set_initial_sync_state(sync_pb::DataTypeState::INITIAL_SYNC_DONE);
    ASSERT_TRUE(
        database.UpdateDataTypeState(syncer::AI_CHAT_CONVERSATION, state));
  }

  // Outlives `task_environment_`, whose shutdown runs the database
  // sequence's remaining tasks, which close the database file.
  base::ScopedTempDir temp_directory_;
  base::test::ScopedFeatureList features_;
  base::test::TaskEnvironment task_environment_;
  const bool os_crypt_is_sync_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;
  std::unique_ptr<os_crypt_async::OSCryptAsync> os_crypt_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  std::unique_ptr<ModelService> model_service_;
  std::unique_ptr<TabTrackerService> tab_tracker_service_;
  std::unique_ptr<AIChatService> ai_chat_service_;
  PreconditionRecordingSyncService sync_service_;
  std::unique_ptr<AIChatDataTypeController> controller_;
  int ai_chat_service_lookups_ = 0;
};

// SyncServiceFactory creates the controller while it builds the SyncService,
// and SyncServiceImpl::Initialize() then stops each unselected type, clearing
// its metadata, before that build finishes. Looking up AIChatService during
// the build would hand its dependencies no SyncService, so the controller must
// not look it up until a later task, and the clear must still reach the
// sync processor then.
TEST_F(AIChatDataTypeControllerTest,
       LooksUpAIChatServiceOnlyAfterConstructionTask) {
  ASSERT_NO_FATAL_FAILURE(SaveFirstSyncDoneMetadata());
  CreateService();
  ASSERT_NO_FATAL_FAILURE(WaitForSyncDatabaseReady());

  base::HistogramTester histograms;
  CreateController();
  controller_->Stop(syncer::CLEAR_METADATA, base::DoNothing());
  EXPECT_EQ(ai_chat_service_lookups_, 0);

  ASSERT_NO_FATAL_FAILURE(WaitForSyncProcessor());
  EXPECT_GT(ai_chat_service_lookups_, 0);
  histograms.ExpectUniqueSample(
      "Sync.ClearMetadataWhileStopped.ImmediateClear",
      syncer::DataTypeHistogramValue(syncer::AI_CHAT_CONVERSATION), 1);
}

// The type can't start until the database is attached to the sync bridge,
// which happens asynchronously after storage turns on. Turning storage off has
// sync check the precondition straight away; turning it on waits for the
// attach, so a first sync never merges into no database.
TEST_F(AIChatDataTypeControllerTest, PreconditionFollowsStorageAndDatabase) {
  // The controller exists before AIChatService, as at browser startup.
  CreateController();
  CreateService();
  EXPECT_EQ(GetPreconditionState(), PreconditionState::kMustStopAndKeepData);

  ASSERT_NO_FATAL_FAILURE(WaitForSyncDatabaseReady());
  EXPECT_EQ(GetPreconditionState(), PreconditionState::kPreconditionsMet);
  EXPECT_EQ(sync_service_.precondition_changed_count(), 1);

  prefs_.SetBoolean(prefs::kBraveChatStorageEnabled, false);
  EXPECT_EQ(GetPreconditionState(), PreconditionState::kMustStopAndClearData);
  EXPECT_EQ(sync_service_.precondition_changed_count(), 2);

  prefs_.SetBoolean(prefs::kBraveChatStorageEnabled, true);
  EXPECT_EQ(GetPreconditionState(), PreconditionState::kMustStopAndKeepData);
  EXPECT_EQ(sync_service_.precondition_changed_count(), 2);

  ASSERT_NO_FATAL_FAILURE(WaitForSyncDatabaseReady());
  EXPECT_EQ(GetPreconditionState(), PreconditionState::kPreconditionsMet);
  EXPECT_EQ(sync_service_.precondition_changed_count(), 3);
}

// Without the chat history feature AIChatService never opens a database, even
// though the storage pref defaults to on.
TEST_F(AIChatDataTypeControllerTest, ClearsDataWithoutChatHistoryFeature) {
  base::test::ScopedFeatureList history_off;
  history_off.InitAndDisableFeature(features::kAIChatHistory);
  CreateService();
  CreateController();

  EXPECT_EQ(GetPreconditionState(), PreconditionState::kMustStopAndClearData);
}

// AIChatServiceFactory returns null where AI Chat isn't available, such as
// when a policy disables it on desktop.
TEST_F(AIChatDataTypeControllerTest, KeepsDataWithoutAIChatService) {
  CreateController();

  EXPECT_EQ(GetPreconditionState(), PreconditionState::kMustStopAndKeepData);
}

// A policy can enable or disable AI Chat at any time, and the type stays
// registered either way, so each change has to have sync check the
// precondition again. A policy enabling AI Chat after startup is also when the
// controller can first connect to AIChatService, which it needs to hear when
// the database is attached.
TEST_F(AIChatDataTypeControllerTest, PreconditionFollowsAIChatPolicy) {
  prefs_.SetManagedPref(prefs::kEnabledByPolicy, base::Value(false));
  CreateController();
  // The connect finds no AIChatService, as on desktop while the policy
  // disables AI Chat.
  ASSERT_NO_FATAL_FAILURE(WaitForPostedConnect());
  EXPECT_EQ(GetPreconditionState(), PreconditionState::kMustStopAndKeepData);

  // AIChatServiceFactory builds AIChatService on the first lookup that the
  // policy allows, so its database isn't attached yet when the controller
  // connects.
  CreateService();
  prefs_.SetManagedPref(prefs::kEnabledByPolicy, base::Value(true));
  EXPECT_EQ(sync_service_.precondition_changed_count(), 1);
  EXPECT_EQ(GetPreconditionState(), PreconditionState::kMustStopAndKeepData);

  ASSERT_NO_FATAL_FAILURE(WaitForSyncDatabaseReady());
  EXPECT_EQ(sync_service_.precondition_changed_count(), 2);
  EXPECT_EQ(GetPreconditionState(), PreconditionState::kPreconditionsMet);

  // AIChatService and its database stay, as they also do on iOS whatever the
  // policy, but the policy still stops the type.
  prefs_.SetManagedPref(prefs::kEnabledByPolicy, base::Value(false));
  EXPECT_EQ(sync_service_.precondition_changed_count(), 3);
  EXPECT_EQ(GetPreconditionState(), PreconditionState::kMustStopAndKeepData);
}

// Sync stops the type and clears its metadata when the precondition reports
// kMustStopAndClearData, as it does once storage is off. The clear has to
// reach the sync processor, or turning storage on again wouldn't run a first
// sync and download the conversations into the new, empty database.
TEST_F(AIChatDataTypeControllerTest, StopAndClearReachesSyncProcessor) {
  ASSERT_NO_FATAL_FAILURE(SaveFirstSyncDoneMetadata());
  CreateController();
  CreateService();
  ASSERT_NO_FATAL_FAILURE(WaitForSyncDatabaseReady());
  ASSERT_EQ(GetPreconditionState(), PreconditionState::kPreconditionsMet);

  prefs_.SetBoolean(prefs::kBraveChatStorageEnabled, false);
  ASSERT_EQ(GetPreconditionState(), PreconditionState::kMustStopAndClearData);

  base::HistogramTester histograms;
  controller_->Stop(syncer::CLEAR_METADATA, base::DoNothing());
  ASSERT_NO_FATAL_FAILURE(WaitForSyncProcessor());
  histograms.ExpectUniqueSample(
      "Sync.ClearMetadataWhileStopped.ImmediateClear",
      syncer::DataTypeHistogramValue(syncer::AI_CHAT_CONVERSATION), 1);
}

// Once OSCryptAsync has initialized, as it has whenever storage is turned on
// after startup, AIChatService gets its encryptor synchronously.
class AIChatDataTypeControllerSyncOSCryptTest
    : public AIChatDataTypeControllerTest {
 public:
  AIChatDataTypeControllerSyncOSCryptTest()
      : AIChatDataTypeControllerTest(/*os_crypt_is_sync=*/true) {}
};

// With the encryptor available synchronously, turning storage on attaches the
// database and notifies the controller from inside the storage pref's
// observer. Sync reads the precondition as soon as the controller says it
// changed, so it must already be met then.
TEST_F(AIChatDataTypeControllerSyncOSCryptTest,
       PreconditionMetWhenDatabaseAttachesSynchronously) {
  CreateController();
  CreateService();
  ASSERT_NO_FATAL_FAILURE(WaitForPostedConnect());
  ASSERT_EQ(GetPreconditionState(), PreconditionState::kPreconditionsMet);

  sync_service_.ReadPreconditionOnChange(
      base::BindLambdaForTesting([this] { return GetPreconditionState(); }));
  prefs_.SetBoolean(prefs::kBraveChatStorageEnabled, false);
  prefs_.SetBoolean(prefs::kBraveChatStorageEnabled, true);

  EXPECT_THAT(sync_service_.read_preconditions(),
              testing::ElementsAre(PreconditionState::kMustStopAndClearData,
                                   PreconditionState::kPreconditionsMet));
}

}  // namespace ai_chat
