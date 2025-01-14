/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/conversation_handler.h"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/files/scoped_temp_dir.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/functional/overloaded.h"
#include "base/memory/scoped_refptr.h"
#include "base/ranges/algorithm.h"
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
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/browser/engine/mock_engine_consumer.h"
#include "brave/components/ai_chat/core/browser/mock_conversation_handler_observer.h"
#include "brave/components/ai_chat/core/browser/test_utils.h"
#include "brave/components/ai_chat/core/browser/text_embedder.h"
#include "brave/components/ai_chat/core/browser/types.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-shared.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "components/grit/brave_components_strings.h"
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

using ConversationHistory = std::vector<ai_chat::mojom::ConversationTurn>;
using ::testing::_;
using ::testing::ByRef;
using ::testing::NiceMock;
using ::testing::StrEq;

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

class MockConversationHandlerClient : public mojom::ConversationUI {
 public:
  explicit MockConversationHandlerClient(ConversationHandler* driver) {
    driver->Bind(conversation_handler_.BindNewPipeAndPassReceiver(),
                 conversation_ui_receiver_.BindNewPipeAndPassRemote());
  }

  ~MockConversationHandlerClient() override = default;

  void Disconnect() {
    conversation_handler_.reset();
    conversation_ui_receiver_.reset();
  }

  MOCK_METHOD(void, OnConversationHistoryUpdate, (), (override));

  MOCK_METHOD(void, OnAPIRequestInProgress, (bool), (override));

  MOCK_METHOD(void, OnAPIResponseError, (mojom::APIError), (override));

  MOCK_METHOD(void,
              OnModelDataChanged,
              (const std::string& conversation_model_key,
               std::vector<mojom::ModelPtr> all_models),
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
  mojo::Remote<mojom::ConversationHandler> conversation_handler_;
};

class MockAssociatedContent
    : public ConversationHandler::AssociatedContentDelegate {
 public:
  MockAssociatedContent() = default;
  ~MockAssociatedContent() override = default;

  int GetContentId() const override { return 0; }

  MOCK_METHOD(GURL, GetURL, (), (const, override));
  MOCK_METHOD(std::u16string, GetTitle, (), (const, override));
  MOCK_METHOD(std::string_view, GetCachedTextContent, (), (override));
  MOCK_METHOD(bool, GetCachedIsVideo, (), (override));

  MOCK_METHOD(void,
              GetContent,
              (ConversationHandler::GetPageContentCallback),
              (override));

  MOCK_METHOD(void,
              GetStagedEntriesFromContent,
              (ConversationHandler::GetStagedEntriesCallback),
              (override));

  MOCK_METHOD(void,
              OnRelatedConversationDisassociated,
              (ConversationHandler*),
              (override));

  base::WeakPtr<ConversationHandler::AssociatedContentDelegate> GetWeakPtr() {
    return weak_ptr_factory_.GetWeakPtr();
  }

 private:
  base::WeakPtrFactory<ConversationHandler::AssociatedContentDelegate>
      weak_ptr_factory_{this};
};

class MockTextEmbedder : public TextEmbedder {
 public:
  explicit MockTextEmbedder(
      scoped_refptr<base::SequencedTaskRunner> task_runner)
      : TextEmbedder(base::FilePath(), task_runner) {}
  ~MockTextEmbedder() override = default;
  MOCK_METHOD(bool, IsInitialized, (), (const, override));
  MOCK_METHOD(void, Initialize, (InitializeCallback), (override));
  MOCK_METHOD(
      void,
      GetTopSimilarityWithPromptTilContextLimit,
      (const std::string&, const std::string&, uint32_t, TopSimilarityCallback),
      (override));
};

}  // namespace

class ConversationHandlerUnitTest : public testing::Test {
 public:
  void SetUp() override {
    ASSERT_TRUE(temp_directory_.CreateUniqueTempDir());
    prefs::RegisterProfilePrefs(prefs_.registry());
    prefs::RegisterLocalStatePrefs(local_state_.registry());
    ModelService::RegisterProfilePrefs(prefs_.registry());

    os_crypt_ = os_crypt_async::GetTestOSCryptAsyncForTesting(
        /*is_sync_for_unittests=*/true);

    shared_url_loader_factory_ =
        base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
            &url_loader_factory_);
    std::unique_ptr<MockAIChatCredentialManager> credential_manager =
        std::make_unique<MockAIChatCredentialManager>(base::NullCallback(),
                                                      &local_state_);

    ON_CALL(*credential_manager, GetPremiumStatus(_))
        .WillByDefault([&](mojom::Service::GetPremiumStatusCallback callback) {
          mojom::PremiumInfoPtr premium_info = mojom::PremiumInfo::New();
          std::move(callback).Run(mojom::PremiumStatus::Inactive,
                                  std::move(premium_info));
        });

    model_service_ = std::make_unique<ModelService>(&prefs_);

    ai_chat_service_ = std::make_unique<AIChatService>(
        model_service_.get(), std::move(credential_manager), &prefs_, nullptr,
        os_crypt_.get(), shared_url_loader_factory_, "",
        temp_directory_.GetPath());

    mojom::SiteInfoPtr non_content = mojom::SiteInfo::New(
        std::nullopt, mojom::ContentType::PageContent, std::nullopt,
        std::nullopt, std::nullopt, 0, false, false);
    conversation_ =
        mojom::Conversation::New("uuid", "title", base::Time::Now(), false,
                                 std::nullopt, std::move(non_content));

    conversation_handler_ = std::make_unique<ConversationHandler>(
        conversation_.get(), ai_chat_service_.get(), model_service_.get(),
        ai_chat_service_->GetCredentialManagerForTesting(),
        ai_chat_service_->GetFeedbackAPIForTesting(),
        shared_url_loader_factory_);

    conversation_handler_->SetEngineForTesting(
        std::make_unique<NiceMock<MockEngineConsumer>>());

    // Add associated content to conversation
    if (has_associated_content_) {
      associated_content_ = std::make_unique<NiceMock<MockAssociatedContent>>();
      conversation_handler_->SetAssociatedContentDelegate(
          associated_content_->GetWeakPtr());
    }

    if (is_opted_in_) {
      EmulateUserOptedIn();
    } else {
      EmulateUserOptedOut();
    }
  }

  void EmulateUserOptedIn() { ::ai_chat::SetUserOptedIn(&prefs_, true); }

  void EmulateUserOptedOut() { ::ai_chat::SetUserOptedIn(&prefs_, false); }

  void TearDown() override { ai_chat_service_.reset(); }

  void SetAssociatedContentStagedEntries(bool empty = false,
                                         bool multi = false) {
    if (empty) {
      ON_CALL(*associated_content_, GetStagedEntriesFromContent)
          .WillByDefault(
              [](ConversationHandler::GetStagedEntriesCallback callback) {
                std::move(callback).Run(std::nullopt);
              });
      return;
    }
    if (!multi) {
      ON_CALL(*associated_content_, GetStagedEntriesFromContent)
          .WillByDefault(
              [](ConversationHandler::GetStagedEntriesCallback callback) {
                std::move(callback).Run(std::vector<SearchQuerySummary>{
                    SearchQuerySummary("query", "summary")});
              });
      return;
    }
    ON_CALL(*associated_content_, GetStagedEntriesFromContent)
        .WillByDefault(
            [](ConversationHandler::GetStagedEntriesCallback callback) {
              std::move(callback).Run(
                  std::make_optional(std::vector<SearchQuerySummary>{
                      SearchQuerySummary("query", "summary"),
                      SearchQuerySummary("query2", "summary2")}));
            });
  }

  // Pair of text and whether it's from Brave Search SERP
  void SetupHistory(std::vector<std::pair<std::string, bool>> entries) {
    std::vector<mojom::ConversationTurnPtr> history;
    for (size_t i = 0; i < entries.size(); i++) {
      bool is_human = i % 2 == 0;

      std::optional<std::vector<mojom::ConversationEntryEventPtr>> events;
      if (!is_human) {
        events = std::vector<mojom::ConversationEntryEventPtr>();
        events->push_back(mojom::ConversationEntryEvent::NewCompletionEvent(
            mojom::CompletionEvent::New(entries[i].first)));
      }

      auto entry = mojom::ConversationTurn::New(
          std::nullopt,
          is_human ? mojom::CharacterType::HUMAN
                   : mojom::CharacterType::ASSISTANT,
          is_human ? mojom::ActionType::QUERY : mojom::ActionType::RESPONSE,
          mojom::ConversationTurnVisibility::VISIBLE,
          entries[i].first /* text */, std::nullopt /* selected_text */,
          std::move(events), base::Time::Now(), std::nullopt /* edits */,
          entries[i].second /* from_brave_search_SERP */);
      history.push_back(std::move(entry));
    }
    conversation_handler_->SetChatHistoryForTesting(std::move(history));
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<AIChatService> ai_chat_service_;
  std::unique_ptr<ModelService> model_service_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;
  std::unique_ptr<os_crypt_async::OSCryptAsync> os_crypt_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
  mojom::ConversationPtr conversation_;
  std::unique_ptr<ConversationHandler> conversation_handler_;
  std::unique_ptr<NiceMock<MockAssociatedContent>> associated_content_;
  bool is_opted_in_ = true;
  bool has_associated_content_ = true;

 private:
  base::ScopedTempDir temp_directory_;
};

class ConversationHandlerUnitTest_OptedOut
    : public ConversationHandlerUnitTest {
 public:
  ConversationHandlerUnitTest_OptedOut() { is_opted_in_ = false; }
};

class ConversationHandlerUnitTest_NoAssociatedContent
    : public ConversationHandlerUnitTest {
 public:
  ConversationHandlerUnitTest_NoAssociatedContent() {
    has_associated_content_ = false;
  }
};

MATCHER_P(LastTurnHasText, expected_text, "") {
  return !arg.empty() &&
         (!arg.back()->edits->empty() ? arg.back()->edits->back()->text
                                      : arg.back()->text) == expected_text;
}

MATCHER_P(LastTurnHasSelectedText, expected_text, "") {
  return !arg.empty() && arg.back()->selected_text == expected_text;
}

TEST_F(ConversationHandlerUnitTest, GetState) {
  NiceMock<MockConversationHandlerClient> client(conversation_handler_.get());
  for (bool should_send_content : {false, true}) {
    SCOPED_TRACE(testing::Message()
                 << "should_send_content: " << should_send_content);
    base::RunLoop run_loop;
    conversation_handler_->SetShouldSendPageContents(should_send_content);
    EXPECT_FALSE(conversation_handler_->HasAnyHistory());
    conversation_handler_->GetState(
        base::BindLambdaForTesting([&](mojom::ConversationStatePtr state) {
          EXPECT_EQ(state->conversation_uuid, "uuid");
          EXPECT_FALSE(state->is_request_in_progress);
          EXPECT_EQ(state->all_models.size(),
                    model_service_->GetModels().size());
          EXPECT_EQ(state->current_model_key,
                    model_service_->GetDefaultModelKey());
          if (should_send_content) {
            EXPECT_THAT(state->suggested_questions,
                        testing::ElementsAre(l10n_util::GetStringUTF8(
                            IDS_CHAT_UI_SUMMARIZE_PAGE)));
          } else {
            EXPECT_EQ(4u, state->suggested_questions.size());
          }
          EXPECT_EQ(state->suggestion_status,
                    should_send_content
                        ? mojom::SuggestionGenerationStatus::CanGenerate
                        : mojom::SuggestionGenerationStatus::None);
          EXPECT_TRUE(
              state->associated_content_info->is_content_association_possible);
          EXPECT_EQ(state->should_send_content, should_send_content);
          EXPECT_EQ(state->error, mojom::APIError::None);
          run_loop.Quit();
        }));
    run_loop.Run();
  }
}

TEST_F(ConversationHandlerUnitTest, SubmitSelectedText) {
  MockEngineConsumer* engine = static_cast<MockEngineConsumer*>(
      conversation_handler_->GetEngineForTesting());

  std::string selected_text = "I have spoken.";
  std::string expected_turn_text =
      l10n_util::GetStringUTF8(IDS_AI_CHAT_QUESTION_SUMMARIZE_SELECTED_TEXT);
  const std::string expected_response = "This is the way.";

  // Expect the ConversationHandler to call the engine with the selected text
  // and the action's expanded text.
  EXPECT_CALL(*engine,
              GenerateAssistantResponse(
                  false, StrEq(""), LastTurnHasSelectedText(selected_text),
                  StrEq(expected_turn_text), StrEq(""), _, _))
      // Mock the response from the engine
      .WillOnce(::testing::DoAll(
          base::test::RunOnceCallback<5>(
              mojom::ConversationEntryEvent::NewCompletionEvent(
                  mojom::CompletionEvent::New(expected_response))),
          base::test::RunOnceCallback<6>(base::ok(""))));

  EXPECT_FALSE(conversation_handler_->HasAnyHistory());

  // Test without page contents.
  conversation_handler_->GetAssociatedContentInfo(base::BindLambdaForTesting(
      [&](mojom::SiteInfoPtr site_info, bool should_send_page_contents) {
        EXPECT_TRUE(should_send_page_contents);
      }));
  conversation_handler_->SetShouldSendPageContents(false);
  conversation_handler_->GetAssociatedContentInfo(base::BindLambdaForTesting(
      [&](mojom::SiteInfoPtr site_info, bool should_send_page_contents) {
        EXPECT_FALSE(should_send_page_contents);
      }));

  // Should never ask for page content
  EXPECT_CALL(*associated_content_, GetContent).Times(0);

  NiceMock<MockConversationHandlerClient> client(conversation_handler_.get());
  EXPECT_CALL(client, OnAPIRequestInProgress(true)).Times(1);
  // Human, AI entries and content event for AI response.
  EXPECT_CALL(client, OnConversationHistoryUpdate()).Times(3);
  // Fired from OnEngineCompletionComplete.
  EXPECT_CALL(client, OnAPIRequestInProgress(false)).Times(1);
  // Ensure everything is sanitized
  EXPECT_CALL(*engine, SanitizeInput(StrEq(selected_text)));
  EXPECT_CALL(*engine, SanitizeInput(StrEq(expected_turn_text)));

  // Submitting conversation entry should inform associated content
  // that it is no longer associated with the conversation
  // and shouldn't access the conversation because the conversation
  // will not be considering the associated content for lifetime notifications.
  EXPECT_CALL(*associated_content_, OnRelatedConversationDisassociated)
      .Times(1);

  conversation_handler_->SubmitSelectedText(
      selected_text, mojom::ActionType::SUMMARIZE_SELECTED_TEXT);

  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&client);
  testing::Mock::VerifyAndClearExpectations(associated_content_.get());
  // article_text_ and suggestions_ should be cleared when page content is
  // unlinked.
  conversation_handler_->GetAssociatedContentInfo(base::BindLambdaForTesting(
      [&](mojom::SiteInfoPtr site_info, bool should_send_page_contents) {
        EXPECT_FALSE(should_send_page_contents);
        // We should not have any relationship to associated content
        // once conversation history is committed.
        EXPECT_FALSE(site_info->is_content_association_possible);
      }));
  EXPECT_TRUE(conversation_handler_->GetSuggestedQuestionsForTest().empty());

  EXPECT_TRUE(conversation_handler_->HasAnyHistory());
  const auto& history = conversation_handler_->GetConversationHistory();
  std::vector<mojom::ConversationTurnPtr> expected_history;

  expected_history.push_back(mojom::ConversationTurn::New(
      std::nullopt, mojom::CharacterType::HUMAN,
      mojom::ActionType::SUMMARIZE_SELECTED_TEXT,
      mojom::ConversationTurnVisibility::VISIBLE, expected_turn_text,
      selected_text, std::nullopt, base::Time::Now(), std::nullopt, false));

  std::vector<mojom::ConversationEntryEventPtr> response_events;
  response_events.push_back(mojom::ConversationEntryEvent::NewCompletionEvent(
      mojom::CompletionEvent::New(expected_response)));
  expected_history.push_back(mojom::ConversationTurn::New(
      std::nullopt, mojom::CharacterType::ASSISTANT,
      mojom::ActionType::RESPONSE, mojom::ConversationTurnVisibility::VISIBLE,
      expected_response, std::nullopt, std::move(response_events),
      base::Time::Now(), std::nullopt, false));
  ExpectConversationHistoryEquals(FROM_HERE, history, expected_history, false);
}

TEST_F(ConversationHandlerUnitTest, SubmitSelectedText_WithAssociatedContent) {
  // Test with page contents.
  MockEngineConsumer* engine = static_cast<MockEngineConsumer*>(
      conversation_handler_->GetEngineForTesting());

  // Expect the ConversationHandler to call the engine with the selected text
  // and the action's expanded text.
  std::string page_content = "The child's name is Grogu";
  std::string selected_text = "I have spoken again.";
  std::string expected_turn_text =
      l10n_util::GetStringUTF8(IDS_AI_CHAT_QUESTION_SUMMARIZE_SELECTED_TEXT);
  std::string expected_response = "This is the way.";
  EXPECT_CALL(*engine, GenerateAssistantResponse(
                           false, StrEq(page_content),
                           LastTurnHasSelectedText(selected_text),
                           StrEq(expected_turn_text), StrEq(""), _, _))
      // Mock the response from the engine
      .WillOnce(::testing::DoAll(
          base::test::RunOnceCallback<5>(
              mojom::ConversationEntryEvent::NewCompletionEvent(
                  mojom::CompletionEvent::New(expected_response))),
          base::test::RunOnceCallback<6>(base::ok(""))));

  ON_CALL(*associated_content_, GetURL)
      .WillByDefault(testing::Return(GURL("https://www.brave.com")));
  EXPECT_CALL(*associated_content_, GetContent)
      .WillOnce(base::test::RunOnceCallback<0>(page_content, false, ""));
  conversation_handler_->SetShouldSendPageContents(true);
  conversation_handler_->GetAssociatedContentInfo(base::BindLambdaForTesting(
      [&](mojom::SiteInfoPtr site_info, bool should_send_page_contents) {
        EXPECT_TRUE(should_send_page_contents);
        EXPECT_TRUE(site_info->is_content_association_possible);
        EXPECT_EQ(site_info->url->spec(), "https://www.brave.com/");
      }));

  NiceMock<MockConversationHandlerClient> client(conversation_handler_.get());
  EXPECT_CALL(client, OnAPIRequestInProgress(true)).Times(1);
  // Human and AI entries, and content event for AI response.
  EXPECT_CALL(client, OnConversationHistoryUpdate()).Times(3);
  // Fired from OnEngineCompletionComplete.
  EXPECT_CALL(client, OnAPIRequestInProgress(false)).Times(1);
  // Ensure everything is sanitized
  EXPECT_CALL(*engine, SanitizeInput(StrEq(page_content)));
  EXPECT_CALL(*engine, SanitizeInput(StrEq(selected_text)));
  EXPECT_CALL(*engine, SanitizeInput(StrEq(expected_turn_text)));
  // Should not ask LLM for suggested questions
  EXPECT_CALL(*engine, GenerateQuestionSuggestions).Times(0);

  conversation_handler_->SubmitSelectedText(
      selected_text, mojom::ActionType::SUMMARIZE_SELECTED_TEXT);

  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&client);

  // associated info should be unchanged
  conversation_handler_->GetAssociatedContentInfo(base::BindLambdaForTesting(
      [&](mojom::SiteInfoPtr site_info, bool should_send_page_contents) {
        EXPECT_TRUE(should_send_page_contents);
        EXPECT_TRUE(site_info->is_content_association_possible);
        EXPECT_EQ(site_info->url->spec(), "https://www.brave.com/");
      }));

  // Should not be any LLM-generated suggested questions yet because they
  // weren't asked for
  const auto questions = conversation_handler_->GetSuggestedQuestionsForTest();
  EXPECT_EQ(1u, questions.size());
  EXPECT_EQ(questions[0], "Summarize this page");

  const auto& history2 = conversation_handler_->GetConversationHistory();
  std::vector<mojom::ConversationTurnPtr> expected_history2;
  expected_history2.push_back(mojom::ConversationTurn::New(
      std::nullopt, mojom::CharacterType::HUMAN,
      mojom::ActionType::SUMMARIZE_SELECTED_TEXT,
      mojom::ConversationTurnVisibility::VISIBLE, expected_turn_text,
      selected_text, std::nullopt, base::Time::Now(), std::nullopt, false));

  std::vector<mojom::ConversationEntryEventPtr> response_events;
  response_events.push_back(mojom::ConversationEntryEvent::NewCompletionEvent(
      mojom::CompletionEvent::New(expected_response)));
  expected_history2.push_back(mojom::ConversationTurn::New(
      std::nullopt, mojom::CharacterType::ASSISTANT,
      mojom::ActionType::RESPONSE, mojom::ConversationTurnVisibility::VISIBLE,
      expected_response, std::nullopt, std::move(response_events),
      base::Time::Now(), std::nullopt, false));
  ExpectConversationHistoryEquals(FROM_HERE, history2, expected_history2,
                                  false);
}

TEST_F(ConversationHandlerUnitTest, UpdateOrCreateLastAssistantEntry_Delta) {
  // Tests that history combines completion events when the engine provides
  // delta text responses.
  conversation_handler_->SetEngineForTesting(
      std::make_unique<MockEngineConsumer>());
  auto* mock_engine = static_cast<MockEngineConsumer*>(
      conversation_handler_->GetEngineForTesting());
  mock_engine->SetSupportsDeltaTextResponses(true);

  EXPECT_EQ(conversation_handler_->GetConversationHistory().size(), 0u);
  {
    auto event = mojom::ConversationEntryEvent::NewCompletionEvent(
        mojom::CompletionEvent::New("This"));
    conversation_handler_->UpdateOrCreateLastAssistantEntry(std::move(event));

    const std::vector<mojom::ConversationTurnPtr>& history =
        conversation_handler_->GetConversationHistory();
    EXPECT_EQ(history.size(), 1u);

    EXPECT_EQ(history.back()->text, "This");
    auto& events = history.back()->events;
    EXPECT_EQ(events->size(), 1u);

    EXPECT_TRUE(events->at(0)->is_completion_event());
    EXPECT_EQ(events->at(0)->get_completion_event()->completion, "This");
  }
  {
    auto event = mojom::ConversationEntryEvent::NewCompletionEvent(
        mojom::CompletionEvent::New(" is "));
    conversation_handler_->UpdateOrCreateLastAssistantEntry(std::move(event));

    const std::vector<mojom::ConversationTurnPtr>& history =
        conversation_handler_->GetConversationHistory();
    EXPECT_EQ(history.size(), 1u);

    EXPECT_EQ(history.back()->text, "This is ");
    auto& events = history.back()->events;
    EXPECT_EQ(events->size(), 1u);

    EXPECT_TRUE(events->at(0)->is_completion_event());
    EXPECT_EQ(events->at(0)->get_completion_event()->completion, "This is ");
  }
  {
    auto event = mojom::ConversationEntryEvent::NewCompletionEvent(
        mojom::CompletionEvent::New("successful."));
    conversation_handler_->UpdateOrCreateLastAssistantEntry(std::move(event));

    const std::vector<mojom::ConversationTurnPtr>& history =
        conversation_handler_->GetConversationHistory();
    EXPECT_EQ(history.size(), 1u);

    EXPECT_EQ(history.back()->text, "This is successful.");
    auto& events = history.back()->events;
    EXPECT_EQ(events->size(), 1u);

    EXPECT_TRUE(events->at(0)->is_completion_event());
    EXPECT_EQ(events->at(0)->get_completion_event()->completion,
              "This is successful.");
  }
}

TEST_F(ConversationHandlerUnitTest,
       UpdateOrCreateLastAssistantEntry_DeltaWithSearch) {
  // Tests that history combines completion events when the engine provides
  // delta text responses.
  conversation_handler_->SetEngineForTesting(
      std::make_unique<MockEngineConsumer>());
  auto* mock_engine = static_cast<MockEngineConsumer*>(
      conversation_handler_->GetEngineForTesting());
  mock_engine->SetSupportsDeltaTextResponses(true);
  // In addition, add a non-completion event (e.g. search) and verify it's
  // not removed.
  {
    auto event = mojom::ConversationEntryEvent::NewSearchStatusEvent(
        mojom::SearchStatusEvent::New());
    conversation_handler_->UpdateOrCreateLastAssistantEntry(std::move(event));
    const std::vector<mojom::ConversationTurnPtr>& history =
        conversation_handler_->GetConversationHistory();
    EXPECT_EQ(history.size(), 1u);
    auto& events = history.back()->events;
    EXPECT_EQ(events->size(), 1u);
  }
  {
    // Leading space on the first message should be removed
    auto event = mojom::ConversationEntryEvent::NewCompletionEvent(
        mojom::CompletionEvent::New(" This is"));
    conversation_handler_->UpdateOrCreateLastAssistantEntry(std::move(event));

    const std::vector<mojom::ConversationTurnPtr>& history =
        conversation_handler_->GetConversationHistory();
    EXPECT_EQ(history.size(), 1u);

    EXPECT_EQ(history.back()->text, "This is");
    auto& events = history.back()->events;
    EXPECT_EQ(events->size(), 2u);

    EXPECT_TRUE(events->at(1)->is_completion_event());
    EXPECT_EQ(events->at(1)->get_completion_event()->completion, "This is");
  }
  {
    // Leading space on subsequent message should be kept
    auto event = mojom::ConversationEntryEvent::NewCompletionEvent(
        mojom::CompletionEvent::New(" successful."));
    conversation_handler_->UpdateOrCreateLastAssistantEntry(std::move(event));

    const std::vector<mojom::ConversationTurnPtr>& history =
        conversation_handler_->GetConversationHistory();
    EXPECT_EQ(history.size(), 1u);

    EXPECT_EQ(history.back()->text, "This is successful.");
    auto& events = history.back()->events;
    EXPECT_EQ(events->size(), 2u);

    EXPECT_TRUE(events->at(1)->is_completion_event());
    EXPECT_EQ(events->at(1)->get_completion_event()->completion,
              "This is successful.");
  }
}

TEST_F(ConversationHandlerUnitTest, UpdateOrCreateLastAssistantEntry_NotDelta) {
  // Tests that history combines completion events when the engine provides
  // delta text responses.
  conversation_handler_->SetEngineForTesting(
      std::make_unique<MockEngineConsumer>());
  auto* mock_engine = static_cast<MockEngineConsumer*>(
      conversation_handler_->GetEngineForTesting());
  mock_engine->SetSupportsDeltaTextResponses(false);

  EXPECT_EQ(conversation_handler_->GetConversationHistory().size(), 0u);
  {
    auto event = mojom::ConversationEntryEvent::NewCompletionEvent(
        mojom::CompletionEvent::New("This"));
    conversation_handler_->UpdateOrCreateLastAssistantEntry(std::move(event));

    const std::vector<mojom::ConversationTurnPtr>& history =
        conversation_handler_->GetConversationHistory();
    EXPECT_EQ(history.size(), 1u);

    EXPECT_EQ(history.back()->text, "This");
    auto& events = history.back()->events;
    EXPECT_EQ(events->size(), 1u);

    EXPECT_TRUE(events->at(0)->is_completion_event());
    EXPECT_EQ(events->at(0)->get_completion_event()->completion, "This");
  }
  {
    // Leading space should be removed for every partial message
    auto event = mojom::ConversationEntryEvent::NewCompletionEvent(
        mojom::CompletionEvent::New(" This is "));
    conversation_handler_->UpdateOrCreateLastAssistantEntry(std::move(event));

    const std::vector<mojom::ConversationTurnPtr>& history =
        conversation_handler_->GetConversationHistory();
    EXPECT_EQ(history.size(), 1u);

    EXPECT_EQ(history.back()->text, "This is ");
    auto& events = history.back()->events;
    EXPECT_EQ(events->size(), 1u);

    EXPECT_TRUE(events->at(0)->is_completion_event());
    EXPECT_EQ(events->at(0)->get_completion_event()->completion, "This is ");
  }
  {
    auto event = mojom::ConversationEntryEvent::NewCompletionEvent(
        mojom::CompletionEvent::New("This is successful."));
    conversation_handler_->UpdateOrCreateLastAssistantEntry(std::move(event));

    const std::vector<mojom::ConversationTurnPtr>& history =
        conversation_handler_->GetConversationHistory();
    EXPECT_EQ(history.size(), 1u);

    EXPECT_EQ(history.back()->text, "This is successful.");
    auto& events = history.back()->events;
    EXPECT_EQ(events->size(), 1u);

    EXPECT_TRUE(events->at(0)->is_completion_event());
    EXPECT_EQ(events->at(0)->get_completion_event()->completion,
              "This is successful.");
  }
}

TEST_F(ConversationHandlerUnitTest,
       UpdateOrCreateLastAssistantEntry_NotDeltaWithSearch) {
  // Tests that history combines completion events when the engine provides
  // delta text responses.
  conversation_handler_->SetEngineForTesting(
      std::make_unique<MockEngineConsumer>());
  auto* mock_engine = static_cast<MockEngineConsumer*>(
      conversation_handler_->GetEngineForTesting());
  mock_engine->SetSupportsDeltaTextResponses(false);
  // In addition, add a non-completion event (e.g. search) and verify it's
  // not removed.
  {
    auto event = mojom::ConversationEntryEvent::NewSearchStatusEvent(
        mojom::SearchStatusEvent::New());
    conversation_handler_->UpdateOrCreateLastAssistantEntry(std::move(event));
    const std::vector<mojom::ConversationTurnPtr>& history =
        conversation_handler_->GetConversationHistory();
    EXPECT_EQ(history.size(), 1u);
    auto& events = history.back()->events;
    EXPECT_EQ(events->size(), 1u);
  }
  {
    // Leading space should be removed for every partial message
    auto event = mojom::ConversationEntryEvent::NewCompletionEvent(
        mojom::CompletionEvent::New(" This is "));
    conversation_handler_->UpdateOrCreateLastAssistantEntry(std::move(event));

    const std::vector<mojom::ConversationTurnPtr>& history =
        conversation_handler_->GetConversationHistory();
    EXPECT_EQ(history.size(), 1u);

    EXPECT_EQ(history.back()->text, "This is ");
    auto& events = history.back()->events;
    EXPECT_EQ(events->size(), 2u);

    EXPECT_TRUE(events->at(1)->is_completion_event());
    EXPECT_EQ(events->at(1)->get_completion_event()->completion, "This is ");
  }
  {
    auto event = mojom::ConversationEntryEvent::NewCompletionEvent(
        mojom::CompletionEvent::New("This is successful."));
    conversation_handler_->UpdateOrCreateLastAssistantEntry(std::move(event));

    const std::vector<mojom::ConversationTurnPtr>& history =
        conversation_handler_->GetConversationHistory();
    EXPECT_EQ(history.size(), 1u);

    EXPECT_EQ(history.back()->text, "This is successful.");
    auto& events = history.back()->events;
    EXPECT_EQ(events->size(), 2u);

    EXPECT_TRUE(events->at(1)->is_completion_event());
    EXPECT_EQ(events->at(1)->get_completion_event()->completion,
              "This is successful.");
  }
}

TEST_F(ConversationHandlerUnitTest, ModifyConversation) {
  conversation_handler_->SetShouldSendPageContents(false);

  MockEngineConsumer* engine = static_cast<MockEngineConsumer*>(
      conversation_handler_->GetEngineForTesting());

  // Setup history for testing. Items have IDs so we can test removal
  // notifications to an observer.
  std::vector<mojom::ConversationTurnPtr> history = CreateSampleChatHistory(1);
  EXPECT_FALSE(history[0]->edits);
  conversation_handler_->SetChatHistoryForTesting(CloneHistory(history));
  mojom::ConversationEntryEventPtr expected_new_completion_event =
      mojom::ConversationEntryEvent::NewCompletionEvent(
          mojom::CompletionEvent::New("new answer"));
  // Modify an entry for the first time.
  EXPECT_CALL(*engine, GenerateAssistantResponse(
                           false, StrEq(""), LastTurnHasText("prompt2"),
                           StrEq("prompt2"), StrEq(""), _, _))
      // Mock the response from the engine
      .WillOnce(::testing::DoAll(base::test::RunOnceCallback<5>(
                                     expected_new_completion_event->Clone()),
                                 base::test::RunOnceCallback<6>(base::ok(""))));
  testing::NiceMock<MockConversationHandlerObserver> observer;
  // Verify both entries are removed
  EXPECT_CALL(observer, OnConversationEntryRemoved(conversation_handler_.get(),
                                                   history[0]->uuid.value()))
      .Times(1);
  EXPECT_CALL(observer, OnConversationEntryRemoved(conversation_handler_.get(),
                                                   history[1]->uuid.value()))
      .Times(1);
  // Verify edited entry is added as well as the new response
  EXPECT_CALL(observer,
              OnConversationEntryAdded(conversation_handler_.get(), _, _))
      .Times(2);
  observer.Observe(conversation_handler_.get());

  // Make a first edit
  conversation_handler_->ModifyConversation(0, "prompt2");
  testing::Mock::VerifyAndClearExpectations(&observer);

  // Create the entries events in the way we're expecting to look
  // post-modification.
  auto first_edit_expected_history = CloneHistory(history);
  auto first_edit = history[0]->Clone();
  first_edit->uuid = "ignore_me";
  first_edit->selected_text = std::nullopt;
  first_edit->text = "prompt2";

  first_edit_expected_history[0]->edits.emplace();
  first_edit_expected_history[0]->edits->push_back(first_edit->Clone());

  first_edit_expected_history[1]->text = "new answer";
  first_edit_expected_history[1]->events.emplace();
  first_edit_expected_history[1]->events->emplace_back(
      expected_new_completion_event->Clone());

  // Verify the first entry still has original details
  const auto& conversation_history =
      conversation_handler_->GetConversationHistory();

  ExpectConversationHistoryEquals(FROM_HERE, conversation_history,
                                  first_edit_expected_history, false);
  // Create time shouldn't be changed
  EXPECT_EQ(conversation_history[0]->created_time, history[0]->created_time);

  auto created_time2 = conversation_history[0]->edits->at(0)->created_time;
  // New edit should have a different created time
  EXPECT_NE(created_time2, history[0]->created_time);

  // Modify the same entry again.
  EXPECT_CALL(*engine, GenerateAssistantResponse(
                           false, StrEq(""), LastTurnHasText("prompt3"),
                           StrEq("prompt3"), StrEq(""), _, _))
      // Mock the response from the engine
      .WillOnce(::testing::DoAll(base::test::RunOnceCallback<5>(
                                     expected_new_completion_event->Clone()),
                                 base::test::RunOnceCallback<6>(base::ok(""))));

  conversation_handler_->ModifyConversation(0, "prompt3");

  auto second_edit_expected_history = CloneHistory(first_edit_expected_history);
  auto second_edit = first_edit->Clone();
  second_edit->text = "prompt3";
  second_edit_expected_history[0]->edits->emplace_back(second_edit->Clone());

  ExpectConversationHistoryEquals(FROM_HERE, conversation_history,
                                  second_edit_expected_history, false);
  // Create time shouldn't be changed
  EXPECT_EQ(conversation_history[0]->created_time, history[0]->created_time);
  // New edit should have a different create time
  EXPECT_EQ(conversation_history[0]->edits->at(0)->created_time, created_time2);
  EXPECT_NE(conversation_history[0]->edits->at(1)->created_time,
            conversation_history[0]->created_time);
  EXPECT_NE(conversation_history[0]->edits->at(1)->created_time, created_time2);

  // Modifying server response should have text and completion event updated in
  // the entry of edits.
  // Engine should not be called for an assistant edit
  EXPECT_CALL(*engine, GenerateAssistantResponse(_, _, _, _, _, _, _)).Times(0);
  conversation_handler_->ModifyConversation(1, " answer2 ");

  auto third_edit_expected_history = CloneHistory(second_edit_expected_history);

  auto response_edit = third_edit_expected_history[1]->Clone();
  response_edit->uuid = "ignore_me";
  response_edit->text = "answer2";  // trimmed
  response_edit->events->at(0) =
      mojom::ConversationEntryEvent::NewCompletionEvent(
          mojom::CompletionEvent::New("answer2"));

  third_edit_expected_history[1]->edits.emplace();
  third_edit_expected_history[1]->edits->emplace_back(response_edit->Clone());

  ExpectConversationHistoryEquals(FROM_HERE, conversation_history,
                                  third_edit_expected_history, false);

  // Edit time should be set differently
  EXPECT_NE(conversation_history[1]->edits->at(0)->created_time,
            conversation_history[1]->created_time);
}

TEST_F(ConversationHandlerUnitTest,
       MaybeFetchOrClearContentStagedConversation) {
  // Fetch with result should update the conversation history and call
  // OnConversationHistoryUpdate on observers.
  SetAssociatedContentStagedEntries(/*empty=*/false);

  // Shouldn't get any notification of real entries added
  NiceMock<MockConversationHandlerObserver> observer;
  observer.Observe(conversation_handler_.get());
  EXPECT_CALL(observer, OnConversationEntryAdded).Times(0);

  // Client connecting will trigger content staging
  EXPECT_CALL(*associated_content_, GetStagedEntriesFromContent).Times(1);
  NiceMock<MockConversationHandlerClient> client(conversation_handler_.get());
  EXPECT_TRUE(conversation_handler_->IsAnyClientConnected());

  // History update notification once for each entry
  EXPECT_CALL(client, OnConversationHistoryUpdate()).Times(2);

  conversation_handler_->GetAssociatedContentInfo(base::BindLambdaForTesting(
      [&](mojom::SiteInfoPtr site_info, bool should_send_page_contents) {
        EXPECT_TRUE(should_send_page_contents);
      }));

  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(associated_content_.get());
  testing::Mock::VerifyAndClearExpectations(&observer);
  testing::Mock::VerifyAndClearExpectations(&client);

  auto& history = conversation_handler_->GetConversationHistory();
  std::vector<mojom::ConversationTurnPtr> expected_history;
  expected_history.push_back(mojom::ConversationTurn::New(
      std::nullopt, mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
      mojom::ConversationTurnVisibility::VISIBLE, "query", std::nullopt,
      std::nullopt, base::Time::Now(), std::nullopt, true));
  std::vector<mojom::ConversationEntryEventPtr> events;
  events.push_back(mojom::ConversationEntryEvent::NewCompletionEvent(
      mojom::CompletionEvent::New("summary")));
  expected_history.push_back(mojom::ConversationTurn::New(
      std::nullopt, mojom::CharacterType::ASSISTANT,
      mojom::ActionType::RESPONSE, mojom::ConversationTurnVisibility::VISIBLE,
      "summary", std::nullopt, std::move(events), base::Time::Now(),
      std::nullopt, true));
  ASSERT_EQ(history.size(), expected_history.size());
  for (size_t i = 0; i < history.size(); i++) {
    expected_history[i]->created_time = history[i]->created_time;
    ExpectConversationEntryEquals(FROM_HERE, history[i], expected_history[i],
                                  false);
  }
  // HasAnyHistory should still return false since all entries are staged
  EXPECT_FALSE(conversation_handler_->HasAnyHistory());

  // Verify turning off content association clears the conversation history.
  EXPECT_CALL(client, OnConversationHistoryUpdate()).Times(1);
  // Shouldn't ask for staged entries if user doesn't want to be associated
  // with content. This verifies that even with existing staged entries,
  // MaybeFetchOrClearContentStagedConversation will always early return.
  EXPECT_CALL(*associated_content_, GetStagedEntriesFromContent).Times(0);

  conversation_handler_->SetShouldSendPageContents(false);

  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&client);
  testing::Mock::VerifyAndClearExpectations(associated_content_.get());

  EXPECT_TRUE(conversation_handler_->GetConversationHistory().empty());
}

TEST_F(ConversationHandlerUnitTest,
       MaybeFetchOrClearContentStagedConversation_Multi) {
  // Fetch with result should update the conversation history and call
  // OnConversationHistoryUpdate on observers.
  SetAssociatedContentStagedEntries(/*empty=*/false, /*multi=*/true);
  // Client connecting will trigger content staging
  testing::NiceMock<MockConversationHandlerObserver> observer;
  observer.Observe(conversation_handler_.get());
  EXPECT_CALL(observer, OnConversationEntryAdded).Times(0);
  EXPECT_CALL(*associated_content_, GetStagedEntriesFromContent).Times(1);
  NiceMock<MockConversationHandlerClient> client(conversation_handler_.get());
  EXPECT_CALL(client, OnConversationHistoryUpdate()).Times(4);
  EXPECT_TRUE(conversation_handler_->IsAnyClientConnected());
  conversation_handler_->GetAssociatedContentInfo(base::BindLambdaForTesting(
      [&](mojom::SiteInfoPtr site_info, bool should_send_page_contents) {
        EXPECT_TRUE(should_send_page_contents);
      }));

  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(associated_content_.get());
  testing::Mock::VerifyAndClearExpectations(&observer);
  testing::Mock::VerifyAndClearExpectations(&client);

  auto& history = conversation_handler_->GetConversationHistory();
  std::vector<mojom::ConversationTurnPtr> expected_history;
  expected_history.push_back(mojom::ConversationTurn::New(
      std::nullopt, mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
      mojom::ConversationTurnVisibility::VISIBLE, "query", std::nullopt,
      std::nullopt, base::Time::Now(), std::nullopt, true));
  std::vector<mojom::ConversationEntryEventPtr> events;
  events.push_back(mojom::ConversationEntryEvent::NewCompletionEvent(
      mojom::CompletionEvent::New("summary")));
  expected_history.push_back(mojom::ConversationTurn::New(
      std::nullopt, mojom::CharacterType::ASSISTANT,
      mojom::ActionType::RESPONSE, mojom::ConversationTurnVisibility::VISIBLE,
      "summary", std::nullopt, std::move(events), base::Time::Now(),
      std::nullopt, true));

  expected_history.push_back(mojom::ConversationTurn::New(
      std::nullopt, mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
      mojom::ConversationTurnVisibility::VISIBLE, "query2", std::nullopt,
      std::nullopt, base::Time::Now(), std::nullopt, true));
  std::vector<mojom::ConversationEntryEventPtr> events2;
  events2.push_back(mojom::ConversationEntryEvent::NewCompletionEvent(
      mojom::CompletionEvent::New("summary2")));
  expected_history.push_back(mojom::ConversationTurn::New(
      std::nullopt, mojom::CharacterType::ASSISTANT,
      mojom::ActionType::RESPONSE, mojom::ConversationTurnVisibility::VISIBLE,
      "summary2", std::nullopt, std::move(events2), base::Time::Now(),
      std::nullopt, true));

  ASSERT_EQ(history.size(), expected_history.size());
  for (size_t i = 0; i < history.size(); i++) {
    expected_history[i]->created_time = history[i]->created_time;
    ExpectConversationEntryEquals(FROM_HERE, history[i], expected_history[i],
                                  false);
  }
  // HasAnyHistory should still return false since all entries are staged
  EXPECT_FALSE(conversation_handler_->HasAnyHistory());

  // Verify adding an actual conversation entry causes all entries to be
  // notified and HasAnyHistory to return true.
  // Modify an entry for the first time.
  MockEngineConsumer* engine = static_cast<MockEngineConsumer*>(
      conversation_handler_->GetEngineForTesting());
  EXPECT_CALL(*associated_content_, GetContent)
      .WillOnce(base::test::RunOnceCallback<0>("page content", false, ""));
  EXPECT_CALL(*engine, GenerateAssistantResponse)
      // Mock the response from the engine
      .WillOnce(::testing::DoAll(
          base::test::RunOnceCallback<5>(
              mojom::ConversationEntryEvent::NewCompletionEvent(
                  mojom::CompletionEvent::New("new answer"))),
          base::test::RunOnceCallback<6>(base::ok(""))));

  EXPECT_CALL(observer, OnConversationEntryAdded).Times(6);
  EXPECT_CALL(client, OnConversationHistoryUpdate()).Times(3);

  conversation_handler_->SubmitHumanConversationEntry("query3");

  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&client);
  testing::Mock::VerifyAndClearExpectations(&observer);
  testing::Mock::VerifyAndClearExpectations(associated_content_.get());

  EXPECT_TRUE(conversation_handler_->HasAnyHistory());
}

TEST_F(ConversationHandlerUnitTest,
       MaybeFetchOrClearContentStagedConversation_NoResult) {
  // Ensure delegate provides empty result
  SetAssociatedContentStagedEntries(/*empty=*/true);
  // Client connecting will trigger content staging
  EXPECT_CALL(*associated_content_, GetStagedEntriesFromContent).Times(1);
  NiceMock<MockConversationHandlerClient> client(conversation_handler_.get());
  // Should not notify of new history
  EXPECT_CALL(client, OnConversationHistoryUpdate()).Times(0);
  EXPECT_TRUE(conversation_handler_->IsAnyClientConnected());

  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(conversation_handler_.get());
  testing::Mock::VerifyAndClearExpectations(&client);

  // Should not have any history
  EXPECT_TRUE(conversation_handler_->GetConversationHistory().empty());
}

TEST_F(
    ConversationHandlerUnitTest,
    MaybeFetchOrClearContentStagedConversation_FetchStagedEntriesWithHistory) {
  NiceMock<MockConversationHandlerClient> client(conversation_handler_.get());
  ASSERT_TRUE(conversation_handler_->IsAnyClientConnected());

  // MaybeFetchOrClearContentStagedConversation should clear old staged entries
  // and fetch new ones.
  EXPECT_CALL(*associated_content_, GetStagedEntriesFromContent).Times(1);
  // 4 from SetupHistory and 4 from adding
  // new entries in OnGetStagedEntriesFromContent.
  EXPECT_CALL(client, OnConversationHistoryUpdate()).Times(8);

  // Fill history with staged and non-staged entries.
  SetupHistory({{"old query" /* text */, true /*from_brave_search_SERP */},
                {"old summary", "true"},
                {"normal query", false},
                {"normal response", false}});

  // Setting mock return values for GetStagedEntriesFromContent.
  SetAssociatedContentStagedEntries(/*empty=*/false, /*multi=*/true);

  conversation_handler_->MaybeFetchOrClearContentStagedConversation();
  task_environment_.RunUntilIdle();

  testing::Mock::VerifyAndClearExpectations(associated_content_.get());
  testing::Mock::VerifyAndClearExpectations(&client);

  auto& history = conversation_handler_->GetConversationHistory();
  ASSERT_EQ(history.size(), 6u);
  EXPECT_FALSE(history[0]->from_brave_search_SERP);
  EXPECT_EQ(history[0]->text, "normal query");
  EXPECT_FALSE(history[1]->from_brave_search_SERP);
  EXPECT_EQ(history[1]->text, "normal response");
  EXPECT_TRUE(history[2]->from_brave_search_SERP);
  EXPECT_EQ(history[2]->text, "query");
  EXPECT_TRUE(history[3]->from_brave_search_SERP);
  EXPECT_EQ(history[3]->text, "summary");
  EXPECT_TRUE(history[4]->from_brave_search_SERP);
  EXPECT_EQ(history[4]->text, "query2");
  EXPECT_TRUE(history[5]->from_brave_search_SERP);
  EXPECT_EQ(history[5]->text, "summary2");
}

TEST_F(ConversationHandlerUnitTest,
       OnGetStagedEntriesFromContent_FailedChecks) {
  // No staged entries would be added if a request is in progress.
  conversation_handler_->SetRequestInProgressForTesting(true);
  std::vector<SearchQuerySummary> entries = {{"query", "summary"},
                                             {"query2", "summary2"}};
  conversation_handler_->OnGetStagedEntriesFromContent(entries);
  task_environment_.RunUntilIdle();
  EXPECT_EQ(conversation_handler_->GetConversationHistory().size(), 0u);

  // No staged entries if should_send_page_contents_ is false.
  conversation_handler_->SetRequestInProgressForTesting(false);
  conversation_handler_->SetShouldSendPageContents(false);
  conversation_handler_->OnGetStagedEntriesFromContent(entries);
  task_environment_.RunUntilIdle();
  EXPECT_EQ(conversation_handler_->GetConversationHistory().size(), 0u);
}

TEST_F(ConversationHandlerUnitTest, OnGetStagedEntriesFromContent) {
  NiceMock<MockConversationHandlerClient> client(conversation_handler_.get());
  ASSERT_TRUE(conversation_handler_->IsAnyClientConnected());

  EXPECT_CALL(client, OnConversationHistoryUpdate()).Times(8);
  // Fill history with staged and non-staged entries.
  SetupHistory({{"q1" /* text */, true /*from_brave_search_SERP */},
                {"s1", "true"},
                {"q2", false},
                {"r1", false}});

  std::vector<SearchQuerySummary> entries = {{"query", "summary"},
                                             {"query2", "summary2"}};
  conversation_handler_->OnGetStagedEntriesFromContent(entries);
  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&client);

  auto& history = conversation_handler_->GetConversationHistory();
  ASSERT_EQ(history.size(), 6u);
  EXPECT_FALSE(history[0]->from_brave_search_SERP);
  EXPECT_EQ(history[0]->text, "q2");
  EXPECT_FALSE(history[1]->from_brave_search_SERP);
  EXPECT_EQ(history[1]->text, "r1");
  EXPECT_TRUE(history[2]->from_brave_search_SERP);
  EXPECT_EQ(history[2]->text, "query");
  EXPECT_TRUE(history[3]->from_brave_search_SERP);
  EXPECT_EQ(history[3]->text, "summary");
  EXPECT_TRUE(history[4]->from_brave_search_SERP);
  EXPECT_EQ(history[4]->text, "query2");
  EXPECT_TRUE(history[5]->from_brave_search_SERP);
  EXPECT_EQ(history[5]->text, "summary2");
}

TEST_F(ConversationHandlerUnitTest,
       MaybeFetchOrClearSearchQuerySummary_NotOptedIn) {
  // Staged entries could be retrieved before user opts in.
  SetAssociatedContentStagedEntries(/*empty=*/false);
  EXPECT_CALL(*associated_content_, GetStagedEntriesFromContent).Times(1);
  // Don't get a false positive because no client is automatically connected.
  // Connecting a client will trigger content staging.
  NiceMock<MockConversationHandlerClient> client(conversation_handler_.get());
  EXPECT_TRUE(conversation_handler_->IsAnyClientConnected());
  conversation_handler_->GetAssociatedContentInfo(base::BindLambdaForTesting(
      [&](mojom::SiteInfoPtr site_info, bool should_send_page_contents) {
        EXPECT_TRUE(should_send_page_contents);
      }));

  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(conversation_handler_.get());

  EXPECT_FALSE(conversation_handler_->GetConversationHistory().empty());
}

TEST_F(ConversationHandlerUnitTest,
       MaybeFetchOrClearSearchQuerySummary_NotSendingAssociatedContent) {
  // Content will have staged entries, but we want to make sure that
  // ConversationHandler won't ask for them when user has chosen not to
  // use page content.
  SetAssociatedContentStagedEntries(/*empty=*/false);
  conversation_handler_->SetShouldSendPageContents(false);
  conversation_handler_->GetAssociatedContentInfo(base::BindLambdaForTesting(
      [&](mojom::SiteInfoPtr site_info, bool should_send_page_contents) {
        EXPECT_TRUE(site_info->is_content_association_possible);
        EXPECT_FALSE(should_send_page_contents);
      }));

  // Client connecting will trigger content staging
  NiceMock<MockConversationHandlerClient> client(conversation_handler_.get());
  EXPECT_CALL(client, OnConversationHistoryUpdate()).Times(0);
  EXPECT_TRUE(conversation_handler_->IsAnyClientConnected());

  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(conversation_handler_.get());
  testing::Mock::VerifyAndClearExpectations(&client);

  EXPECT_TRUE(conversation_handler_->GetConversationHistory().empty());
}

TEST_F(ConversationHandlerUnitTest_NoAssociatedContent,
       MaybeFetchOrClearSearchQuerySummary) {
  // Ensure nothing gets staged when there's no associated content.
  conversation_handler_->GetAssociatedContentInfo(base::BindLambdaForTesting(
      [&](mojom::SiteInfoPtr site_info, bool should_send_page_contents) {
        EXPECT_FALSE(site_info->is_content_association_possible);
      }));
  // Client connecting would trigger content staging
  NiceMock<MockConversationHandlerClient> client(conversation_handler_.get());
  EXPECT_CALL(client, OnConversationHistoryUpdate()).Times(0);
  EXPECT_TRUE(conversation_handler_->IsAnyClientConnected());

  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(conversation_handler_.get());
  testing::Mock::VerifyAndClearExpectations(&client);

  EXPECT_TRUE(conversation_handler_->GetConversationHistory().empty());
}

TEST_F(ConversationHandlerUnitTest,
       MaybeFetchOrClearSearchQuerySummary_OnClientConnectionChanged) {
  SetAssociatedContentStagedEntries(/*empty=*/false);
  // Verify that no fetch happens when no client
  EXPECT_FALSE(conversation_handler_->IsAnyClientConnected());
  EXPECT_CALL(*associated_content_, GetStagedEntriesFromContent).Times(0);
  // Set page content sending should trigger staged content fetch
  conversation_handler_->SetShouldSendPageContents(false);
  conversation_handler_->SetShouldSendPageContents(true);

  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(associated_content_.get());

  EXPECT_TRUE(conversation_handler_->GetConversationHistory().empty());

  // Verify that fetch happens when first client connects
  EXPECT_CALL(*associated_content_, GetStagedEntriesFromContent).Times(1);
  NiceMock<MockConversationHandlerClient> client(conversation_handler_.get());
  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(associated_content_.get());

  // Verify that fetch happens when another client connects.
  client.Disconnect();
  task_environment_.RunUntilIdle();
  EXPECT_FALSE(conversation_handler_->IsAnyClientConnected());
  EXPECT_CALL(*associated_content_, GetStagedEntriesFromContent).Times(1);
  NiceMock<MockConversationHandlerClient> client2(conversation_handler_.get());
  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(associated_content_.get());
}

TEST_F(ConversationHandlerUnitTest, GenerateQuestions) {
  std::string page_content = "Some example page content";
  const std::string initial_question =
      l10n_util::GetStringUTF8(IDS_CHAT_UI_SUMMARIZE_PAGE);
  const std::vector<std::string> questions = {"Question 1?", "Question 2?",
                                              "Question 3?", "Question 4?"};
  std::vector<std::string> expected_results;
  expected_results.push_back(initial_question);
  base::ranges::copy(questions, std::back_inserter(expected_results));

  conversation_handler_->SetShouldSendPageContents(true);
  EXPECT_CALL(*associated_content_, GetURL)
      .WillOnce(testing::Return(GURL("https://www.example.com")));
  EXPECT_CALL(*associated_content_, GetContent)
      .WillOnce(base::test::RunOnceCallback<0>(page_content, false, ""));

  // Mock engine response
  MockEngineConsumer* engine = static_cast<MockEngineConsumer*>(
      conversation_handler_->GetEngineForTesting());
  EXPECT_CALL(*engine, GenerateQuestionSuggestions(false, StrEq(page_content),
                                                   StrEq(""), _))
      .WillOnce(base::test::RunOnceCallback<3>(questions));

  NiceMock<MockConversationHandlerClient> client(conversation_handler_.get());
  testing::Sequence s;
  EXPECT_CALL(client, OnSuggestedQuestionsChanged(
                          testing::ElementsAre(initial_question),
                          mojom::SuggestionGenerationStatus::IsGenerating))
      .Times(1)
      .InSequence(s);
  EXPECT_CALL(client, OnSuggestedQuestionsChanged(
                          testing::ContainerEq(expected_results),
                          mojom::SuggestionGenerationStatus::HasGenerated))
      .Times(1)
      .InSequence(s);
  conversation_handler_->GenerateQuestions();
  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&client);
  testing::Mock::VerifyAndClearExpectations(associated_content_.get());
  testing::Mock::VerifyAndClearExpectations(engine);
}

TEST_F(ConversationHandlerUnitTest, GenerateQuestions_DisableSendPageContent) {
  conversation_handler_->SetShouldSendPageContents(false);
  conversation_handler_->GetAssociatedContentInfo(base::BindLambdaForTesting(
      [&](mojom::SiteInfoPtr site_info, bool should_send_page_contents) {
        EXPECT_FALSE(should_send_page_contents);
      }));
  EXPECT_CALL(*associated_content_, GetURL).Times(0);
  EXPECT_CALL(*associated_content_, GetContent).Times(0);

  // Mock engine response
  MockEngineConsumer* engine = static_cast<MockEngineConsumer*>(
      conversation_handler_->GetEngineForTesting());
  EXPECT_CALL(*engine, GenerateQuestionSuggestions(_, _, _, _)).Times(0);

  NiceMock<MockConversationHandlerClient> client(conversation_handler_.get());
  testing::Sequence s;
  EXPECT_CALL(client, OnSuggestedQuestionsChanged(_, _)).Times(0);
  conversation_handler_->GenerateQuestions();
  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&client);
  testing::Mock::VerifyAndClearExpectations(associated_content_.get());
  testing::Mock::VerifyAndClearExpectations(engine);
}

TEST_F(ConversationHandlerUnitTest_NoAssociatedContent, GenerateQuestions) {
  // Mock engine response
  MockEngineConsumer* engine = static_cast<MockEngineConsumer*>(
      conversation_handler_->GetEngineForTesting());
  EXPECT_CALL(*engine, GenerateQuestionSuggestions(_, _, _, _)).Times(0);

  NiceMock<MockConversationHandlerClient> client(conversation_handler_.get());
  testing::Sequence s;
  EXPECT_CALL(client, OnSuggestedQuestionsChanged(_, _)).Times(0);
  conversation_handler_->GenerateQuestions();
  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&client);
  testing::Mock::VerifyAndClearExpectations(engine);
}

TEST_F(ConversationHandlerUnitTest_NoAssociatedContent,
       GeneratesQuestionsByDefault) {
  EXPECT_EQ(4u, conversation_handler_->GetSuggestedQuestionsForTest().size());
}

TEST_F(ConversationHandlerUnitTest_NoAssociatedContent,
       SelectingDefaultQuestionSendsPrompt) {
  conversation_handler_->SetSuggestedQuestionForTest("the thing",
                                                     "do the thing!");
  auto suggestions = conversation_handler_->GetSuggestedQuestionsForTest();
  EXPECT_EQ(1u, suggestions.size());

  // Mock engine response
  MockEngineConsumer* engine = static_cast<MockEngineConsumer*>(
      conversation_handler_->GetEngineForTesting());

  base::RunLoop loop;
  // The prompt should be submitted to the engine, not the title.
  EXPECT_CALL(*engine,
              GenerateAssistantResponse(false, StrEq(""), _, "do the thing!",
                                        StrEq(""), _, _))
      .WillOnce(testing::InvokeWithoutArgs(&loop, &base::RunLoop::Quit));

  conversation_handler_->SubmitHumanConversationEntry("the thing");
  loop.Run();
  testing::Mock::VerifyAndClearExpectations(engine);

  // Suggestion should be removed
  EXPECT_EQ(0u, conversation_handler_->GetSuggestedQuestionsForTest().size());
}

TEST_F(ConversationHandlerUnitTest, SelectedLanguage) {
  MockEngineConsumer* engine = static_cast<MockEngineConsumer*>(
      conversation_handler_->GetEngineForTesting());

  std::string expected_input1 = "Now stand aside, worthy adversary.";
  std::string expected_input2 = "A scratch? Your arm's off!";
  std::string expected_selected_language = "fr";

  testing::Sequence s;
  EXPECT_CALL(*engine,
              GenerateAssistantResponse(false, StrEq(""), _, expected_input1,
                                        StrEq(""), _, _))
      .Times(1)
      .WillOnce(::testing::DoAll(
          base::test::RunOnceCallback<5>(
              mojom::ConversationEntryEvent::NewCompletionEvent(
                  mojom::CompletionEvent::New("Tis but a scratch."))),
          base::test::RunOnceCallback<5>(
              mojom::ConversationEntryEvent::NewSelectedLanguageEvent(
                  mojom::SelectedLanguageEvent::New(
                      expected_selected_language))),
          base::test::RunOnceCallback<6>(base::ok(""))));
  EXPECT_CALL(*engine, GenerateAssistantResponse(
                           false, StrEq(""), _, expected_input2,
                           StrEq(expected_selected_language), _, _))
      .WillOnce(::testing::DoAll(
          base::test::RunOnceCallback<5>(
              mojom::ConversationEntryEvent::NewCompletionEvent(
                  mojom::CompletionEvent::New("No, it isn't."))),
          base::test::RunOnceCallback<6>(base::ok(""))));

  conversation_handler_->PerformAssistantGeneration(expected_input1, "", false,
                                                    "");
  conversation_handler_->PerformAssistantGeneration(expected_input2, "", false,
                                                    "");

  // Selected Language events should not be added to the conversation events
  // history
  const auto& conversation_history =
      conversation_handler_->GetConversationHistory();
  ASSERT_FALSE(conversation_history.empty());
  bool has_selected_language_event =
      base::ranges::any_of(conversation_history, [](const auto& conversation) {
        return base::ranges::any_of(
            *conversation->events, [](const auto& event) {
              return event->is_selected_language_event();
            });
      });
  EXPECT_FALSE(has_selected_language_event)
      << "There is an 'is_selected_language_event' present.";

  // And internally the conversation handler should know the selected language
  // was set
  EXPECT_EQ(conversation_handler_->selected_language_,
            expected_selected_language);

  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(engine);
}

TEST_F(ConversationHandlerUnitTest, StopGenerationAndMaybeGetHumanEntry) {
  std::vector<mojom::ConversationTurnPtr> history = CreateSampleChatHistory(1);
  conversation_handler_->SetChatHistoryForTesting(CloneHistory(history));

  // When the last entry isn't human generated the callback should be nullptr
  conversation_handler_->StopGenerationAndMaybeGetHumanEntry(
      base::BindLambdaForTesting(
          [](mojom::ConversationTurnPtr entry) { EXPECT_FALSE(entry); }));

  // Modify the conversation so the last entry is human, pass it to the callback
  history.pop_back();
  conversation_handler_->SetChatHistoryForTesting(CloneHistory(history));
  conversation_handler_->StopGenerationAndMaybeGetHumanEntry(
      base::BindLambdaForTesting([](mojom::ConversationTurnPtr entry) {
        EXPECT_EQ(entry->character_type, mojom::CharacterType::HUMAN);
      }));
}

TEST_F(ConversationHandlerUnitTest, Destuctor) {
  // Verify that the conversation handler cleans up the associated content
  // object when it is destroyed.
  EXPECT_CALL(*associated_content_, OnRelatedConversationDisassociated)
      .Times(1);
  conversation_handler_.reset();
  testing::Mock::VerifyAndClearExpectations(associated_content_.get());
}

class PageContentRefineTest : public ConversationHandlerUnitTest,
                              public testing::WithParamInterface<bool> {
 public:
  PageContentRefineTest() {
    scoped_feature_list_.InitWithFeatureState(features::kPageContentRefine,
                                              GetParam());
  }
  void SetUp() override {
    ConversationHandlerUnitTest::SetUp();
    embedder_task_runner_ = base::ThreadPool::CreateSequencedTaskRunner(
        {base::MayBlock(), base::TaskPriority::USER_BLOCKING});
  }

  bool IsPageContentRefineEnabled() { return GetParam(); }

 protected:
  scoped_refptr<base::SequencedTaskRunner> embedder_task_runner_;

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

INSTANTIATE_TEST_SUITE_P(
    ,
    PageContentRefineTest,
    ::testing::Bool(),
    [](const testing::TestParamInfo<PageContentRefineTest::ParamType>& info) {
      return base::StringPrintf("PageContentRefine_%s",
                                info.param ? "Enabled" : "Disabled");
    });

TEST_P(PageContentRefineTest, TextEmbedder) {
  auto* mock_engine = static_cast<MockEngineConsumer*>(
      conversation_handler_->GetEngineForTesting());

  associated_content_->SetTextEmbedderForTesting(
      std::unique_ptr<MockTextEmbedder, base::OnTaskRunnerDeleter>(
          new MockTextEmbedder(embedder_task_runner_),
          base::OnTaskRunnerDeleter(embedder_task_runner_)));
  auto* mock_text_embedder = static_cast<MockTextEmbedder*>(
      associated_content_->GetTextEmbedderForTesting());

  if (IsPageContentRefineEnabled()) {
    ON_CALL(*mock_text_embedder, IsInitialized)
        .WillByDefault(testing::Return(true));
  } else {
    EXPECT_CALL(*mock_text_embedder, IsInitialized).Times(0);
  }

  uint32_t max_associated_content_length =
      conversation_handler_->GetCurrentModel()
          .options->get_leo_model_options()
          ->max_associated_content_length;
  struct {
    std::string prompt;
    std::string page_content;
    bool should_refine_page_content;
  } test_cases[] = {
      {"prompt1", std::string(max_associated_content_length - 1, 'A'), false},
      {"prompt2", std::string(max_associated_content_length, 'A'), false},
      {"prompt3", std::string(max_associated_content_length + 1, 'A'), true},
      {l10n_util::GetStringUTF8(IDS_AI_CHAT_QUESTION_SUMMARIZE_PAGE),
       std::string(max_associated_content_length + 1, 'A'), false},
  };

  for (const auto& test_case : test_cases) {
    SCOPED_TRACE(testing::Message()
                 << "Prompt: " << test_case.prompt
                 << ", Page content length: " << test_case.page_content.length()
                 << ", Should refine page content: "
                 << test_case.should_refine_page_content);
    if (test_case.should_refine_page_content && IsPageContentRefineEnabled()) {
      EXPECT_CALL(*mock_text_embedder,
                  GetTopSimilarityWithPromptTilContextLimit(
                      test_case.prompt, test_case.page_content,
                      max_associated_content_length, _))
          .Times(1);
      EXPECT_CALL(*mock_engine, GenerateAssistantResponse(_, _, _, _, _, _, _))
          .Times(0);
    } else {
      EXPECT_CALL(*mock_text_embedder,
                  GetTopSimilarityWithPromptTilContextLimit(_, _, _, _))
          .Times(0);
      EXPECT_CALL(*mock_engine, GenerateAssistantResponse(_, _, _, _, _, _, _))
          .Times(1);
    }
    conversation_handler_->PerformAssistantGeneration(
        test_case.prompt, test_case.page_content, false, "");

    testing::Mock::VerifyAndClearExpectations(mock_engine);
    testing::Mock::VerifyAndClearExpectations(mock_text_embedder);

    if (test_case.should_refine_page_content && IsPageContentRefineEnabled()) {
      const auto& conversation_history =
          conversation_handler_->GetConversationHistory();
      ASSERT_FALSE(conversation_history.empty());
      ASSERT_FALSE(conversation_history.back()->events->empty());
      EXPECT_TRUE(conversation_history.back()
                      ->events->back()
                      ->is_page_content_refine_event());

      conversation_handler_->OnGetRefinedPageContent(
          test_case.prompt, base::DoNothing(), base::DoNothing(),
          test_case.page_content, false, base::ok("refined_page_content"));
      EXPECT_FALSE(
          base::ranges::any_of(conversation_history, [](const auto& turn) {
            return !turn->events->empty() &&
                   turn->events->back()->is_page_content_refine_event();
          }));
    }
  }
}

TEST_P(PageContentRefineTest, TextEmbedderInitialized) {
  if (!IsPageContentRefineEnabled()) {
    return;
  }
  auto* mock_engine = static_cast<MockEngineConsumer*>(
      conversation_handler_->GetEngineForTesting());

  associated_content_->SetTextEmbedderForTesting(
      std::unique_ptr<MockTextEmbedder, base::OnTaskRunnerDeleter>(
          new MockTextEmbedder(embedder_task_runner_),
          base::OnTaskRunnerDeleter(embedder_task_runner_)));
  auto* mock_text_embedder = static_cast<MockTextEmbedder*>(
      associated_content_->GetTextEmbedderForTesting());
  struct {
    bool is_initialized;
    bool initialize_result;
  } test_cases[] = {
      {true, false},  // Already initialized so initialize_result is ignored.
      {false, false},
      {false, true},
  };

  uint32_t max_associated_content_length =
      conversation_handler_->GetCurrentModel()
          .options->get_leo_model_options()
          ->max_associated_content_length;
  constexpr char test_prompt[] = "prompt";
  const std::string test_page_content(max_associated_content_length + 1, 'A');
  for (const auto& test_case : test_cases) {
    SCOPED_TRACE(testing::Message()
                 << "IsInitialized: " << test_case.is_initialized
                 << ", Initialize result: " << test_case.initialize_result);

    ON_CALL(*mock_text_embedder, IsInitialized)
        .WillByDefault(testing::Return(test_case.is_initialized));
    if (test_case.is_initialized) {
      EXPECT_CALL(*mock_text_embedder, Initialize).Times(0);
      EXPECT_CALL(*mock_engine, GenerateAssistantResponse(_, _, _, _, _, _, _))
          .Times(0);
      EXPECT_CALL(
          *mock_text_embedder,
          GetTopSimilarityWithPromptTilContextLimit(
              test_prompt, test_page_content, max_associated_content_length, _))
          .Times(1);
    } else {
      EXPECT_CALL(*mock_text_embedder, Initialize)
          .WillOnce(
              base::test::RunOnceCallback<0>(test_case.initialize_result));
      if (!test_case.initialize_result) {
        EXPECT_CALL(*mock_engine,
                    GenerateAssistantResponse(_, _, _, _, _, _, _))
            .Times(1);
        EXPECT_CALL(*mock_text_embedder,
                    GetTopSimilarityWithPromptTilContextLimit(_, _, _, _))
            .Times(0);
      } else {
        EXPECT_CALL(*mock_engine,
                    GenerateAssistantResponse(_, _, _, _, _, _, _))
            .Times(0);
        EXPECT_CALL(*mock_text_embedder,
                    GetTopSimilarityWithPromptTilContextLimit(
                        test_prompt, test_page_content,
                        max_associated_content_length, _))
            .Times(1);
      }
    }

    conversation_handler_->PerformAssistantGeneration(
        test_prompt, test_page_content, false, "");

    if (test_case.is_initialized || test_case.initialize_result) {
      const auto& conversation_history =
          conversation_handler_->GetConversationHistory();
      ASSERT_FALSE(conversation_history.empty());
      ASSERT_FALSE(conversation_history.back()->events->empty());
      EXPECT_TRUE(conversation_history.back()
                      ->events->back()
                      ->is_page_content_refine_event());
    }

    testing::Mock::VerifyAndClearExpectations(mock_engine);
    testing::Mock::VerifyAndClearExpectations(mock_text_embedder);
  }
}

}  // namespace ai_chat
