/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/ai_chat_service.h"

#include <array>
#include <cstddef>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/files/scoped_temp_dir.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/functional/callback_forward.h"
#include "base/functional/callback_helpers.h"
#include "base/logging.h"
#include "base/memory/scoped_refptr.h"
#include "base/run_loop.h"
#include "base/scoped_observation.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool.h"
#include "base/test/bind.h"
#include "base/test/gmock_callback_support.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/test/values_test_util.h"
#include "base/time/time.h"
#include "brave/components/ai_chat/core/browser/ai_chat_credential_manager.h"
#include "brave/components/ai_chat/core/browser/associated_content_manager.h"
#include "brave/components/ai_chat/core/browser/constants.h"
#include "brave/components/ai_chat/core/browser/conversation_handler.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/browser/engine/mock_engine_consumer.h"
#include "brave/components/ai_chat/core/browser/mock_conversation_handler_observer.h"
#include "brave/components/ai_chat/core/browser/model_service.h"
#include "brave/components/ai_chat/core/browser/tab_tracker_service.h"
#include "brave/components/ai_chat/core/browser/test/mock_associated_content.h"
#include "brave/components/ai_chat/core/browser/test_utils.h"
#include "brave/components/ai_chat/core/browser/tools/memory_storage_tool.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "brave/components/ai_chat/core/common/prefs.h"
#include "components/os_crypt/async/browser/os_crypt_async.h"
#include "components/os_crypt/async/browser/test_utils.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "services/network/test/test_utils.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/gurl.h"

using ::testing::_;
using ::testing::Eq;
using ::testing::NiceMock;

namespace ai_chat {

namespace {

class MockAIChatCredentialManager : public AIChatCredentialManager {
 public:
  using AIChatCredentialManager::AIChatCredentialManager;
  MOCK_METHOD(void,
              GetPremiumStatus,
              (mojom::Service::GetPremiumStatusCallback callback),
              (override));
};

class MockServiceClient : public mojom::ServiceObserver {
 public:
  explicit MockServiceClient(AIChatService* service) {
    service->BindObserver(service_observer_receiver_.BindNewPipeAndPassRemote(),
                          base::DoNothing());
    service->Bind(service_remote_.BindNewPipeAndPassReceiver());
  }

  ~MockServiceClient() override = default;

  void Disconnect() {
    service_observer_receiver_.reset();
    service_remote_.reset();
  }

  mojo::Remote<mojom::Service>& service_remote() { return service_remote_; }

  MOCK_METHOD(void,
              OnConversationListChanged,
              (std::vector<mojom::ConversationPtr>),
              (override));

  MOCK_METHOD(void, OnStateChanged, (mojom::ServiceStatePtr), (override));

  MOCK_METHOD(void,
              OnSmartModesChanged,
              (std::vector<mojom::SmartModePtr>),
              (override));

 private:
  mojo::Receiver<mojom::ServiceObserver> service_observer_receiver_{this};
  mojo::Remote<mojom::Service> service_remote_;
};

class MockConversationHandlerClient : public mojom::ConversationUI {
 public:
  explicit MockConversationHandlerClient(ConversationHandler* driver) {
    driver->Bind(conversation_handler_remote_.BindNewPipeAndPassReceiver(),
                 conversation_ui_receiver_.BindNewPipeAndPassRemote());
  }

  ~MockConversationHandlerClient() override = default;

  void Disconnect() {
    conversation_handler_remote_.reset();
    conversation_ui_receiver_.reset();
  }

  MOCK_METHOD(void,
              OnConversationHistoryUpdate,
              (mojom::ConversationTurnPtr),
              (override));

  MOCK_METHOD(void, OnAPIRequestInProgress, (bool), (override));

  MOCK_METHOD(void, OnAPIResponseError, (mojom::APIError), (override));

  MOCK_METHOD(void,
              OnModelDataChanged,
              (const std::string& conversation_model_key,
               const std::string& default_model_key,
               std::vector<mojom::ModelPtr> all_models),
              (override));

  MOCK_METHOD(void,
              OnSuggestedQuestionsChanged,
              (const std::vector<std::string>&,
               mojom::SuggestionGenerationStatus),
              (override));

  MOCK_METHOD(void,
              OnAssociatedContentInfoChanged,
              (std::vector<mojom::AssociatedContentPtr>),
              (override));

  MOCK_METHOD(void, OnConversationDeleted, (), (override));

 private:
  mojo::Receiver<mojom::ConversationUI> conversation_ui_receiver_{this};
  mojo::Remote<mojom::ConversationHandler> conversation_handler_remote_;
};

class MockAIChatDatabase : public AIChatDatabase {
 public:
  MockAIChatDatabase()
      : AIChatDatabase(base::FilePath(),
                       os_crypt_async::GetTestEncryptorForTesting()) {}
  ~MockAIChatDatabase() override = default;

  MOCK_METHOD(bool,
              AddConversation,
              (mojom::ConversationPtr,
               std::vector<std::string>,
               mojom::ConversationTurnPtr),
              (override));

  MOCK_METHOD(bool,
              AddConversationEntry,
              (std::string_view,
               mojom::ConversationTurnPtr,
               std::optional<std::string>),
              (override));

  MOCK_METHOD(bool,
              AddOrUpdateAssociatedContent,
              (std::string_view,
               std::vector<mojom::AssociatedContentPtr>,
               std::vector<std::string>),
              (override));

  MOCK_METHOD(bool,
              UpdateConversationTitle,
              (std::string_view, std::string_view),
              (override));

  MOCK_METHOD(bool,
              UpdateConversationModelKey,
              (std::string_view, std::optional<std::string>),
              (override));

  MOCK_METHOD(bool,
              UpdateConversationTokenInfo,
              (std::string_view, uint64_t, uint64_t),
              (override));

  MOCK_METHOD(bool, DeleteConversationEntry, (std::string_view), (override));
  MOCK_METHOD(bool, DeleteConversation, (std::string_view), (override));
  MOCK_METHOD(bool, DeleteAllData, (), (override));
  MOCK_METHOD(bool,
              DeleteAssociatedWebContent,
              (std::optional<base::Time>, std::optional<base::Time>),
              (override));

  MOCK_METHOD(std::vector<mojom::ConversationPtr>,
              GetAllConversations,
              (),
              (override));

  MOCK_METHOD(mojom::ConversationArchivePtr,
              GetConversationData,
              (std::string_view),
              (override));
};

}  // namespace

class AIChatServiceUnitTest : public testing::Test,
                              public ::testing::WithParamInterface<bool> {
 public:
  AIChatServiceUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {
    scoped_feature_list_.InitWithFeatureState(features::kAIChatHistory,
                                              GetParam());
  }

  void SetUp() override {
    CHECK(temp_directory_.CreateUniqueTempDir());
    DVLOG(0) << "Temp directory: " << temp_directory_.GetPath().value();
    prefs::RegisterProfilePrefs(prefs_.registry());
    prefs::RegisterLocalStatePrefs(local_state_.registry());
    ModelService::RegisterProfilePrefs(prefs_.registry());

    os_crypt_ = os_crypt_async::GetTestOSCryptAsyncForTesting(
        /*is_sync_for_unittests=*/true);

    shared_url_loader_factory_ =
        base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
            &url_loader_factory_);

    model_service_ = std::make_unique<ModelService>(&prefs_);
    tab_tracker_service_ = std::make_unique<TabTrackerService>();

    CreateService();

    if (is_opted_in_) {
      EmulateUserOptedIn();
    } else {
      EmulateUserOptedOut();
    }
  }

  void TearDown() override {
    ai_chat_service_.reset();
    // Allow handles on the db to be released, otherwise for very quick
    // tests, we get crashes on temp_directory_.Delete().
    task_environment_.RunUntilIdle();
    CHECK(temp_directory_.Delete());
  }

  void CreateService() {
    std::unique_ptr<MockAIChatCredentialManager> credential_manager =
        std::make_unique<MockAIChatCredentialManager>(base::NullCallback(),
                                                      &local_state_);

    ON_CALL(*credential_manager, GetPremiumStatus(_))
        .WillByDefault([&](mojom::Service::GetPremiumStatusCallback callback) {
          mojom::PremiumInfoPtr premium_info = mojom::PremiumInfo::New();
          std::move(callback).Run(mojom::PremiumStatus::Active,
                                  std::move(premium_info));
        });

    ai_chat_service_ = std::make_unique<AIChatService>(
        model_service_.get(), tab_tracker_service_.get(),
        std::move(credential_manager), &prefs_, nullptr, os_crypt_.get(),
        shared_url_loader_factory_, "", temp_directory_.GetPath());

    client_ =
        std::make_unique<NiceMock<MockServiceClient>>(ai_chat_service_.get());
  }

  void ResetService() {
    ai_chat_service_.reset();
    task_environment_.RunUntilIdle();
    CreateService();
  }

  void ExpectConversationsSize(base::Location location, size_t size) {
    SCOPED_TRACE(testing::Message() << location.ToString());
    base::RunLoop run_loop;
    client_->service_remote()->GetConversations(base::BindLambdaForTesting(
        [&](std::vector<mojom::ConversationPtr> conversations) {
          EXPECT_EQ(conversations.size(), size);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  ConversationHandler* CreateConversation() {
    auto* conversation_handler = ai_chat_service_->CreateConversation();
    EXPECT_TRUE(conversation_handler);
    return conversation_handler;
  }

  std::unique_ptr<MockConversationHandlerClient> CreateConversationClient(
      ConversationHandler* conversation_handler) {
    base::RunLoop run_loop;
    MockConversationHandlerObserver observer{};
    observer.Observe(conversation_handler);
    EXPECT_CALL(observer, OnClientConnectionChanged(Eq(conversation_handler)))
        .WillOnce(
            [&run_loop](ConversationHandler* handler) { run_loop.Quit(); });
    std::unique_ptr<NiceMock<MockConversationHandlerClient>> client =
        std::make_unique<NiceMock<MockConversationHandlerClient>>(
            conversation_handler);
    run_loop.Run();
    return client;
  }

  void DisconnectConversationClient(MockConversationHandlerClient* client) {
    // Difficult to use a RunLoop here because disconnecting the client
    // can result in the ConversationHandler being deleted, which will
    // prevent OnClientConnectionChanged from being fired, depending on
    // the order of observers being notified.
    client->Disconnect();
    task_environment_.RunUntilIdle();
  }

  // Conversations are unloaded after a delay, so we advance the clock by that
  // delay and let the task environment run until idle to give the deletion
  // handlers a chance to run.
  void WaitForConversationUnload() {
    task_environment_.AdvanceClock(base::Seconds(5));
    task_environment_.RunUntilIdle();
  }

  bool IsAIChatHistoryEnabled() { return GetParam(); }

  void EmulateUserOptedIn() { ::ai_chat::SetUserOptedIn(&prefs_, true); }

  void EmulateUserOptedOut() { ::ai_chat::SetUserOptedIn(&prefs_, false); }

  void TestGetEngineForTabOrganization(const std::string& expected_model_name,
                                       mojom::PremiumStatus premium_status) {
    auto* cred_manager = static_cast<MockAIChatCredentialManager*>(
        ai_chat_service_->GetCredentialManagerForTesting());
    EXPECT_CALL(*cred_manager, GetPremiumStatus(_))
        .WillOnce([&](mojom::Service::GetPremiumStatusCallback callback) {
          mojom::PremiumInfoPtr premium_info = mojom::PremiumInfo::New();
          std::move(callback).Run(premium_status, std::move(premium_info));
        });
    ai_chat_service_->GetEngineForTabOrganization(base::DoNothing());
    EXPECT_EQ(
        ai_chat_service_->GetTabOrganizationEngineForTesting()->GetModelName(),
        expected_model_name);
    testing::Mock::VerifyAndClearExpectations(cred_manager);
  }

  void TestGetSuggestedTopics(
      base::expected<std::vector<std::string>, mojom::APIError> expected_result,
      const base::Location& location = FROM_HERE) {
    SCOPED_TRACE(location.ToString());
    base::RunLoop run_loop;
    ai_chat_service_->GetSuggestedTopics(
        {}, base::BindLambdaForTesting(
                [&](base::expected<std::vector<std::string>, mojom::APIError>
                        result) {
                  EXPECT_EQ(result, expected_result);
                  run_loop.Quit();
                }));
    run_loop.Run();
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<AIChatService> ai_chat_service_;
  std::unique_ptr<ModelService> model_service_;
  std::unique_ptr<TabTrackerService> tab_tracker_service_;
  std::unique_ptr<NiceMock<MockServiceClient>> client_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;
  std::unique_ptr<os_crypt_async::OSCryptAsync> os_crypt_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
  bool is_opted_in_ = true;

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
  base::ScopedTempDir temp_directory_;
};

INSTANTIATE_TEST_SUITE_P(
    ,
    AIChatServiceUnitTest,
    ::testing::Bool(),
    [](const testing::TestParamInfo<AIChatServiceUnitTest::ParamType>& info) {
      return absl::StrFormat("History%s", info.param ? "Enabled" : "Disabled");
    });

TEST_P(AIChatServiceUnitTest, ConversationLifecycle_NoMessages) {
  EXPECT_CALL(*client_, OnConversationListChanged(testing::SizeIs(1))).Times(2);
  EXPECT_CALL(*client_, OnConversationListChanged(testing::SizeIs(2))).Times(2);
  // 1 extra calls in OnLoadConversationsLazyData if history is enabled.
  EXPECT_CALL(*client_, OnConversationListChanged(testing::SizeIs(3)))
      .Times(IsAIChatHistoryEnabled() ? 2 : 1);
  EXPECT_CALL(*client_, OnConversationListChanged(testing::SizeIs(0))).Times(1);

  ConversationHandler* conversation_handler1 = CreateConversation();

  ConversationHandler* conversation_handler2 = CreateConversation();

  ConversationHandler* temporary_conversation = CreateConversation();
  temporary_conversation->SetTemporary(true);

  ExpectConversationsSize(FROM_HERE, 3);

  // Before connecting any clients to the conversations, none should be deleted
  EXPECT_EQ(ai_chat_service_->GetInMemoryConversationCountForTesting(), 3u);

  // Connect a client then disconnect
  auto client1 = CreateConversationClient(conversation_handler1);
  DisconnectConversationClient(client1.get());
  WaitForConversationUnload();
  // Only 1 should be deleted
  EXPECT_EQ(ai_chat_service_->GetInMemoryConversationCountForTesting(), 2u);

  // Connect a client then disconnect
  auto client2 = CreateConversationClient(conversation_handler2);
  DisconnectConversationClient(client2.get());
  WaitForConversationUnload();
  EXPECT_EQ(ai_chat_service_->GetInMemoryConversationCountForTesting(), 1u);

  // Connect a client then disconnect for temporary conversation
  auto temp_client = CreateConversationClient(temporary_conversation);
  DisconnectConversationClient(temp_client.get());
  WaitForConversationUnload();
  EXPECT_EQ(ai_chat_service_->GetInMemoryConversationCountForTesting(), 0u);

  testing::Mock::VerifyAndClearExpectations(client_.get());
  task_environment_.RunUntilIdle();
}

TEST_P(AIChatServiceUnitTest,
       ConversationLifecycle_ShouldNotUnloadInProgressConversations) {
  ConversationHandler* conversation = CreateConversation();

  // Store a weak pointer to the conversation, so we can check if its been
  // destroyed.
  auto weak_ptr = conversation->GetWeakPtr();

  // Set up the engine so we can submit a turn.
  conversation->SetEngineForTesting(std::make_unique<MockEngineConsumer>());
  auto* engine =
      static_cast<MockEngineConsumer*>(conversation->GetEngineForTesting());

  // Function to call to finish generating the response.
  base::OnceClosure resolve;
  EXPECT_CALL(*engine, GenerateAssistantResponse)
      .WillOnce(
          [&resolve](PageContentsMap page_contents,
                     const std::vector<mojom::ConversationTurnPtr>& history,
                     const std::string& selected_language,
                     bool is_temporary_chat,
                     const std::vector<base::WeakPtr<Tool>>& tools,
                     std::optional<std::string_view> preferred_tool_name,
                     mojom::ConversationCapability conversation_capability,
                     base::RepeatingCallback<void(
                         EngineConsumer::GenerationResultData)> callback,
                     base::OnceCallback<void(
                         base::expected<EngineConsumer::GenerationResultData,
                                        mojom::APIError>)> done_callback) {
            resolve = base::BindOnce(
                [](base::OnceCallback<void(
                       base::expected<EngineConsumer::GenerationResultData,
                                      mojom::APIError>)> done_callback) {
                  std::move(done_callback)
                      .Run(base::ok(EngineConsumer::GenerationResultData(
                          mojom::ConversationEntryEvent::NewCompletionEvent(
                              mojom::CompletionEvent::New("")),
                          std::nullopt /* model_key */)));
                },
                std::move(done_callback));
          });

  // Conversation should exist in memory.
  EXPECT_EQ(ai_chat_service_->GetInMemoryConversationCountForTesting(), 1u);

  conversation->SubmitHumanConversationEntry(
      ai_chat::mojom::ConversationTurn::New());
  EXPECT_TRUE(conversation->IsRequestInProgress());

  // Check nothing has a pending unload.
  WaitForConversationUnload();

  // Conversation should not be unloaded
  EXPECT_EQ(ai_chat_service_->GetInMemoryConversationCountForTesting(), 1u);

  // Weak pointer should still be valid
  EXPECT_TRUE(weak_ptr);

  // Let the engine complete the request
  std::move(resolve).Run();

  WaitForConversationUnload();

  // Conversation should be unloaded
  EXPECT_EQ(ai_chat_service_->GetInMemoryConversationCountForTesting(), 0u);

  // Weak pointer should be invalid
  EXPECT_FALSE(weak_ptr);
}

TEST_P(AIChatServiceUnitTest, ConversationLifecycle_WithMessages) {
  // Should have these combinations at some point
  EXPECT_CALL(*client_, OnConversationListChanged(testing::SizeIs(1)))
      .Times(testing::AtLeast(1));
  EXPECT_CALL(*client_, OnConversationListChanged(testing::SizeIs(2)))
      .Times(testing::AtLeast(1));
  EXPECT_CALL(*client_, OnConversationListChanged(testing::SizeIs(3)))
      .Times(testing::AtLeast(1));
  // 0 times if history is enabled because there are entries persisted.
  EXPECT_CALL(*client_, OnConversationListChanged(testing::SizeIs(0)))
      .Times(IsAIChatHistoryEnabled() ? 0 : 1);

  ConversationHandler* conversation_handler1 = CreateConversation();
  conversation_handler1->SetChatHistoryForTesting(CreateSampleChatHistory(1u));

  ConversationHandler* conversation_handler2 = CreateConversation();
  conversation_handler2->SetChatHistoryForTesting(CreateSampleChatHistory(1u));

  ConversationHandler* temporary_conversation = CreateConversation();
  temporary_conversation->SetTemporary(true);
  temporary_conversation->SetChatHistoryForTesting(CreateSampleChatHistory(1u));

  ExpectConversationsSize(FROM_HERE, 3u);

  // Make sure nothing is queued to unload.
  WaitForConversationUnload();

  // Before connecting any clients to the conversations, none should be deleted
  EXPECT_EQ(ai_chat_service_->GetInMemoryConversationCountForTesting(), 3u);

  // Connect a client then disconnect
  auto client1 = CreateConversationClient(conversation_handler1);
  auto client2 = CreateConversationClient(conversation_handler2);
  auto temp_client = CreateConversationClient(temporary_conversation);

  DisconnectConversationClient(client1.get());
  WaitForConversationUnload();

  // Only 1 should be deleted, whether we preserve history or not (is preserved
  // in the database).
  EXPECT_EQ(ai_chat_service_->GetInMemoryConversationCountForTesting(), 2u);

  ExpectConversationsSize(FROM_HERE, IsAIChatHistoryEnabled() ? 3u : 2u);

  // Connect a client then disconnect
  DisconnectConversationClient(client2.get());
  WaitForConversationUnload();
  EXPECT_EQ(ai_chat_service_->GetInMemoryConversationCountForTesting(), 1u);

  ExpectConversationsSize(FROM_HERE, IsAIChatHistoryEnabled() ? 3u : 1u);

  // Disconnect temporary conversation client
  DisconnectConversationClient(temp_client.get());
  WaitForConversationUnload();
  EXPECT_EQ(ai_chat_service_->GetInMemoryConversationCountForTesting(), 0u);

  ExpectConversationsSize(FROM_HERE, IsAIChatHistoryEnabled() ? 2u : 0u);

  testing::Mock::VerifyAndClearExpectations(client_.get());
  task_environment_.RunUntilIdle();
}

TEST_P(AIChatServiceUnitTest, ConversationLifecycle_WithContent) {
  NiceMock<MockAssociatedContent> associated_content{};
  associated_content.SetUrl(GURL("https://example.com"));
  associated_content.SetContentId(1);
  ConversationHandler* conversation_with_content_no_messages =
      ai_chat_service_->GetOrCreateConversationHandlerForContent(
          associated_content.content_id(), associated_content.GetWeakPtr());
  EXPECT_TRUE(conversation_with_content_no_messages);
  // Asking again for same content ID gets same conversation
  EXPECT_EQ(
      conversation_with_content_no_messages,
      ai_chat_service_->GetOrCreateConversationHandlerForContent(
          associated_content.content_id(), associated_content.GetWeakPtr()));
  ExpectConversationsSize(FROM_HERE, 1u);
  EXPECT_EQ(ai_chat_service_->GetInMemoryConversationCountForTesting(), 1u);
  // Disconnecting the client should unload the handler and delete the
  // conversation.
  auto client1 =
      CreateConversationClient(conversation_with_content_no_messages);
  DisconnectConversationClient(client1.get());
  WaitForConversationUnload();
  EXPECT_EQ(ai_chat_service_->GetInMemoryConversationCountForTesting(), 0u);
  ExpectConversationsSize(FROM_HERE, 0u);

  // Create a new conversation for same content, with messages this time
  ConversationHandler* conversation_with_content =
      ai_chat_service_->GetOrCreateConversationHandlerForContent(
          associated_content.content_id(), associated_content.GetWeakPtr());
  conversation_with_content->SetChatHistoryForTesting(
      CreateSampleChatHistory(1u));
  ExpectConversationsSize(FROM_HERE, 1u);
  EXPECT_EQ(ai_chat_service_->GetInMemoryConversationCountForTesting(), 1u);
  auto client2 = CreateConversationClient(conversation_with_content);
  DisconnectConversationClient(client2.get());
  WaitForConversationUnload();
  // Disconnecting all clients should keep the handler in memory until
  // the content is destroyed.
  EXPECT_EQ(ai_chat_service_->GetInMemoryConversationCountForTesting(), 1u);
  ExpectConversationsSize(FROM_HERE, 1u);

  // Create a temporary conversation with content
  NiceMock<MockAssociatedContent> associated_content2{};
  associated_content2.SetUrl(GURL("https://example2.com"));
  associated_content2.SetContentId(2);
  ConversationHandler* temporary_conversation_with_content =
      ai_chat_service_->GetOrCreateConversationHandlerForContent(
          associated_content2.content_id(), associated_content2.GetWeakPtr());
  temporary_conversation_with_content->SetTemporary(true);
  temporary_conversation_with_content->SetChatHistoryForTesting(
      CreateSampleChatHistory(1u));
  ExpectConversationsSize(FROM_HERE, 2u);
  EXPECT_EQ(ai_chat_service_->GetInMemoryConversationCountForTesting(), 2u);
  auto temp_client =
      CreateConversationClient(temporary_conversation_with_content);
  DisconnectConversationClient(temp_client.get());
  WaitForConversationUnload();
  // Handler would still be in memory until the content is destroyed unless
  // it is a temporary chat.
  // Conversation would be unloaded when there are no live associated content.
  EXPECT_EQ(ai_chat_service_->GetInMemoryConversationCountForTesting(), 1u);
  ExpectConversationsSize(FROM_HERE, 1u);

  // Reset the content to be empty.
  conversation_with_content->associated_content_manager()->ClearContent();

  WaitForConversationUnload();

  if (IsAIChatHistoryEnabled()) {
    EXPECT_EQ(ai_chat_service_->GetInMemoryConversationCountForTesting(), 0u);
    ExpectConversationsSize(FROM_HERE, 1u);
  } else {
    EXPECT_EQ(ai_chat_service_->GetInMemoryConversationCountForTesting(), 0u);
    ExpectConversationsSize(FROM_HERE, 0u);
  }
}

TEST_P(AIChatServiceUnitTest, ConversationLifecycle_IsNotDeletedImmediately) {
  ConversationHandler* conversation = CreateConversation();
  auto client = CreateConversationClient(conversation);
  DisconnectConversationClient(client.get());

  // Should not have been deleted yet
  ExpectConversationsSize(FROM_HERE, 1u);

  WaitForConversationUnload();

  // Should have been deleted after the delay.
  ExpectConversationsSize(FROM_HERE, 0u);
}

TEST_P(AIChatServiceUnitTest, ConversationLifecycle_DeleteCanBeCancelled) {
  ConversationHandler* conversation = CreateConversation();
  auto client = CreateConversationClient(conversation);
  DisconnectConversationClient(client.get());

  // Should not have been deleted yet
  ExpectConversationsSize(FROM_HERE, 1u);

  // Reconnect a client.
  client = CreateConversationClient(conversation);

  WaitForConversationUnload();

  // Should not have been deleted after the delay as a client connected.
  ExpectConversationsSize(FROM_HERE, 1u);
}

TEST_P(AIChatServiceUnitTest, GetOrCreateConversationHandlerForContent) {
  ConversationHandler* conversation_without_content = CreateConversation();

  NiceMock<MockAssociatedContent> associated_content{};
  // Allowed scheme to be associated with a conversation
  associated_content.SetUrl(GURL("https://example.com"));
  associated_content.SetContentId(1);
  ConversationHandler* conversation_with_content =
      ai_chat_service_->GetOrCreateConversationHandlerForContent(
          associated_content.content_id(), associated_content.GetWeakPtr());
  EXPECT_TRUE(conversation_with_content);
  EXPECT_NE(conversation_without_content, conversation_with_content);
  EXPECT_NE(conversation_without_content->get_conversation_uuid(),
            conversation_with_content->get_conversation_uuid());
  base::RunLoop run_loop;
  conversation_with_content->GetAssociatedContentInfo(
      base::BindLambdaForTesting(
          [&](std::vector<mojom::AssociatedContentPtr> associated_content) {
            EXPECT_EQ(associated_content.size(), 1u);
            EXPECT_EQ(associated_content[0]->url, GURL("https://example.com"));
            run_loop.Quit();
          }));
  run_loop.Run();

  // Shouldn't create a conversation again when given the same content id
  EXPECT_EQ(
      ai_chat_service_->GetOrCreateConversationHandlerForContent(
          associated_content.content_id(), associated_content.GetWeakPtr()),
      conversation_with_content);

  // Creating a second conversation with the same associated content should
  // make the second conversation the default for that content, but leave
  // the first still associated with the content.
  ConversationHandler* conversation2 =
      ai_chat_service_->CreateConversationHandlerForContent(
          associated_content.content_id(), associated_content.GetWeakPtr());
  EXPECT_NE(conversation_with_content, conversation2);
  EXPECT_NE(conversation_with_content->get_conversation_uuid(),
            conversation2->get_conversation_uuid());

  EXPECT_EQ(conversation2->associated_content_manager()
                ->GetContentDelegatesForTesting()[0],
            &associated_content);
  ExpectAssociatedContentEquals(
      FROM_HERE,
      conversation2->associated_content_manager()->GetAssociatedContent(),
      conversation_with_content->associated_content_manager()
          ->GetAssociatedContent());

  // Check the second conversation is the default for that content ID
  EXPECT_EQ(
      ai_chat_service_->GetOrCreateConversationHandlerForContent(
          associated_content.content_id(), associated_content.GetWeakPtr()),
      conversation2);
  // Let the conversation be deleted
  std::string conversation2_uuid = conversation2->get_conversation_uuid();
  auto client1 = CreateConversationClient(conversation2);
  DisconnectConversationClient(client1.get());
  WaitForConversationUnload();

  ConversationHandler* conversation_with_content3 =
      ai_chat_service_->GetOrCreateConversationHandlerForContent(
          associated_content.content_id(), associated_content.GetWeakPtr());
  EXPECT_NE(conversation_with_content3->get_conversation_uuid(),
            conversation2_uuid);
}

TEST_P(AIChatServiceUnitTest,
       GetOrCreateConversationHandlerForContent_DisallowedScheme) {
  NiceMock<MockAssociatedContent> associated_content{};
  // Disallowed scheme to be associated with a conversation
  associated_content.SetUrl(GURL("chrome://example"));
  ConversationHandler* conversation_with_content =
      ai_chat_service_->GetOrCreateConversationHandlerForContent(
          associated_content.content_id(), associated_content.GetWeakPtr());
  EXPECT_TRUE(conversation_with_content);
  // Conversation will still be retrievable via associated content, but won't
  // be provided with the associated content.
  EXPECT_EQ(
      ai_chat_service_->GetOrCreateConversationHandlerForContent(
          associated_content.content_id(), associated_content.GetWeakPtr()),
      conversation_with_content);
  base::RunLoop run_loop;
  conversation_with_content->GetAssociatedContentInfo(
      base::BindLambdaForTesting(
          [&](std::vector<mojom::AssociatedContentPtr> associated_content) {
            EXPECT_TRUE(associated_content.empty());
            run_loop.Quit();
          }));
  run_loop.Run();
}

TEST_P(AIChatServiceUnitTest, GetConversation_AfterRestart) {
  auto history = CreateSampleChatHistory(1u);
  std::string uuid;
  {
    ConversationHandler* conversation_handler = CreateConversation();
    uuid = conversation_handler->get_conversation_uuid();
    auto client = CreateConversationClient(conversation_handler);
    conversation_handler->SetChatHistoryForTesting(CloneHistory(history));
    ExpectConversationsSize(FROM_HERE, 1);
    DisconnectConversationClient(client.get());
    WaitForConversationUnload();
  }
  ExpectConversationsSize(FROM_HERE, IsAIChatHistoryEnabled() ? 1 : 0);

  // Allow entries to finish being persisted before restarting service
  task_environment_.RunUntilIdle();
  DVLOG(0) << "Restarting service";
  ResetService();

  if (IsAIChatHistoryEnabled()) {
    EXPECT_CALL(*client_, OnConversationListChanged(testing::SizeIs(1)))
        .Times(testing::AtLeast(1));
  } else {
    EXPECT_CALL(*client_, OnConversationListChanged).Times(0);
  }
  // Can get conversation data
  if (IsAIChatHistoryEnabled()) {
    base::RunLoop run_loop;
    ai_chat_service_->GetConversation(
        uuid, base::BindLambdaForTesting(
                  [&](ConversationHandler* conversation_handler) {
                    EXPECT_TRUE(conversation_handler);
                    ExpectConversationHistoryEquals(
                        FROM_HERE,
                        conversation_handler->GetConversationHistory(),
                        history);
                    run_loop.Quit();
                  }));
    run_loop.Run();
  }
}

TEST_P(AIChatServiceUnitTest, MaybeInitStorage_DisableStoragePref) {
  // This test is only relevant when history feature is enabled initially
  if (!IsAIChatHistoryEnabled()) {
    return;
  }
  // Create history, verify it's persisted, then disable storage and verify
  // no history is returned, even in-memory (unless a client is connected).
  ConversationHandler* conversation_handler1 = CreateConversation();
  auto client1 = CreateConversationClient(conversation_handler1);
  conversation_handler1->SetChatHistoryForTesting(CreateSampleChatHistory(1u));

  ConversationHandler* conversation_handler2 = CreateConversation();
  auto client2 = CreateConversationClient(conversation_handler2);
  conversation_handler2->SetChatHistoryForTesting(CreateSampleChatHistory(1u));

  ConversationHandler* conversation_handler3 = CreateConversation();
  auto client3 = CreateConversationClient(conversation_handler3);
  conversation_handler3->SetTemporary(true);
  conversation_handler3->SetChatHistoryForTesting(CreateSampleChatHistory(1u));

  DisconnectConversationClient(client2.get());
  ExpectConversationsSize(FROM_HERE, 3);

  // Disable storage
  prefs_.SetBoolean(prefs::kBraveChatStorageEnabled, false);

  WaitForConversationUnload();

  // Conversation with no client was erased from memory
  ExpectConversationsSize(FROM_HERE, 2);

  // Disconnecting conversations should erase them fom memory
  DisconnectConversationClient(client1.get());
  DisconnectConversationClient(client3.get());

  WaitForConversationUnload();

  ExpectConversationsSize(FROM_HERE, 0);

  // Restart service and verify still doesn't load from storage
  ResetService();
  ExpectConversationsSize(FROM_HERE, 0);

  // Re-enable storage preference
  prefs_.SetBoolean(prefs::kBraveChatStorageEnabled, true);
  // Conversations are no longer in persistant storage
  ExpectConversationsSize(FROM_HERE, 0);
}

TEST_P(AIChatServiceUnitTest, OpenConversationWithStagedEntries_NoPermission) {
  NiceMock<MockAssociatedContent> associated_content{};
  ConversationHandler* conversation =
      ai_chat_service_->CreateConversationHandlerForContent(
          associated_content.content_id(), associated_content.GetWeakPtr());
  auto conversation_client = CreateConversationClient(conversation);

  ON_CALL(associated_content, HasOpenAIChatPermission)
      .WillByDefault(testing::Return(false));
  EXPECT_CALL(associated_content, GetStagedEntriesFromContent).Times(0);

  bool opened = false;
  ai_chat_service_->OpenConversationWithStagedEntries(
      associated_content.GetWeakPtr(),
      base::BindLambdaForTesting([&]() { opened = true; }));
  EXPECT_FALSE(opened);
  testing::Mock::VerifyAndClearExpectations(&associated_content);
}

TEST_P(AIChatServiceUnitTest, OpenConversationWithStagedEntries) {
  NiceMock<MockAssociatedContent> associated_content{};
  ON_CALL(associated_content, GetStagedEntriesFromContent)
      .WillByDefault([](GetStagedEntriesCallback callback) {
        std::move(callback).Run(std::vector<SearchQuerySummary>{
            SearchQuerySummary("query", "summary")});
      });
  ON_CALL(associated_content, HasOpenAIChatPermission)
      .WillByDefault(testing::Return(true));

  // Allowed scheme to be associated with a conversation
  associated_content.SetUrl(GURL("https://example.com"));

  ConversationHandler* conversation =
      ai_chat_service_->CreateConversationHandlerForContent(
          associated_content.content_id(), associated_content.GetWeakPtr());
  auto conversation_client = CreateConversationClient(conversation);

  EXPECT_CALL(associated_content, GetStagedEntriesFromContent)
      .Times(testing::AtLeast(1));

  bool opened = false;
  ai_chat_service_->OpenConversationWithStagedEntries(
      associated_content.GetWeakPtr(),
      base::BindLambdaForTesting([&]() { opened = true; }));

  base::RunLoop().RunUntilIdle();
  auto& history = conversation->GetConversationHistory();
  ASSERT_EQ(history.size(), 2u);
  EXPECT_EQ(history[0]->text, "query");
  EXPECT_EQ(history[1]->text, "summary");
  EXPECT_TRUE(opened);
  testing::Mock::VerifyAndClearExpectations(&associated_content);
}

TEST_P(AIChatServiceUnitTest, DeleteConversations) {
  // Create conversations, call DeleteConversations and verify all
  // conversations are deleted, whether a client is connected or not.
  ConversationHandler* conversation_handler1 = CreateConversation();
  auto client1 = CreateConversationClient(conversation_handler1);
  conversation_handler1->SetChatHistoryForTesting(CreateSampleChatHistory(1u));

  ConversationHandler* conversation_handler2 = CreateConversation();
  auto client2 = CreateConversationClient(conversation_handler2);
  conversation_handler2->SetChatHistoryForTesting(CreateSampleChatHistory(1u));

  ConversationHandler* conversation_handler3 = CreateConversation();
  auto client3 = CreateConversationClient(conversation_handler3);
  conversation_handler3->SetChatHistoryForTesting(CreateSampleChatHistory(1u));

  // Create a temporary conversation
  ConversationHandler* temporary_conversation = CreateConversation();
  temporary_conversation->SetTemporary(true);
  auto temp_client = CreateConversationClient(temporary_conversation);
  temporary_conversation->SetChatHistoryForTesting(CreateSampleChatHistory(1u));

  ExpectConversationsSize(FROM_HERE, 4);

  ai_chat_service_->DeleteConversations();

  ExpectConversationsSize(FROM_HERE, 0);

  // Verify deleted from database
  ResetService();
  ExpectConversationsSize(FROM_HERE, 0);
}

TEST_P(AIChatServiceUnitTest, DeleteConversations_TimeRange) {
  // Create conversations, call DeleteConversations and verify all
  // conversations are deleted, whether a client is connected or not.
  ConversationHandler* conversation_handler1 = CreateConversation();
  auto client1 = CreateConversationClient(conversation_handler1);
  // This conversation 3 hours in the past
  conversation_handler1->SetChatHistoryForTesting(
      CreateSampleChatHistory(1u, -3));

  ConversationHandler* conversation_handler2 = CreateConversation();
  auto client2 = CreateConversationClient(conversation_handler2);
  // This conversation 2 hours in the past
  conversation_handler2->SetChatHistoryForTesting(
      CreateSampleChatHistory(1u, -2));

  ConversationHandler* conversation_handler3 = CreateConversation();
  auto client3 = CreateConversationClient(conversation_handler3);
  // This conversation 1 hour in the past
  conversation_handler3->SetChatHistoryForTesting(
      CreateSampleChatHistory(1u, -1));

  // Create a temporary conversation 3 hours in the past
  ConversationHandler* temporary_conversation = CreateConversation();
  temporary_conversation->SetTemporary(true);
  auto temp_client = CreateConversationClient(temporary_conversation);
  temporary_conversation->SetChatHistoryForTesting(
      CreateSampleChatHistory(1u, -3));

  ExpectConversationsSize(FROM_HERE, 4);

  ai_chat_service_->DeleteConversations(base::Time::Now() - base::Minutes(245),
                                        base::Time::Now() - base::Minutes(110));

  // Should only keep conversation_handler3 (1 hour ago)
  ExpectConversationsSize(FROM_HERE, 1);

  // Verify deleted from database
  ResetService();
  ExpectConversationsSize(FROM_HERE, IsAIChatHistoryEnabled() ? 1 : 0);
}

TEST_P(
    AIChatServiceUnitTest,
    CreateConversationHandlerForContent_ShouldNotAssociate_WhenPageContextEnabledInitiallyDisabled) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(
      features::kPageContextEnabledInitially);

  NiceMock<MockAssociatedContent> associated_content;
  associated_content.SetUrl(GURL("https://example.com"));
  ConversationHandler* conversation =
      ai_chat_service_->CreateConversationHandlerForContent(
          associated_content.content_id(), associated_content.GetWeakPtr());
  EXPECT_FALSE(conversation->should_send_page_contents());

  // Conversation should still be associated with the content, even though its
  // not being sent.
  EXPECT_EQ(
      conversation,
      ai_chat_service_->GetOrCreateConversationHandlerForContent(
          associated_content.content_id(), associated_content.GetWeakPtr()));
}

TEST_P(
    AIChatServiceUnitTest,
    CreateConversationHandlerForContent_ShouldAssociate_WhenPageContextEnabledInitiallyEnabled) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeature(
      features::kPageContextEnabledInitially);

  NiceMock<MockAssociatedContent> associated_content;
  associated_content.SetUrl(GURL("https://example.com"));
  ConversationHandler* conversation =
      ai_chat_service_->CreateConversationHandlerForContent(
          associated_content.content_id(), associated_content.GetWeakPtr());
  EXPECT_TRUE(conversation->should_send_page_contents());

  // Conversation should be associated with the content
  EXPECT_EQ(
      conversation,
      ai_chat_service_->GetOrCreateConversationHandlerForContent(
          associated_content.content_id(), associated_content.GetWeakPtr()));
}

TEST_P(AIChatServiceUnitTest, MaybeAssociateContent) {
  NiceMock<MockAssociatedContent> associated_content;
  associated_content.SetUrl(GURL("https://example.com"));

  ConversationHandler* handler = CreateConversation();
  ai_chat_service_->MaybeAssociateContent(&associated_content,
                                          handler->get_conversation_uuid());

  EXPECT_TRUE(handler->associated_content_manager()->HasAssociatedContent());

  EXPECT_EQ(handler, ai_chat_service_->GetOrCreateConversationHandlerForContent(
                         associated_content.content_id(),
                         associated_content.GetWeakPtr()));
}

TEST_P(AIChatServiceUnitTest,
       MaybeAssociateContent_AlreadyAttachedToOtherConversation) {
  NiceMock<MockAssociatedContent> associated_content;
  associated_content.SetUrl(GURL("https://example.com"));

  ConversationHandler* handler1 = CreateConversation();
  ConversationHandler* handler2 = CreateConversation();
  auto client1 = CreateConversationClient(handler1);
  auto client2 = CreateConversationClient(handler2);

  ai_chat_service_->MaybeAssociateContent(&associated_content,
                                          handler1->get_conversation_uuid());

  EXPECT_TRUE(handler1->associated_content_manager()->HasAssociatedContent());

  EXPECT_EQ(
      handler1,
      ai_chat_service_->GetOrCreateConversationHandlerForContent(
          associated_content.content_id(), associated_content.GetWeakPtr()));

  ai_chat_service_->MaybeAssociateContent(&associated_content,
                                          handler2->get_conversation_uuid());

  EXPECT_TRUE(handler1->associated_content_manager()->HasAssociatedContent());
  EXPECT_TRUE(handler2->associated_content_manager()->HasAssociatedContent());

  EXPECT_EQ(
      handler2,
      ai_chat_service_->GetOrCreateConversationHandlerForContent(
          associated_content.content_id(), associated_content.GetWeakPtr()));
}

TEST_P(AIChatServiceUnitTest, MaybeAssociateContent_InvalidScheme) {
  NiceMock<MockAssociatedContent> associated_content;
  associated_content.SetUrl(GURL("chrome://example"));

  ConversationHandler* handler = CreateConversation();
  ai_chat_service_->MaybeAssociateContent(&associated_content,
                                          handler->get_conversation_uuid());

  EXPECT_FALSE(handler->associated_content_manager()->HasAssociatedContent());
  EXPECT_EQ(handler, ai_chat_service_->GetOrCreateConversationHandlerForContent(
                         associated_content.content_id(),
                         associated_content.GetWeakPtr()));
}

TEST_P(AIChatServiceUnitTest, DisassociateContent) {
  NiceMock<MockAssociatedContent> associated_content;
  associated_content.SetUrl(GURL("https://example.com"));

  ConversationHandler* handler = CreateConversation();
  auto client = CreateConversationClient(handler);
  ai_chat_service_->MaybeAssociateContent(&associated_content,
                                          handler->get_conversation_uuid());

  EXPECT_TRUE(handler->associated_content_manager()->HasAssociatedContent());
  EXPECT_EQ(handler, ai_chat_service_->GetOrCreateConversationHandlerForContent(
                         associated_content.content_id(),
                         associated_content.GetWeakPtr()));

  auto content = std::move(
      handler->associated_content_manager()->GetAssociatedContent()[0]);
  ai_chat_service_->DisassociateContent(content,
                                        handler->get_conversation_uuid());

  EXPECT_FALSE(handler->associated_content_manager()->HasAssociatedContent());
  EXPECT_NE(handler, ai_chat_service_->GetOrCreateConversationHandlerForContent(
                         associated_content.content_id(),
                         associated_content.GetWeakPtr()));
}

TEST_P(AIChatServiceUnitTest, DisassociateContent_NotAttached) {
  NiceMock<MockAssociatedContent> associated_content;
  associated_content.SetUrl(GURL("https://example.com"));

  ConversationHandler* handler = CreateConversation();
  auto client = CreateConversationClient(handler);

  EXPECT_FALSE(handler->associated_content_manager()->HasAssociatedContent());

  auto content = mojom::AssociatedContent::New();
  content->uuid = associated_content.uuid();
  ai_chat_service_->DisassociateContent(content,
                                        handler->get_conversation_uuid());

  EXPECT_FALSE(handler->associated_content_manager()->HasAssociatedContent());
  EXPECT_NE(handler, ai_chat_service_->GetOrCreateConversationHandlerForContent(
                         associated_content.content_id(),
                         associated_content.GetWeakPtr()));
}

TEST_P(AIChatServiceUnitTest, DisassociateContent_NotAttachedInvalidScheme) {
  NiceMock<MockAssociatedContent> associated_content;
  associated_content.SetUrl(GURL("chrome://example"));

  ConversationHandler* handler = CreateConversation();
  auto client = CreateConversationClient(handler);

  ai_chat_service_->MaybeAssociateContent(&associated_content,
                                          handler->get_conversation_uuid());

  EXPECT_FALSE(handler->associated_content_manager()->HasAssociatedContent());
  EXPECT_EQ(handler, ai_chat_service_->GetOrCreateConversationHandlerForContent(
                         associated_content.content_id(),
                         associated_content.GetWeakPtr()));

  auto content = mojom::AssociatedContent::New();
  content->uuid = associated_content.uuid();
  content->content_id = associated_content.content_id();
  ai_chat_service_->DisassociateContent(content,
                                        handler->get_conversation_uuid());

  EXPECT_FALSE(handler->associated_content_manager()->HasAssociatedContent());
  EXPECT_NE(handler, ai_chat_service_->GetOrCreateConversationHandlerForContent(
                         associated_content.content_id(),
                         associated_content.GetWeakPtr()));
}

TEST_P(AIChatServiceUnitTest, DisassociateContent_AttachedToOtherConversation) {
  NiceMock<MockAssociatedContent> associated_content;
  associated_content.SetUrl(GURL("https://example.com"));

  ConversationHandler* handler1 = CreateConversation();
  ConversationHandler* handler2 = CreateConversation();
  auto client1 = CreateConversationClient(handler1);
  auto client2 = CreateConversationClient(handler2);

  ai_chat_service_->MaybeAssociateContent(&associated_content,
                                          handler1->get_conversation_uuid());

  EXPECT_TRUE(handler1->associated_content_manager()->HasAssociatedContent());
  EXPECT_EQ(
      handler1,
      ai_chat_service_->GetOrCreateConversationHandlerForContent(
          associated_content.content_id(), associated_content.GetWeakPtr()));

  ai_chat_service_->MaybeAssociateContent(&associated_content,
                                          handler2->get_conversation_uuid());

  EXPECT_TRUE(handler2->associated_content_manager()->HasAssociatedContent());
  EXPECT_EQ(
      handler2,
      ai_chat_service_->GetOrCreateConversationHandlerForContent(
          associated_content.content_id(), associated_content.GetWeakPtr()));

  auto content = mojom::AssociatedContent::New();
  content->uuid = associated_content.uuid();
  ai_chat_service_->DisassociateContent(content,
                                        handler1->get_conversation_uuid());

  EXPECT_FALSE(handler1->associated_content_manager()->HasAssociatedContent());
  EXPECT_TRUE(handler2->associated_content_manager()->HasAssociatedContent());
  EXPECT_EQ(
      handler2,
      ai_chat_service_->GetOrCreateConversationHandlerForContent(
          associated_content.content_id(), associated_content.GetWeakPtr()));
}

TEST_P(AIChatServiceUnitTest, DeleteAssociatedWebContent) {
  // Only valid when history is enabled
  if (!IsAIChatHistoryEnabled()) {
    return;
  }

  const GURL content_url("https://example.com");
  const std::u16string page_title = u"page title";
  const std::string page_content = "page content";

  struct Data {
    NiceMock<MockAssociatedContent> associated_content;
    raw_ptr<ConversationHandler> conversation_handler;
    std::unique_ptr<MockConversationHandlerClient> client;
  };
  std::array<Data, 3> data;

  // First conversation and its content should stay alive and still report
  // actual content info even though it falls in the deletion time range.
  // Second conversation should have its content archived and should report
  // empty content info since it falls in the deletion time range.
  // Third conversation should have its content archived but should report
  // actual content info since it does not fall in the deletion time range.

  for (int i = 0; i < 3; i++) {
    data[i].associated_content.SetUrl(content_url);
    data[i].associated_content.SetTitle(page_title);
    data[i].associated_content.SetTextContent(page_content);
    data[i].associated_content.SetContentId(i);

    data[i].conversation_handler =
        ai_chat_service_->GetOrCreateConversationHandlerForContent(
            data[i].associated_content.content_id(),
            data[i].associated_content.GetWeakPtr());
    WaitForAssociatedContentFetch(
        data[i].conversation_handler->associated_content_manager());
    EXPECT_TRUE(data[i].conversation_handler);
    data[i].client = CreateConversationClient(data[i].conversation_handler);
    data[i].conversation_handler->SetChatHistoryForTesting(
        CreateSampleChatHistory(1u, -3 + i));

    // Verify associated are initially correct
    base::RunLoop run_loop;
    data[i].conversation_handler->GetAssociatedContentInfo(
        base::BindLambdaForTesting(
            [&](std::vector<mojom::AssociatedContentPtr> site_info) {
              SCOPED_TRACE(testing::Message() << "data index: " << i);
              ASSERT_FALSE(site_info.empty());
              EXPECT_EQ(site_info.size(), 1u);
              EXPECT_EQ(site_info[0]->url, content_url);
              EXPECT_EQ(site_info[0]->title, base::UTF16ToUTF8(page_title));
              run_loop.Quit();
            }));
    run_loop.Run();
  }

  // Archive content for conversations 2 and 3
  data[1].conversation_handler->associated_content_manager()->OnRequestArchive(
      &data[1].associated_content);
  data[2].conversation_handler->associated_content_manager()->OnRequestArchive(
      &data[2].associated_content);

  // Delete associated content from conversations between 1 hours ago and 3
  // hours ago.
  base::RunLoop deletion_run_loop;
  ai_chat_service_->DeleteAssociatedWebContent(
      base::Time::Now() - base::Minutes(182),
      base::Time::Now() - base::Minutes(70),
      base::BindLambdaForTesting([&](bool success) {
        EXPECT_TRUE(success);
        deletion_run_loop.Quit();
      }));
  deletion_run_loop.Run();

  ExpectConversationsSize(FROM_HERE, 3);

  task_environment_.RunUntilIdle();

  for (int i = 0; i < 3; i++) {
    base::RunLoop run_loop;
    data[i].conversation_handler->GetAssociatedContentInfo(
        base::BindLambdaForTesting(
            [&](std::vector<mojom::AssociatedContentPtr> site_info) {
              SCOPED_TRACE(testing::Message() << "data index: " << i);
              if (i == 1) {
                EXPECT_EQ(0u, site_info.size());
              } else {
                ASSERT_EQ(site_info.size(), 1u);
                EXPECT_EQ(site_info[0]->url, content_url);
                EXPECT_EQ(site_info[0]->title, base::UTF16ToUTF8(page_title));
              }
              run_loop.Quit();
            }));
    run_loop.Run();

    base::RunLoop run_loop_2;
    data[i].conversation_handler->GeneratePageContentInternal(
        base::BindLambdaForTesting([&]() {
          auto page_contents =
              data[i]
                  .conversation_handler->associated_content_manager()
                  ->GetCachedContents();
          if (i == 1) {
            EXPECT_TRUE(page_contents.empty()) << i << " content was not empty";
          } else {
            EXPECT_EQ(page_contents.size(), 1u);
            EXPECT_EQ(page_contents[0].get().content, page_content)
                << i << " content did not match";
          }
          run_loop_2.Quit();
        }));
    run_loop_2.Run();
  }
}

TEST_P(AIChatServiceUnitTest, GetEngineForTabOrganization) {
  TestGetEngineForTabOrganization(kClaudeHaikuModelName,
                                  mojom::PremiumStatus::Inactive);
  TestGetEngineForTabOrganization(kClaudeSonnetModelName,
                                  mojom::PremiumStatus::Active);
  TestGetEngineForTabOrganization(kClaudeHaikuModelName,
                                  mojom::PremiumStatus::Inactive);
}

TEST_P(AIChatServiceUnitTest, GetSuggestedTopics_CacheTopics) {
  ai_chat_service_->SetTabOrganizationEngineForTesting(
      std::make_unique<testing::NiceMock<ai_chat::MockEngineConsumer>>());
  auto* engine = static_cast<MockEngineConsumer*>(
      ai_chat_service_->GetTabOrganizationEngineForTesting());

  std::string model_name = kClaudeSonnetModelName;
  ON_CALL(*engine, GetModelName())
      .WillByDefault(testing::ReturnRef(model_name));

  std::vector<std::string> topics1{"topic1"};
  std::vector<std::string> topics2{"topic2"};
  EXPECT_CALL(*engine, GetSuggestedTopics(_, _))
      .WillOnce(base::test::RunOnceCallback<1>(topics1))
      .WillOnce(base::test::RunOnceCallback<1>(topics2));

  TestGetSuggestedTopics(topics1);
  TestGetSuggestedTopics(topics1);
  ai_chat_service_->TabDataChanged({});
  TestGetSuggestedTopics(topics2);
}

TEST_P(AIChatServiceUnitTest, TemporaryConversation_NoDatabaseInteraction) {
  // We create mock DB object regardless of whether history is enabled.
  // In real case, there's no DB object at all if history is disabled, this
  // test is irrelevant when there's no DB object at all.
  if (!IsAIChatHistoryEnabled()) {
    return;
  }

  // Create a mock database
  auto mock_ptr = std::make_unique<NiceMock<MockAIChatDatabase>>();
  auto* mock_db_ptr = mock_ptr.get();
  auto mock_db = base::SequenceBound<std::unique_ptr<AIChatDatabase>>(
      task_environment_.GetMainThreadTaskRunner(), std::move(mock_ptr));

  // Set up expectations - no database calls should be made
  EXPECT_CALL(*mock_db_ptr, AddConversation(_, _, _)).Times(0);
  EXPECT_CALL(*mock_db_ptr, AddConversationEntry(_, _, _)).Times(0);
  EXPECT_CALL(*mock_db_ptr, AddOrUpdateAssociatedContent(_, _, _)).Times(0);
  EXPECT_CALL(*mock_db_ptr, UpdateConversationTitle(_, _)).Times(0);
  EXPECT_CALL(*mock_db_ptr, UpdateConversationModelKey).Times(0);
  EXPECT_CALL(*mock_db_ptr, UpdateConversationTokenInfo(_, _, _)).Times(0);
  EXPECT_CALL(*mock_db_ptr, DeleteConversationEntry(_)).Times(0);
  EXPECT_CALL(*mock_db_ptr, DeleteConversation(_)).Times(0);

  // Replace the real database with our mock
  ai_chat_service_->SetDatabaseForTesting(std::move(mock_db));

  // Create a temporary conversation
  ConversationHandler* conversation = CreateConversation();
  auto client = CreateConversationClient(conversation);

  conversation->SetTemporary(true);
  auto uuid = conversation->get_conversation_uuid();

  // This would trigger OnConversationEntryAdded.
  conversation->SetChatHistoryForTesting(CreateSampleChatHistory(1u));

  // Test title change
  ai_chat_service_->OnConversationTitleChanged(uuid, "New Title");

  // Test token info change
  ai_chat_service_->OnConversationTokenInfoChanged(uuid, 100, 50);

  // Test removing a message
  ai_chat_service_->OnConversationEntryRemoved(conversation, "uuid");

  DisconnectConversationClient(client.get());

  // Verify no database calls were made
  testing::Mock::VerifyAndClearExpectations(mock_db_ptr);

  // Also do a simple sanity test with permanent conversation (test add only),
  // just for making sure our mock is working as expected. Permanent
  // conversation is already tested in other test cases.
  ConversationHandler* permanent_conversation = CreateConversation();
  auto client2 = CreateConversationClient(permanent_conversation);
  ASSERT_FALSE(permanent_conversation->GetIsTemporary());
  permanent_conversation->SetChatHistoryForTesting(CreateSampleChatHistory(1u));
  EXPECT_CALL(*mock_db_ptr, AddConversation(_, _, _)).Times(1);
  EXPECT_CALL(*mock_db_ptr, AddConversationEntry(_, _, _)).Times(1);
  EXPECT_CALL(*mock_db_ptr, UpdateConversationModelKey).Times(1);
  DisconnectConversationClient(client2.get());
  testing::Mock::VerifyAndClearExpectations(mock_db_ptr);
}

TEST_P(AIChatServiceUnitTest,
       OnConversationEntryAdded_GetsLatestAssociatedContent) {
  NiceMock<MockAssociatedContent> associated_content;
  associated_content.SetUrl(GURL("https://example.com"));

  ConversationHandler* handler = CreateConversation();
  auto client = CreateConversationClient(handler);

  // Don't notify listeners the content has been updated.
  handler->associated_content_manager()->AddContent(&associated_content,
                                                    /*notify_updated=*/false);

  // |associated_content| shouldn't have been updated on the metadata yet.
  EXPECT_EQ(handler->GetMetadataForTesting().associated_content.size(), 0u);

  handler->SubmitHumanConversationEntry("Human message", {});

  EXPECT_EQ(handler->GetMetadataForTesting().associated_content.size(), 1u);
}

TEST_P(AIChatServiceUnitTest, InitializeTools_MemoryDisabled) {
  // Test that no memory tool is created when memory is disabled
  prefs_.SetBoolean(prefs::kBraveAIChatUserMemoryEnabled, false);
  ResetService();

  EXPECT_FALSE(ai_chat_service_->GetMemoryToolForTesting());
}

TEST_P(AIChatServiceUnitTest, InitializeTools_MemoryEnabled) {
  // Test that memory tool is created when memory is enabled
  prefs_.SetBoolean(prefs::kBraveAIChatUserMemoryEnabled, true);
  ResetService();

  EXPECT_TRUE(ai_chat_service_->GetMemoryToolForTesting());
}

TEST_P(AIChatServiceUnitTest, OnMemoryEnabledChanged_EnabledToDisabled) {
  // Start with memory enabled
  prefs_.SetBoolean(prefs::kBraveAIChatUserMemoryEnabled, true);
  ResetService();

  // Verify memory tool exists
  EXPECT_TRUE(ai_chat_service_->GetMemoryToolForTesting());

  // Disable memory
  prefs_.SetBoolean(prefs::kBraveAIChatUserMemoryEnabled, false);

  // Verify memory tool is removed
  EXPECT_FALSE(ai_chat_service_->GetMemoryToolForTesting());
}

TEST_P(AIChatServiceUnitTest, OnMemoryEnabledChanged_DisabledToEnabled) {
  // Start with memory disabled
  prefs_.SetBoolean(prefs::kBraveAIChatUserMemoryEnabled, false);
  ResetService();

  // Verify no memory tool exists
  EXPECT_FALSE(ai_chat_service_->GetMemoryToolForTesting());

  // Enable memory
  prefs_.SetBoolean(prefs::kBraveAIChatUserMemoryEnabled, true);

  // Verify memory tool is added
  EXPECT_TRUE(ai_chat_service_->GetMemoryToolForTesting());
}

TEST_P(AIChatServiceUnitTest, GetSmartModes) {
  // Add a smart mode to preferences directly
  prefs::AddSmartModeToPrefs("test", "Test prompt", "model", prefs_);

  base::RunLoop run_loop;
  std::vector<mojom::SmartModePtr> result;

  ai_chat_service_->GetSmartModes(
      base::BindLambdaForTesting([&](std::vector<mojom::SmartModePtr> modes) {
        result = std::move(modes);
        run_loop.Quit();
      }));

  run_loop.Run();
  ASSERT_EQ(result.size(), 1u);
  EXPECT_EQ(result[0]->shortcut, "test");
  EXPECT_EQ(result[0]->prompt, "Test prompt");
  EXPECT_EQ(result[0]->model, "model");
}

TEST_P(AIChatServiceUnitTest, CreateSmartMode) {
  MockServiceClient mock_client(ai_chat_service_.get());
  base::RunLoop run_loop;
  EXPECT_CALL(mock_client, OnSmartModesChanged(_))
      .WillOnce([&](std::vector<mojom::SmartModePtr> smart_modes) {
        ASSERT_EQ(smart_modes.size(), 1u);
        EXPECT_EQ(smart_modes[0]->shortcut, "test_shortcut");
        EXPECT_EQ(smart_modes[0]->prompt, "Test prompt");
        EXPECT_EQ(smart_modes[0]->model, "test_model");
        run_loop.Quit();
      });

  ai_chat_service_->CreateSmartMode("test_shortcut", "Test prompt",
                                    "test_model");
  run_loop.Run();
}

TEST_P(AIChatServiceUnitTest, UpdateSmartMode) {
  // First create a smart mode
  prefs::AddSmartModeToPrefs("original", "Original prompt", "original_model",
                             prefs_);
  auto smart_modes = prefs::GetSmartModesFromPrefs(prefs_);
  ASSERT_EQ(smart_modes.size(), 1u);
  std::string id = smart_modes[0]->id;

  MockServiceClient mock_client(ai_chat_service_.get());
  base::RunLoop run_loop;
  EXPECT_CALL(mock_client, OnSmartModesChanged(_))
      .WillOnce([&](std::vector<mojom::SmartModePtr> smart_modes) {
        ASSERT_EQ(smart_modes.size(), 1u);
        EXPECT_EQ(smart_modes[0]->shortcut, "updated_shortcut");
        EXPECT_EQ(smart_modes[0]->prompt, "Updated prompt");
        EXPECT_EQ(smart_modes[0]->model, "updated_model");
        run_loop.Quit();
      });

  ai_chat_service_->UpdateSmartMode(id, "updated_shortcut", "Updated prompt",
                                    "updated_model");
  run_loop.Run();
}

TEST_P(AIChatServiceUnitTest, DeleteSmartMode) {
  prefs::AddSmartModeToPrefs("test", "Test prompt", "model", prefs_);
  auto smart_modes = prefs::GetSmartModesFromPrefs(prefs_);
  ASSERT_EQ(smart_modes.size(), 1u);
  std::string id = smart_modes[0]->id;

  MockServiceClient mock_client(ai_chat_service_.get());
  base::RunLoop run_loop;
  EXPECT_CALL(mock_client, OnSmartModesChanged(_))
      .WillOnce([&](std::vector<mojom::SmartModePtr> smart_modes) {
        EXPECT_TRUE(smart_modes.empty());
        run_loop.Quit();
      });

  ai_chat_service_->DeleteSmartMode(id);
  run_loop.Run();

  // Verify it was deleted
  auto mode = prefs::GetSmartModeFromPrefs(prefs_, id);
  EXPECT_FALSE(mode);

  auto all_smart_modes = prefs::GetSmartModesFromPrefs(prefs_);
  EXPECT_TRUE(all_smart_modes.empty());
}

}  // namespace ai_chat
