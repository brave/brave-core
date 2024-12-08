/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/ai_chat_service.h"

#include <cstddef>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/files/scoped_temp_dir.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/functional/callback_helpers.h"
#include "base/functional/overloaded.h"
#include "base/memory/scoped_refptr.h"
#include "base/run_loop.h"
#include "base/scoped_observation.h"
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
#include "brave/components/ai_chat/core/browser/conversation_handler.h"
#include "brave/components/ai_chat/core/browser/mock_conversation_handler_observer.h"
#include "brave/components/ai_chat/core/browser/test_utils.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
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


  MOCK_METHOD(void, OnConversationHistoryUpdate, (), (override));

  MOCK_METHOD(void, OnAPIRequestInProgress, (bool), (override));

  MOCK_METHOD(void, OnAPIResponseError, (mojom::APIError), (override));

  MOCK_METHOD(void,
              OnModelDataChanged,
              (const std::string&, std::vector<mojom::ModelPtr>),
              (override));

  MOCK_METHOD(void,
              OnSuggestedQuestionsChanged,
              (const std::vector<std::string>&,
               mojom::SuggestionGenerationStatus),
              (override));

  MOCK_METHOD(void,
              OnAssociatedContentInfoChanged,
              (const mojom::SiteInfoPtr, bool),
              (override));

  MOCK_METHOD(void, OnFaviconImageDataChanged, (), (override));

  MOCK_METHOD(void, OnConversationDeleted, (), (override));

 private:
  mojo::Receiver<mojom::ConversationUI> conversation_ui_receiver_{this};
  mojo::Remote<mojom::ConversationHandler> conversation_handler_remote_;
};

class MockAssociatedContent
    : public ConversationHandler::AssociatedContentDelegate {
 public:
  MockAssociatedContent() = default;
  ~MockAssociatedContent() override = default;

  int GetContentId() const override { return content_id_; }

  void SetContentId(int id) { content_id_ = id; }

  void GetContent(
      ConversationHandler::GetPageContentCallback callback) override {
    std::move(callback).Run(GetTextContent(), GetCachedIsVideo(), "");
  }

  std::string_view GetCachedTextContent() override {
    cached_text_content_ = GetTextContent();
    return cached_text_content_;
  }

  MOCK_METHOD(GURL, GetURL, (), (const, override));
  MOCK_METHOD(std::u16string, GetTitle, (), (const, override));
  MOCK_METHOD(bool, GetCachedIsVideo, (), (override));

  MOCK_METHOD(std::string, GetTextContent, (), ());

  MOCK_METHOD(void,
              GetStagedEntriesFromContent,
              (ConversationHandler::GetStagedEntriesCallback),
              (override));
  MOCK_METHOD(bool, HasOpenAIChatPermission, (), (const, override));

  void AddRelatedConversation(ConversationHandler* conversation) override {
    related_conversations_.insert(conversation);
  }

  void OnRelatedConversationDisassociated(
      ConversationHandler* conversation) override {
    related_conversations_.erase(conversation);
  }

  void DisassociateWithConversations(std::string archived_text_content,
                                     bool archived_is_video) {
    std::vector<base::WeakPtr<ConversationHandler>> related_conversations;
    for (auto& conversation : related_conversations_) {
      related_conversations.push_back(conversation->GetWeakPtr());
    }

    for (auto& conversation : related_conversations) {
      if (conversation) {
        conversation->OnAssociatedContentDestroyed(archived_text_content,
                                                   archived_is_video);
      }
    }
  }

  base::WeakPtr<ConversationHandler::AssociatedContentDelegate> GetWeakPtr() {
    return weak_ptr_factory_.GetWeakPtr();
  }

 private:
  base::WeakPtrFactory<ConversationHandler::AssociatedContentDelegate>
      weak_ptr_factory_{this};
  int content_id_ = 0;
  std::string cached_text_content_;
  std::set<raw_ptr<ConversationHandler>> related_conversations_;
};

}  // namespace

class AIChatServiceUnitTest : public testing::Test,
                              public ::testing::WithParamInterface<bool> {
 public:
  AIChatServiceUnitTest() {
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
        model_service_.get(), std::move(credential_manager), &prefs_, nullptr,
        os_crypt_.get(), shared_url_loader_factory_, "",
        temp_directory_.GetPath());

    client_ =
        std::make_unique<NiceMock<MockServiceClient>>(ai_chat_service_.get());
  }

  void ResetService() {
    ai_chat_service_.reset();
    task_environment_.RunUntilIdle();
    CreateService();
  }

  void ExpectVisibleConversationsSize(base::Location location, size_t size) {
    SCOPED_TRACE(testing::Message() << location.ToString());
    base::RunLoop run_loop;
    client_->service_remote()->GetVisibleConversations(
        base::BindLambdaForTesting(
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

  bool IsAIChatHistoryEnabled() { return GetParam(); }

  void EmulateUserOptedIn() { ::ai_chat::SetUserOptedIn(&prefs_, true); }

  void EmulateUserOptedOut() { ::ai_chat::SetUserOptedIn(&prefs_, false); }

 protected:
  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<AIChatService> ai_chat_service_;
  std::unique_ptr<ModelService> model_service_;
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
      return base::StringPrintf("History%s",
                                info.param ? "Enabled" : "Disabled");
    });

TEST_P(AIChatServiceUnitTest, ConversationLifecycle_NoMessages) {
  // Shouldn't get any visible conversations when no conversations have messages
  EXPECT_CALL(*client_, OnConversationListChanged(testing::IsEmpty()))
      .Times(testing::AnyNumber());

  ConversationHandler* conversation_handler1 = CreateConversation();

  ConversationHandler* conversation_handler2 = CreateConversation();

  // Shouldn't "display" any conversations without messages
  ExpectVisibleConversationsSize(FROM_HERE, 0);

  // Before connecting any clients to the conversations, none should be deleted
  EXPECT_EQ(ai_chat_service_->GetInMemoryConversationCountForTesting(), 2u);

  // Connect a client then disconnect
  auto client1 = CreateConversationClient(conversation_handler1);
  DisconnectConversationClient(client1.get());
  // Only 1 should be deleted
  EXPECT_EQ(ai_chat_service_->GetInMemoryConversationCountForTesting(), 1u);

  // Connect a client then disconnect
  auto client2 = CreateConversationClient(conversation_handler2);
  DisconnectConversationClient(client2.get());
  EXPECT_EQ(ai_chat_service_->GetInMemoryConversationCountForTesting(), 0u);

  testing::Mock::VerifyAndClearExpectations(client_.get());
  task_environment_.RunUntilIdle();
}

TEST_P(AIChatServiceUnitTest, ConversationLifecycle_WithMessages) {
  // Should have these combinations at some point
  EXPECT_CALL(*client_, OnConversationListChanged(testing::SizeIs(0)))
      .Times(testing::AtLeast(1));
  EXPECT_CALL(*client_, OnConversationListChanged(testing::SizeIs(1)))
      .Times(testing::AtLeast(1));
  EXPECT_CALL(*client_, OnConversationListChanged(testing::SizeIs(2)))
      .Times(testing::AtLeast(1));

  ConversationHandler* conversation_handler1 = CreateConversation();
  conversation_handler1->SetChatHistoryForTesting(CreateSampleChatHistory(1u));

  ConversationHandler* conversation_handler2 = CreateConversation();
  conversation_handler2->SetChatHistoryForTesting(CreateSampleChatHistory(1u));

  ExpectVisibleConversationsSize(FROM_HERE, 2u);

  // Before connecting any clients to the conversations, none should be deleted
  EXPECT_EQ(ai_chat_service_->GetInMemoryConversationCountForTesting(), 2u);

  // Connect a client then disconnect
  auto client1 = CreateConversationClient(conversation_handler1);
  DisconnectConversationClient(client1.get());
  // Only 1 should be deleted, whether we preserve history or not (is preserved
  // in the database).
  EXPECT_EQ(ai_chat_service_->GetInMemoryConversationCountForTesting(), 1u);

  ExpectVisibleConversationsSize(FROM_HERE, IsAIChatHistoryEnabled() ? 2u : 1u);

  // Connect a client then disconnect
  auto client2 = CreateConversationClient(conversation_handler2);
  DisconnectConversationClient(client2.get());
  EXPECT_EQ(ai_chat_service_->GetInMemoryConversationCountForTesting(), 0u);

  ExpectVisibleConversationsSize(FROM_HERE, IsAIChatHistoryEnabled() ? 2u : 0u);

  testing::Mock::VerifyAndClearExpectations(client_.get());
  task_environment_.RunUntilIdle();
}

TEST_P(AIChatServiceUnitTest, ConversationLifecycle_WithContent) {
  NiceMock<MockAssociatedContent> associated_content{};
  ON_CALL(associated_content, GetURL())
      .WillByDefault(testing::Return(GURL("https://example.com")));
  associated_content.SetContentId(1);
  ConversationHandler* conversation_with_content_no_messages =
      ai_chat_service_->GetOrCreateConversationHandlerForContent(
          associated_content.GetContentId(), associated_content.GetWeakPtr());
  EXPECT_TRUE(conversation_with_content_no_messages);
  // Asking again for same content ID gets same conversation
  EXPECT_EQ(
      conversation_with_content_no_messages,
      ai_chat_service_->GetOrCreateConversationHandlerForContent(
          associated_content.GetContentId(), associated_content.GetWeakPtr()));
  // Shouldn't be visible without messages
  ExpectVisibleConversationsSize(FROM_HERE, 0u);
  EXPECT_EQ(ai_chat_service_->GetInMemoryConversationCountForTesting(), 1u);
  // Disconnecting the client should unload the handler and delete the
  // conversation.
  auto client1 =
      CreateConversationClient(conversation_with_content_no_messages);
  DisconnectConversationClient(client1.get());
  EXPECT_EQ(ai_chat_service_->GetInMemoryConversationCountForTesting(), 0u);
  ExpectVisibleConversationsSize(FROM_HERE, 0u);

  // Create a new conversation for same content, with messages this time
  ConversationHandler* conversation_with_content =
      ai_chat_service_->GetOrCreateConversationHandlerForContent(
          associated_content.GetContentId(), associated_content.GetWeakPtr());
  conversation_with_content->SetChatHistoryForTesting(
      CreateSampleChatHistory(1u));
  ExpectVisibleConversationsSize(FROM_HERE, 1u);
  EXPECT_EQ(ai_chat_service_->GetInMemoryConversationCountForTesting(), 1u);
  auto client2 = CreateConversationClient(conversation_with_content);
  DisconnectConversationClient(client2.get());
  // Disconnecting all clients should keep the handler in memory until
  // the content is destroyed.
  EXPECT_EQ(ai_chat_service_->GetInMemoryConversationCountForTesting(), 1u);
  ExpectVisibleConversationsSize(FROM_HERE, 1u);
  associated_content.DisassociateWithConversations("", false);

  if (IsAIChatHistoryEnabled()) {
    EXPECT_EQ(ai_chat_service_->GetInMemoryConversationCountForTesting(), 0u);
    ExpectVisibleConversationsSize(FROM_HERE, 1u);
  } else {
    EXPECT_EQ(ai_chat_service_->GetInMemoryConversationCountForTesting(), 0u);
    ExpectVisibleConversationsSize(FROM_HERE, 0u);
  }
}

TEST_P(AIChatServiceUnitTest, GetOrCreateConversationHandlerForContent) {
  ConversationHandler* conversation_without_content = CreateConversation();

  NiceMock<MockAssociatedContent> associated_content{};
  // Allowed scheme to be associated with a conversation
  ON_CALL(associated_content, GetURL())
      .WillByDefault(testing::Return(GURL("https://example.com")));
  associated_content.SetContentId(1);
  ConversationHandler* conversation_with_content =
      ai_chat_service_->GetOrCreateConversationHandlerForContent(
          associated_content.GetContentId(), associated_content.GetWeakPtr());
  EXPECT_TRUE(conversation_with_content);
  EXPECT_NE(conversation_without_content, conversation_with_content);
  EXPECT_NE(conversation_without_content->get_conversation_uuid(),
            conversation_with_content->get_conversation_uuid());
  base::RunLoop run_loop;
  conversation_with_content->GetAssociatedContentInfo(
      base::BindLambdaForTesting(
          [&](mojom::SiteInfoPtr site_info, bool should_send_page_contents) {
            EXPECT_TRUE(site_info->is_content_association_possible);
            EXPECT_TRUE(site_info->url.has_value());
            EXPECT_EQ(site_info->url.value(), GURL("https://example.com"));
            run_loop.Quit();
          }));
  run_loop.Run();

  // Shouldn't create a conversation again when given the same content id
  EXPECT_EQ(
      ai_chat_service_->GetOrCreateConversationHandlerForContent(
          associated_content.GetContentId(), associated_content.GetWeakPtr()),
      conversation_with_content);

  // Creating a second conversation with the same associated content should
  // make the second conversation the default for that content, but leave
  // the first still associated with the content.
  ConversationHandler* conversation2 =
      ai_chat_service_->CreateConversationHandlerForContent(
          associated_content.GetContentId(), associated_content.GetWeakPtr());
  EXPECT_NE(conversation_with_content, conversation2);
  EXPECT_NE(conversation_with_content->get_conversation_uuid(),
            conversation2->get_conversation_uuid());
  EXPECT_EQ(conversation2->GetAssociatedContentDelegateForTesting(),
            &associated_content);
  EXPECT_EQ(conversation_with_content->GetAssociatedContentDelegateForTesting(),
            conversation2->GetAssociatedContentDelegateForTesting());
  // Check the second conversation is the default for that content ID
  EXPECT_EQ(
      ai_chat_service_->GetOrCreateConversationHandlerForContent(
          associated_content.GetContentId(), associated_content.GetWeakPtr()),
      conversation2);
  // Let the conversation be deleted
  std::string conversation2_uuid = conversation2->get_conversation_uuid();
  auto client1 = CreateConversationClient(conversation2);
  DisconnectConversationClient(client1.get());
  ConversationHandler* conversation_with_content3 =
      ai_chat_service_->GetOrCreateConversationHandlerForContent(
          associated_content.GetContentId(), associated_content.GetWeakPtr());
  EXPECT_NE(conversation_with_content3->get_conversation_uuid(),
            conversation2_uuid);
}

TEST_P(AIChatServiceUnitTest,
       GetOrCreateConversationHandlerForContent_DisallowedScheme) {
  NiceMock<MockAssociatedContent> associated_content{};
  // Disallowed scheme to be associated with a conversation
  ON_CALL(associated_content, GetURL())
      .WillByDefault(testing::Return(GURL("chrome://example")));
  ConversationHandler* conversation_with_content =
      ai_chat_service_->GetOrCreateConversationHandlerForContent(
          associated_content.GetContentId(), associated_content.GetWeakPtr());
  EXPECT_TRUE(conversation_with_content);
  // Conversation will still be retrievable via associated content, but won't
  // be provided with the associated content.
  EXPECT_EQ(
      ai_chat_service_->GetOrCreateConversationHandlerForContent(
          associated_content.GetContentId(), associated_content.GetWeakPtr()),
      conversation_with_content);
  base::RunLoop run_loop;
  conversation_with_content->GetAssociatedContentInfo(
      base::BindLambdaForTesting(
          [&](mojom::SiteInfoPtr site_info, bool should_send_page_contents) {
            EXPECT_FALSE(should_send_page_contents);
            EXPECT_FALSE(site_info->is_content_association_possible);
            EXPECT_FALSE(site_info->url.has_value());
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
    ExpectVisibleConversationsSize(FROM_HERE, 1);
    DisconnectConversationClient(client.get());
  }
  ExpectVisibleConversationsSize(FROM_HERE, IsAIChatHistoryEnabled() ? 1 : 0);

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
  conversation_handler3->SetChatHistoryForTesting(CreateSampleChatHistory(1u));

  DisconnectConversationClient(client2.get());
  ExpectVisibleConversationsSize(FROM_HERE, 3);

  // Disable storage
  prefs_.SetBoolean(prefs::kStorageEnabled, false);
  // Wait for OnConversationListChanged which indicates data has been removed
  task_environment_.RunUntilIdle();

  // Conversation with no client was erased from memory
  ExpectVisibleConversationsSize(FROM_HERE, 2);

  // Disconnecting conversations should erase them fom memory
  DisconnectConversationClient(client1.get());
  DisconnectConversationClient(client3.get());
  ExpectVisibleConversationsSize(FROM_HERE, 0);

  // Restart service and verify still doesn't load from storage
  ResetService();
  ExpectVisibleConversationsSize(FROM_HERE, 0);

  // Re-enable storage preference
  prefs_.SetBoolean(prefs::kStorageEnabled, true);
  // Conversations are no longer in persistant storage
  ExpectVisibleConversationsSize(FROM_HERE, 0);
}

TEST_P(AIChatServiceUnitTest, OpenConversationWithStagedEntries_NoPermission) {
  NiceMock<MockAssociatedContent> associated_content{};
  ConversationHandler* conversation =
      ai_chat_service_->CreateConversationHandlerForContent(
          associated_content.GetContentId(), associated_content.GetWeakPtr());
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
  ConversationHandler* conversation =
      ai_chat_service_->CreateConversationHandlerForContent(
          associated_content.GetContentId(), associated_content.GetWeakPtr());
  auto conversation_client = CreateConversationClient(conversation);

  ON_CALL(associated_content, GetStagedEntriesFromContent)
      .WillByDefault(
          [](ConversationHandler::GetStagedEntriesCallback callback) {
            std::move(callback).Run(std::vector<SearchQuerySummary>{
                SearchQuerySummary("query", "summary")});
          });
  ON_CALL(associated_content, HasOpenAIChatPermission)
      .WillByDefault(testing::Return(true));

  // Allowed scheme to be associated with a conversation
  ON_CALL(associated_content, GetURL())
      .WillByDefault(testing::Return(GURL("https://example.com")));

  // One from setting up a connected client, one from
  // OpenConversationWithStagedEntries.
  EXPECT_CALL(associated_content, GetStagedEntriesFromContent).Times(2);

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
  // Create conversations, call DeleteConversations and verify all conversations
  // are deleted, whether a client is connected or not.
  ConversationHandler* conversation_handler1 = CreateConversation();
  auto client1 = CreateConversationClient(conversation_handler1);
  conversation_handler1->SetChatHistoryForTesting(CreateSampleChatHistory(1u));

  ConversationHandler* conversation_handler2 = CreateConversation();
  auto client2 = CreateConversationClient(conversation_handler2);
  conversation_handler2->SetChatHistoryForTesting(CreateSampleChatHistory(1u));

  ConversationHandler* conversation_handler3 = CreateConversation();
  auto client3 = CreateConversationClient(conversation_handler3);
  conversation_handler3->SetChatHistoryForTesting(CreateSampleChatHistory(1u));

  ExpectVisibleConversationsSize(FROM_HERE, 3);

  ai_chat_service_->DeleteConversations();

  ExpectVisibleConversationsSize(FROM_HERE, 0);

  // Verify deleted from database
  ResetService();
  ExpectVisibleConversationsSize(FROM_HERE, 0);
}

TEST_P(AIChatServiceUnitTest, DeleteConversations_TimeRange) {
  // Create conversations, call DeleteConversations and verify all conversations
  // are deleted, whether a client is connected or not.
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

  ExpectVisibleConversationsSize(FROM_HERE, 3);

  ai_chat_service_->DeleteConversations(base::Time::Now() - base::Minutes(245),
                                        base::Time::Now() - base::Minutes(110));

  ExpectVisibleConversationsSize(FROM_HERE, 1);

  // Verify deleted from database
  ResetService();
  ExpectVisibleConversationsSize(FROM_HERE, IsAIChatHistoryEnabled() ? 1 : 0);
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
    ON_CALL(data[i].associated_content, GetURL())
        .WillByDefault(testing::Return(content_url));
    ON_CALL(data[i].associated_content, GetTitle())
        .WillByDefault(testing::Return(page_title));
    ON_CALL(data[i].associated_content, GetTextContent)
        .WillByDefault(testing::Return(page_content));
    data[i].associated_content.SetContentId(i);

    data[i].conversation_handler =
        ai_chat_service_->GetOrCreateConversationHandlerForContent(
            data[i].associated_content.GetContentId(),
            data[i].associated_content.GetWeakPtr());
    EXPECT_TRUE(data[i].conversation_handler);
    data[i].client = CreateConversationClient(data[i].conversation_handler);
    data[i].conversation_handler->SetChatHistoryForTesting(
        CreateSampleChatHistory(1u, -3 + i));

    // Verify associated are initially correct
    base::RunLoop run_loop;
    data[i].conversation_handler->GetAssociatedContentInfo(
        base::BindLambdaForTesting([&](mojom::SiteInfoPtr site_info,
                                       bool should_send_page_contents) {
          SCOPED_TRACE(testing::Message() << "data index: " << i);
          EXPECT_TRUE(site_info->is_content_association_possible);
          EXPECT_TRUE(site_info->url.has_value());
          EXPECT_EQ(site_info->url.value(), content_url);
          EXPECT_EQ(site_info->title.value(), base::UTF16ToUTF8(page_title));
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  // Archive content for conversations 2 and 3
  data[1].associated_content.DisassociateWithConversations(page_content, false);
  data[2].associated_content.DisassociateWithConversations(page_content, false);

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

  ExpectVisibleConversationsSize(FROM_HERE, 3);

  task_environment_.RunUntilIdle();

  for (int i = 0; i < 3; i++) {
    base::RunLoop run_loop;
    data[i].conversation_handler->GetAssociatedContentInfo(
        base::BindLambdaForTesting([&](mojom::SiteInfoPtr site_info,
                                       bool should_send_page_contents) {
          SCOPED_TRACE(testing::Message() << "data index: " << i);
          EXPECT_TRUE(site_info->is_content_association_possible);
          EXPECT_TRUE(site_info->url.has_value());
          EXPECT_TRUE(site_info->title.has_value());
          if (i == 1) {
            EXPECT_TRUE(site_info->url->is_empty());
            EXPECT_TRUE(site_info->title->empty());
          } else {
            EXPECT_EQ(site_info->url.value(), content_url);
            EXPECT_EQ(site_info->title.value(), base::UTF16ToUTF8(page_title));
          }
          run_loop.Quit();
        }));
    run_loop.Run();

    base::RunLoop run_loop_2;
    data[i].conversation_handler->GeneratePageContent(
        base::BindLambdaForTesting([&](std::string content, bool is_video,
                                       std::string invalidation_token) {
          if (i == 1) {
            EXPECT_TRUE(content.empty());
          } else {
            EXPECT_EQ(content, page_content);
          }
          run_loop_2.Quit();
        }));
    run_loop_2.Run();
  }
}

}  // namespace ai_chat
