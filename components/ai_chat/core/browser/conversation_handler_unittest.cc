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

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/functional/overloaded.h"
#include "base/memory/scoped_refptr.h"
#include "base/ranges/algorithm.h"
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
#include "brave/components/ai_chat/core/browser/text_embedder.h"
#include "brave/components/ai_chat/core/browser/types.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-shared.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "components/grit/brave_components_strings.h"
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

bool CompareConversationTurn(const mojom::ConversationTurnPtr& a,
                             const mojom::ConversationTurnPtr& b) {
  if (!a || !b) {
    return a == b;  // Both should be null or neither
  }
  return a->action_type == b->action_type &&
         a->character_type == b->character_type &&
         a->selected_text == b->selected_text && a->text == b->text &&
         a->visibility == b->visibility;
}

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
    prefs::RegisterProfilePrefs(prefs_.registry());
    prefs::RegisterLocalStatePrefs(local_state_.registry());
    ModelService::RegisterProfilePrefs(prefs_.registry());

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
        shared_url_loader_factory_, "");

    conversation_ =
        mojom::Conversation::New("uuid", "title", base::Time::Now(), false);

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

 protected:
  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<AIChatService> ai_chat_service_;
  std::unique_ptr<ModelService> model_service_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
  mojom::ConversationPtr conversation_;
  std::unique_ptr<ConversationHandler> conversation_handler_;
  std::unique_ptr<NiceMock<MockAssociatedContent>> associated_content_;
  bool is_opted_in_ = true;
  bool has_associated_content_ = true;
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
            EXPECT_TRUE(state->suggested_questions.empty());
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
                  mojom::CompletionEvent::New("This is the way."))),
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
  // Human and AI entries.
  EXPECT_CALL(client, OnConversationHistoryUpdate()).Times(2);
  // Fired from OnEngineCompletionComplete.
  EXPECT_CALL(client, OnAPIRequestInProgress(false)).Times(1);
  // Ensure everything is sanitized
  EXPECT_CALL(*engine, SanitizeInput(StrEq(selected_text)));
  EXPECT_CALL(*engine, SanitizeInput(StrEq(expected_turn_text)));

  conversation_handler_->SubmitSelectedText(
      "I have spoken.", mojom::ActionType::SUMMARIZE_SELECTED_TEXT);

  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&client);
  // article_text_ and suggestions_ should be cleared when page content is
  // unlinked.
  conversation_handler_->GetAssociatedContentInfo(base::BindLambdaForTesting(
      [&](mojom::SiteInfoPtr site_info, bool should_send_page_contents) {
        EXPECT_FALSE(should_send_page_contents);
        // We should not have any relationship to associated content
        // once conversation history is committed.
        EXPECT_FALSE(site_info->is_content_association_possible);
      }));
  conversation_handler_->GetSuggestedQuestions(
      base::BindLambdaForTesting([&](const std::vector<std::string>& questions,
                                     mojom::SuggestionGenerationStatus status) {
        EXPECT_TRUE(questions.empty());
      }));

  EXPECT_TRUE(conversation_handler_->HasAnyHistory());
  const auto& history = conversation_handler_->GetConversationHistory();
  std::vector<mojom::ConversationTurnPtr> expected_history;
  expected_history.push_back(mojom::ConversationTurn::New(
      mojom::CharacterType::HUMAN, mojom::ActionType::SUMMARIZE_SELECTED_TEXT,
      mojom::ConversationTurnVisibility::VISIBLE,
      l10n_util::GetStringUTF8(IDS_AI_CHAT_QUESTION_SUMMARIZE_SELECTED_TEXT),
      "I have spoken.", std::nullopt, base::Time::Now(), std::nullopt, false));
  expected_history.push_back(mojom::ConversationTurn::New(
      mojom::CharacterType::ASSISTANT, mojom::ActionType::RESPONSE,
      mojom::ConversationTurnVisibility::VISIBLE, "This is the way.",
      std::nullopt, std::nullopt, base::Time::Now(), std::nullopt, false));
  EXPECT_EQ(history.size(), expected_history.size());
  for (size_t i = 0; i < history.size(); i++) {
    EXPECT_TRUE(CompareConversationTurn(history[i], expected_history[i]));
  }
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
  EXPECT_CALL(*engine, GenerateAssistantResponse(
                           false, StrEq(page_content),
                           LastTurnHasSelectedText(selected_text),
                           StrEq(expected_turn_text), StrEq(""), _, _))
      // Mock the response from the engine
      .WillOnce(::testing::DoAll(
          base::test::RunOnceCallback<5>(
              mojom::ConversationEntryEvent::NewCompletionEvent(
                  mojom::CompletionEvent::New("This is the way."))),
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
  // Human and AI entries.
  EXPECT_CALL(client, OnConversationHistoryUpdate()).Times(2);
  // Fired from OnEngineCompletionComplete.
  EXPECT_CALL(client, OnAPIRequestInProgress(false)).Times(1);
  // Ensure everything is sanitized
  EXPECT_CALL(*engine, SanitizeInput(StrEq(page_content)));
  EXPECT_CALL(*engine, SanitizeInput(StrEq(selected_text)));
  EXPECT_CALL(*engine, SanitizeInput(StrEq(expected_turn_text)));
  // Should not ask LLM for suggested questions
  EXPECT_CALL(*engine, GenerateQuestionSuggestions).Times(0);

  conversation_handler_->SubmitSelectedText(
      "I have spoken again.", mojom::ActionType::SUMMARIZE_SELECTED_TEXT);

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
  conversation_handler_->GetSuggestedQuestions(
      base::BindLambdaForTesting([&](const std::vector<std::string>& questions,
                                     mojom::SuggestionGenerationStatus status) {
        EXPECT_EQ(1u, questions.size());
        EXPECT_EQ(questions[0], "Summarize this page");
      }));

  const auto& history2 = conversation_handler_->GetConversationHistory();
  std::vector<mojom::ConversationTurnPtr> expected_history2;
  expected_history2.push_back(mojom::ConversationTurn::New(
      mojom::CharacterType::HUMAN, mojom::ActionType::SUMMARIZE_SELECTED_TEXT,
      mojom::ConversationTurnVisibility::VISIBLE, expected_turn_text,
      "I have spoken again.", std::nullopt, base::Time::Now(), std::nullopt,
      false));
  expected_history2.push_back(mojom::ConversationTurn::New(
      mojom::CharacterType::ASSISTANT, mojom::ActionType::RESPONSE,
      mojom::ConversationTurnVisibility::VISIBLE, "This is the way.",
      std::nullopt, std::nullopt, base::Time::Now(), std::nullopt, false));
  EXPECT_EQ(history2.size(), expected_history2.size());
  for (size_t i = 0; i < history2.size(); i++) {
    EXPECT_TRUE(CompareConversationTurn(history2[i], expected_history2[i]));
  }
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

  // Setup history for testing.
  auto created_time1 = base::Time::Now();
  std::vector<mojom::ConversationTurnPtr> history;
  history.push_back(mojom::ConversationTurn::New(
      mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
      mojom::ConversationTurnVisibility::VISIBLE, "prompt1", std::nullopt,
      std::nullopt, created_time1, std::nullopt, false));
  history.push_back(mojom::ConversationTurn::New(
      mojom::CharacterType::ASSISTANT, mojom::ActionType::RESPONSE,
      mojom::ConversationTurnVisibility::VISIBLE, "answer1", std::nullopt,
      std::nullopt, base::Time::Now(), std::nullopt, false));
  conversation_handler_->SetChatHistoryForTesting(std::move(history));

  // Modify an entry for the first time.
  EXPECT_CALL(*engine, GenerateAssistantResponse(
                           false, StrEq(""), LastTurnHasText("prompt2"),
                           StrEq("prompt2"), StrEq(""), _, _))
      // Mock the response from the engine
      .WillOnce(::testing::DoAll(
          base::test::RunOnceCallback<5>(
              mojom::ConversationEntryEvent::NewCompletionEvent(
                  mojom::CompletionEvent::New("new answer"))),
          base::test::RunOnceCallback<6>(base::ok(""))));
  conversation_handler_->ModifyConversation(0, "prompt2");
  const auto& conversation_history =
      conversation_handler_->GetConversationHistory();
  ASSERT_EQ(conversation_history.size(), 2u);
  EXPECT_EQ(conversation_history[0]->text, "prompt1");
  EXPECT_EQ(conversation_history[0]->created_time, created_time1);

  ASSERT_TRUE(conversation_history[0]->edits);
  ASSERT_EQ(conversation_history[0]->edits->size(), 1u);
  EXPECT_EQ(conversation_history[0]->edits->at(0)->text, "prompt2");
  EXPECT_NE(conversation_history[0]->edits->at(0)->created_time, created_time1);
  EXPECT_FALSE(conversation_history[0]->edits->at(0)->edits);

  EXPECT_EQ(conversation_history[1]->text, "new answer");

  auto created_time2 = conversation_history[0]->edits->at(0)->created_time;

  // Modify the same entry again.
  EXPECT_CALL(*engine, GenerateAssistantResponse(
                           false, StrEq(""), LastTurnHasText("prompt3"),
                           StrEq("prompt3"), StrEq(""), _, _))
      // Mock the response from the engine
      .WillOnce(::testing::DoAll(
          base::test::RunOnceCallback<5>(
              mojom::ConversationEntryEvent::NewCompletionEvent(
                  mojom::CompletionEvent::New("new answer"))),
          base::test::RunOnceCallback<6>(base::ok(""))));

  conversation_handler_->ModifyConversation(0, "prompt3");
  ASSERT_EQ(conversation_history.size(), 2u);
  EXPECT_EQ(conversation_history[0]->text, "prompt1");
  EXPECT_EQ(conversation_history[0]->created_time, created_time1);

  ASSERT_TRUE(conversation_history[0]->edits);
  ASSERT_EQ(conversation_history[0]->edits->size(), 2u);
  EXPECT_EQ(conversation_history[0]->edits->at(0)->text, "prompt2");
  EXPECT_EQ(conversation_history[0]->edits->at(0)->created_time, created_time2);
  EXPECT_FALSE(conversation_history[0]->edits->at(0)->edits);

  EXPECT_EQ(conversation_history[0]->edits->at(1)->text, "prompt3");
  EXPECT_NE(conversation_history[0]->edits->at(1)->created_time, created_time1);
  EXPECT_NE(conversation_history[0]->edits->at(1)->created_time, created_time2);
  EXPECT_FALSE(conversation_history[0]->edits->at(1)->edits);

  EXPECT_EQ(conversation_history[1]->text, "new answer");

  // Modify server response should have text and completion event updated in
  // the entry of edits.
  // Engine should not be called for an assistant edit
  EXPECT_CALL(*engine, GenerateAssistantResponse(_, _, _, _, _, _, _)).Times(0);
  conversation_handler_->ModifyConversation(1, " answer2 ");
  ASSERT_EQ(conversation_history.size(), 2u);
  EXPECT_EQ(conversation_history[1]->text, "new answer");

  ASSERT_TRUE(conversation_history[1]->edits);
  ASSERT_EQ(conversation_history[1]->edits->size(), 1u);
  EXPECT_EQ(conversation_history[1]->edits->at(0)->text, "answer2");
  EXPECT_NE(conversation_history[1]->edits->at(0)->created_time,
            conversation_history[1]->created_time);

  ASSERT_TRUE(conversation_history[1]->events);
  ASSERT_EQ(conversation_history[1]->events->size(), 1u);
  // Verify the original is left unchanged
  ASSERT_TRUE(conversation_history[1]->events->at(0)->is_completion_event());
  EXPECT_EQ(conversation_history[1]
                ->events->at(0)
                ->get_completion_event()
                ->completion,
            "new answer");

  ASSERT_TRUE(conversation_history[1]->edits->at(0)->events);
  ASSERT_EQ(conversation_history[1]->edits->at(0)->events->size(), 1u);
  ASSERT_TRUE(conversation_history[1]
                  ->edits->at(0)
                  ->events->at(0)
                  ->is_completion_event());
  EXPECT_EQ(conversation_history[1]
                ->edits->at(0)
                ->events->at(0)
                ->get_completion_event()
                ->completion,
            "answer2");
}

TEST_F(ConversationHandlerUnitTest,
       MaybeFetchOrClearContentStagedConversation) {
  // Fetch with result should update the conversation history and call
  // OnConversationHistoryUpdate on observers.
  SetAssociatedContentStagedEntries(/*empty=*/false);
  // Client connecting will trigger content staging
  EXPECT_CALL(*associated_content_, GetStagedEntriesFromContent).Times(1);
  NiceMock<MockConversationHandlerClient> client(conversation_handler_.get());
  EXPECT_CALL(client, OnConversationHistoryUpdate()).Times(1);
  EXPECT_TRUE(conversation_handler_->IsAnyClientConnected());
  conversation_handler_->GetAssociatedContentInfo(base::BindLambdaForTesting(
      [&](mojom::SiteInfoPtr site_info, bool should_send_page_contents) {
        EXPECT_TRUE(should_send_page_contents);
      }));

  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(associated_content_.get());
  testing::Mock::VerifyAndClearExpectations(&client);

  auto& history = conversation_handler_->GetConversationHistory();
  std::vector<mojom::ConversationTurnPtr> expected_history;
  expected_history.push_back(mojom::ConversationTurn::New(
      mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
      mojom::ConversationTurnVisibility::VISIBLE, "query", std::nullopt,
      std::nullopt, base::Time::Now(), std::nullopt, true));
  std::vector<mojom::ConversationEntryEventPtr> events;
  events.push_back(mojom::ConversationEntryEvent::NewCompletionEvent(
      mojom::CompletionEvent::New("summary")));
  expected_history.push_back(mojom::ConversationTurn::New(
      mojom::CharacterType::ASSISTANT, mojom::ActionType::RESPONSE,
      mojom::ConversationTurnVisibility::VISIBLE, "summary", std::nullopt,
      std::move(events), base::Time::Now(), std::nullopt, true));
  ASSERT_EQ(history.size(), expected_history.size());
  for (size_t i = 0; i < history.size(); i++) {
    expected_history[i]->created_time = history[i]->created_time;
    EXPECT_EQ(history[i], expected_history[i]);
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
  EXPECT_CALL(*associated_content_, GetStagedEntriesFromContent).Times(1);
  NiceMock<MockConversationHandlerClient> client(conversation_handler_.get());
  EXPECT_CALL(client, OnConversationHistoryUpdate()).Times(1);
  EXPECT_TRUE(conversation_handler_->IsAnyClientConnected());
  conversation_handler_->GetAssociatedContentInfo(base::BindLambdaForTesting(
      [&](mojom::SiteInfoPtr site_info, bool should_send_page_contents) {
        EXPECT_TRUE(should_send_page_contents);
      }));

  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(associated_content_.get());
  testing::Mock::VerifyAndClearExpectations(&client);

  auto& history = conversation_handler_->GetConversationHistory();
  std::vector<mojom::ConversationTurnPtr> expected_history;
  expected_history.push_back(mojom::ConversationTurn::New(
      mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
      mojom::ConversationTurnVisibility::VISIBLE, "query", std::nullopt,
      std::nullopt, base::Time::Now(), std::nullopt, true));
  std::vector<mojom::ConversationEntryEventPtr> events;
  events.push_back(mojom::ConversationEntryEvent::NewCompletionEvent(
      mojom::CompletionEvent::New("summary")));
  expected_history.push_back(mojom::ConversationTurn::New(
      mojom::CharacterType::ASSISTANT, mojom::ActionType::RESPONSE,
      mojom::ConversationTurnVisibility::VISIBLE, "summary", std::nullopt,
      std::move(events), base::Time::Now(), std::nullopt, true));

  expected_history.push_back(mojom::ConversationTurn::New(
      mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
      mojom::ConversationTurnVisibility::VISIBLE, "query2", std::nullopt,
      std::nullopt, base::Time::Now(), std::nullopt, true));
  std::vector<mojom::ConversationEntryEventPtr> events2;
  events2.push_back(mojom::ConversationEntryEvent::NewCompletionEvent(
      mojom::CompletionEvent::New("summary2")));
  expected_history.push_back(mojom::ConversationTurn::New(
      mojom::CharacterType::ASSISTANT, mojom::ActionType::RESPONSE,
      mojom::ConversationTurnVisibility::VISIBLE, "summary2", std::nullopt,
      std::move(events2), base::Time::Now(), std::nullopt, true));

  ASSERT_EQ(history.size(), expected_history.size());
  for (size_t i = 0; i < history.size(); i++) {
    expected_history[i]->created_time = history[i]->created_time;
    EXPECT_EQ(history[i], expected_history[i]);
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

TEST_F(ConversationHandlerUnitTest_OptedOut,
       MaybeFetchOrClearSearchQuerySummary_NotOptedIn) {
  // Content will have staged entries, but we want to make sure that
  // ConversationHandler won't ask for them when not opted-in yet.
  SetAssociatedContentStagedEntries(/*empty=*/false);
  EXPECT_CALL(*associated_content_, GetStagedEntriesFromContent).Times(0);
  // Modifying whether page contents should be sent should trigger content
  // staging.
  // Don't get a false positive because no client is automatically connected.
  NiceMock<MockConversationHandlerClient> client(conversation_handler_.get());
  EXPECT_TRUE(conversation_handler_->IsAnyClientConnected());
  conversation_handler_->SetShouldSendPageContents(false);
  conversation_handler_->SetShouldSendPageContents(true);
  conversation_handler_->GetAssociatedContentInfo(base::BindLambdaForTesting(
      [&](mojom::SiteInfoPtr site_info, bool should_send_page_contents) {
        EXPECT_TRUE(should_send_page_contents);
      }));

  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(conversation_handler_.get());

  EXPECT_TRUE(conversation_handler_->GetConversationHistory().empty());
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

  // Verify that no re-fetch happens when new client connects
  client.Disconnect();
  task_environment_.RunUntilIdle();
  EXPECT_FALSE(conversation_handler_->IsAnyClientConnected());
  EXPECT_CALL(*associated_content_, GetStagedEntriesFromContent).Times(0);
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

    if (test_case.should_refine_page_content && IsPageContentRefineEnabled()) {
      const auto& conversation_history =
          conversation_handler_->GetConversationHistory();
      ASSERT_FALSE(conversation_history.empty());
      ASSERT_FALSE(conversation_history.back()->events->empty());
      EXPECT_TRUE(conversation_history.back()
                      ->events->back()
                      ->is_page_content_refine_event());
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
