/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/conversation_handler.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "base/files/scoped_temp_dir.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/functional/callback_helpers.h"
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
#include "base/test/test_future.h"
#include "base/test/values_test_util.h"
#include "base/time/time.h"
#include "base/types/cxx23_to_underlying.h"
#include "brave/components/ai_chat/core/browser/ai_chat_credential_manager.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/browser/associated_archive_content.h"
#include "brave/components/ai_chat/core/browser/associated_content_delegate.h"
#include "brave/components/ai_chat/core/browser/associated_content_manager.h"
#include "brave/components/ai_chat/core/browser/engine/mock_engine_consumer.h"
#include "brave/components/ai_chat/core/browser/mock_conversation_handler_observer.h"
#include "brave/components/ai_chat/core/browser/mock_untrusted_conversation_handler_client.h"
#include "brave/components/ai_chat/core/browser/test/mock_associated_content.h"
#include "brave/components/ai_chat/core/browser/test_utils.h"
#include "brave/components/ai_chat/core/browser/tools/mock_tool.h"
#include "brave/components/ai_chat/core/browser/tools/tool_utils.h"
#include "brave/components/ai_chat/core/browser/types.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/constants.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "brave/components/ai_chat/core/common/prefs.h"
#include "brave/components/ai_chat/core/common/test_utils.h"
#include "build/build_config.h"
#include "build/buildflag.h"
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
#include "third_party/abseil-cpp/absl/strings/str_format.h"
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

class MockToolProvider : public ToolProvider {
 public:
  MockToolProvider() = default;
  ~MockToolProvider() override = default;

  MOCK_METHOD(void, OnNewGenerationLoop, (), (override));
  MOCK_METHOD(std::vector<base::WeakPtr<Tool>>, GetTools, (), (override));
  MOCK_METHOD(void, StopAllTasks, (), (override));

  void StartContentTask(int32_t tab_id) {
    for (auto& observer : observers_) {
      observer.OnContentTaskStarted(tab_id);
    }
  }
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

  MOCK_METHOD(void,
              OnConversationHistoryUpdate,
              (const mojom::ConversationTurnPtr),
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
  mojo::Remote<mojom::ConversationHandler> conversation_handler_;
};

class MockAIChatFeedbackAPI : public AIChatFeedbackAPI {
 public:
  MockAIChatFeedbackAPI() : AIChatFeedbackAPI(nullptr, "") {}

  MOCK_METHOD(void,
              SendRating,
              (bool is_liked,
               bool is_premium,
               const base::span<const mojom::ConversationTurnPtr>& history,
               const std::string& model_name,
               const std::string& selected_language,
               api_request_helper::APIRequestHelper::ResultCallback callback),
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
        model_service_.get(), nullptr /* tab_tracker_service */,
        std::move(credential_manager), &prefs_, nullptr, os_crypt_.get(),
        shared_url_loader_factory_, "", temp_directory_.GetPath());

    mock_feedback_api_ = std::make_unique<NiceMock<MockAIChatFeedbackAPI>>();

    conversation_ = mojom::Conversation::New(
        "uuid", "title", base::Time::Now(), false, std::nullopt, 0, 0, false,
        std::vector<mojom::AssociatedContentPtr>());

    std::vector<std::unique_ptr<ToolProvider>> tool_providers;
    tool_providers.push_back(std::make_unique<NiceMock<MockToolProvider>>());

    conversation_handler_ = std::make_unique<ConversationHandler>(
        conversation_.get(), ai_chat_service_.get(), model_service_.get(),
        ai_chat_service_->GetCredentialManagerForTesting(),
        mock_feedback_api_.get(), &prefs_, shared_url_loader_factory_,
        std::move(tool_providers));

    mock_tool_provider_ = static_cast<MockToolProvider*>(
        conversation_handler_->GetFirstToolProviderForTesting());
    ASSERT_TRUE(mock_tool_provider_);

    // No tools by default
    ON_CALL(*mock_tool_provider_, GetTools()).WillByDefault([]() {
      return std::vector<base::WeakPtr<Tool>>();
    });

    conversation_handler_->SetEngineForTesting(
        std::make_unique<NiceMock<MockEngineConsumer>>());

    // Add associated content to conversation
    if (has_associated_content_) {
      associated_content_ = std::make_unique<NiceMock<MockAssociatedContent>>();
      conversation_handler_->associated_content_manager()->AddContent(
          associated_content_.get(), /*notify_updated=*/true,
          /*detach_existing_content=*/true);
    }

    if (is_opted_in_) {
      EmulateUserOptedIn();
    } else {
      EmulateUserOptedOut();
    }
  }

  void EmulateUserOptedIn() { ::ai_chat::SetUserOptedIn(&prefs_, true); }

  void EmulateUserOptedOut() { ::ai_chat::SetUserOptedIn(&prefs_, false); }

  void TearDown() override {
    mock_tool_provider_ = nullptr;
    ai_chat_service_.reset();
  }

  void SetAssociatedContentStagedEntries(bool empty = false,
                                         bool multi = false) {
    if (empty) {
      ON_CALL(*associated_content_, GetStagedEntriesFromContent)
          .WillByDefault([](GetStagedEntriesCallback callback) {
            std::move(callback).Run(std::nullopt);
          });
      return;
    }
    if (!multi) {
      ON_CALL(*associated_content_, GetStagedEntriesFromContent)
          .WillByDefault([](GetStagedEntriesCallback callback) {
            std::move(callback).Run(std::vector<SearchQuerySummary>{
                SearchQuerySummary("query", "summary")});
          });
      return;
    }
    ON_CALL(*associated_content_, GetStagedEntriesFromContent)
        .WillByDefault([](GetStagedEntriesCallback callback) {
          std::move(callback).Run(
              std::make_optional(std::vector<SearchQuerySummary>{
                  SearchQuerySummary("query", "summary"),
                  SearchQuerySummary("query2", "summary2")}));
        });
  }

  // Pair of text and whether it's from Brave Search SERP
  std::vector<mojom::ConversationTurnPtr> SetupHistory(
      std::vector<std::pair<std::string, bool>> entries) {
    std::vector<mojom::ConversationTurnPtr> history;
    std::vector<mojom::ConversationTurnPtr> expected_history;
    for (size_t i = 0; i < entries.size(); i++) {
      bool is_human = i % 2 == 0;

      std::optional<std::vector<mojom::ConversationEntryEventPtr>> events;
      if (!is_human) {
        events = std::vector<mojom::ConversationEntryEventPtr>();
        events->push_back(mojom::ConversationEntryEvent::NewCompletionEvent(
            mojom::CompletionEvent::New(entries[i].first)));
      }

      auto entry = mojom::ConversationTurn::New(
          "turn-" + base::NumberToString(i),
          is_human ? mojom::CharacterType::HUMAN
                   : mojom::CharacterType::ASSISTANT,
          is_human ? mojom::ActionType::QUERY : mojom::ActionType::RESPONSE,
          entries[i].first /* text */, std::nullopt /* prompt */,
          std::nullopt /* selected_text */, std::move(events),
          base::Time::Now(), std::nullopt /* edits */,
          std::nullopt /* uploaed_images */, nullptr /* skill */,
          entries[i].second /* from_brave_search_SERP */,
          std::nullopt /* model_key */);
      expected_history.push_back(entry.Clone());
      history.push_back(std::move(entry));
    }
    conversation_handler_->SetChatHistoryForTesting(std::move(history));
    return expected_history;
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
  std::unique_ptr<NiceMock<MockAssociatedContent>> associated_content_;
  mojom::ConversationPtr conversation_;
  raw_ptr<MockToolProvider> mock_tool_provider_;
  std::unique_ptr<ConversationHandler> conversation_handler_;
  std::unique_ptr<NiceMock<MockAIChatFeedbackAPI>> mock_feedback_api_;
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

MATCHER_P(ConversationEntriesStateIsGenerating, expected_is_generating, "") {
  return arg->is_generating == expected_is_generating;
}

MATCHER_P(ConversationEntriesStateHasVisualContentPercentage,
          expected_percentage,
          "") {
  return arg->visual_content_used_percentage == expected_percentage;
}

MATCHER(ConversationEntriesStateHasAnyVisualContentPercentage, "") {
  return arg->visual_content_used_percentage.has_value();
}

MATCHER_P(TurnHasText, expected_text, "") {
  return arg->prompt.value_or(arg->text) == expected_text;
}

MATCHER_P(LastTurnHasText, expected_text, "") {
  if (arg.empty()) {
    return false;
  }
  const mojom::ConversationTurnPtr& entry =
      (arg.back()->edits.has_value() && !arg.back()->edits->empty()
           ? arg.back()->edits->back()
           : arg.back());
  return entry->prompt.value_or(entry->text) == expected_text;
}

MATCHER_P(LastTurnHasSelectedText, expected_text, "") {
  return !arg.empty() && arg.back()->selected_text == expected_text;
}

// Can't use mojo::Equals with ::testing::Truly
// because we have uuid and created_time fields
MATCHER_P(TurnEq, expected_turn, "") {
  if (!arg && !expected_turn) {
    return true;
  }
  return arg && expected_turn &&
         arg->character_type == expected_turn->character_type &&
         arg->action_type == expected_turn->action_type &&
         arg->text == expected_turn->text &&
         arg->prompt == expected_turn->prompt &&
         arg->selected_text == expected_turn->selected_text &&
         arg->events == expected_turn->events &&
         arg->edits == expected_turn->edits &&
         arg->uploaded_files == expected_turn->uploaded_files &&
         arg->from_brave_search_SERP == expected_turn->from_brave_search_SERP &&
         arg->model_key == expected_turn->model_key;
}

TEST_F(ConversationHandlerUnitTest, GetState) {
  NiceMock<MockConversationHandlerClient> client(conversation_handler_.get());
  for (bool should_send_content : {false, true}) {
    SCOPED_TRACE(testing::Message()
                 << "should_send_content: " << should_send_content);
    base::RunLoop run_loop;
    if (!should_send_content) {
      conversation_handler_->associated_content_manager()->ClearContent();
    } else {
      conversation_handler_->associated_content_manager()->AddContent(
          associated_content_.get());
    }
    EXPECT_EQ(conversation_handler_->associated_content_manager()
                  ->HasAssociatedContent(),
              should_send_content);
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
          EXPECT_NE(state->associated_content.empty(), should_send_content);
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
  EXPECT_CALL(*engine, GenerateAssistantResponse(
                           _, LastTurnHasSelectedText(selected_text), StrEq(""),
                           _, _, _, _, _, _))
      // Mock the response from the engine
      .WillOnce(::testing::DoAll(
          base::test::RunOnceCallback<7>(EngineConsumer::GenerationResultData(
              mojom::ConversationEntryEvent::NewCompletionEvent(
                  mojom::CompletionEvent::New(expected_response)),
              std::nullopt /* model_key */)),
          base::test::RunOnceCallback<8>(
              base::ok(EngineConsumer::GenerationResultData(
                  mojom::ConversationEntryEvent::NewCompletionEvent(
                      mojom::CompletionEvent::New("")),
                  std::nullopt /* model_key */)))));

  EXPECT_FALSE(conversation_handler_->HasAnyHistory());

  // Test without page contents.
  conversation_handler_->GetAssociatedContentInfo(base::BindLambdaForTesting(
      [&](std::vector<mojom::AssociatedContentPtr> site_info) {
        EXPECT_FALSE(site_info.empty());
      }));
  conversation_handler_->associated_content_manager()->ClearContent();
  conversation_handler_->GetAssociatedContentInfo(base::BindLambdaForTesting(
      [&](std::vector<mojom::AssociatedContentPtr> site_info) {
        EXPECT_TRUE(site_info.empty());
      }));

  std::vector<mojom::ConversationTurnPtr> expected_history;

  expected_history.push_back(mojom::ConversationTurn::New(
      std::nullopt, mojom::CharacterType::HUMAN,
      mojom::ActionType::SUMMARIZE_SELECTED_TEXT, expected_turn_text,
      std::nullopt, selected_text, std::nullopt, base::Time::Now(),
      std::nullopt, std::nullopt, nullptr /* skill */, false,
      std::nullopt /* model_key */));

  std::vector<mojom::ConversationEntryEventPtr> response_events;
  response_events.push_back(mojom::ConversationEntryEvent::NewCompletionEvent(
      mojom::CompletionEvent::New(expected_response)));
  expected_history.push_back(mojom::ConversationTurn::New(
      std::nullopt, mojom::CharacterType::ASSISTANT,
      mojom::ActionType::RESPONSE, expected_response, std::nullopt,
      std::nullopt, std::move(response_events), base::Time::Now(), std::nullopt,
      std::nullopt, nullptr /* skill */, false, std::nullopt /* model_key */));

  NiceMock<MockConversationHandlerClient> client(conversation_handler_.get());
  EXPECT_CALL(client, OnAPIRequestInProgress(true)).Times(1);
  // Human, AI entries and content event for AI response.
  EXPECT_CALL(client, OnConversationHistoryUpdate(
                          TurnEq(mojom::ConversationTurnPtr().get())))
      .Times(1);
  EXPECT_CALL(client,
              OnConversationHistoryUpdate(TurnEq(expected_history[1].get())))
      .Times(2);
  // Fired from OnEngineCompletionComplete.
  EXPECT_CALL(client, OnAPIRequestInProgress(false)).Times(1);
  // Ensure everything is sanitized
  EXPECT_CALL(*engine, SanitizeInput(StrEq(selected_text)));
  EXPECT_CALL(*engine, SanitizeInput(StrEq(expected_turn_text)));

  EXPECT_FALSE(conversation_handler_->associated_content_manager()
                   ->HasAssociatedContent());

  conversation_handler_->SubmitSelectedText(
      selected_text, mojom::ActionType::SUMMARIZE_SELECTED_TEXT);

  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&client);
  testing::Mock::VerifyAndClearExpectations(associated_content_.get());
  // article_text_ and suggestions_ should be cleared when page content is
  // unlinked.
  conversation_handler_->GetAssociatedContentInfo(base::BindLambdaForTesting(
      [&](std::vector<mojom::AssociatedContentPtr> site_info) {
        // We should not have any relationship to associated content
        // once conversation history is committed.
        EXPECT_TRUE(site_info.empty());
      }));
  EXPECT_TRUE(conversation_handler_->GetSuggestedQuestionsForTest().empty());

  EXPECT_TRUE(conversation_handler_->HasAnyHistory());
  const auto& history = conversation_handler_->GetConversationHistory();
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
                           _, LastTurnHasSelectedText(selected_text), StrEq(""),
                           _, _, _, _, _, _))
      // Mock the response from the engine
      .WillOnce(::testing::DoAll(
          base::test::RunOnceCallback<7>(EngineConsumer::GenerationResultData(
              mojom::ConversationEntryEvent::NewCompletionEvent(
                  mojom::CompletionEvent::New(expected_response)),
              std::nullopt /* model_key */)),
          base::test::RunOnceCallback<8>(
              base::ok(EngineConsumer::GenerationResultData(
                  mojom::ConversationEntryEvent::NewCompletionEvent(
                      mojom::CompletionEvent::New("")),
                  std::nullopt /* model_key */)))));

  associated_content_->SetUrl(GURL("https://www.brave.com"));
  associated_content_->SetTextContent(page_content);
  conversation_handler_->GetAssociatedContentInfo(base::BindLambdaForTesting(
      [&](std::vector<mojom::AssociatedContentPtr> site_info) {
        ASSERT_EQ(site_info.size(), 1u);
        EXPECT_EQ(site_info[0]->url, GURL("https://www.brave.com/"));
      }));

  std::vector<mojom::ConversationTurnPtr> expected_history;
  expected_history.push_back(mojom::ConversationTurn::New(
      std::nullopt, mojom::CharacterType::HUMAN,
      mojom::ActionType::SUMMARIZE_SELECTED_TEXT, expected_turn_text,
      std::nullopt, selected_text, std::nullopt, base::Time::Now(),
      std::nullopt, std::nullopt, nullptr /* skill */, false,
      std::nullopt /* model_key */));

  std::vector<mojom::ConversationEntryEventPtr> response_events;
  response_events.push_back(mojom::ConversationEntryEvent::NewCompletionEvent(
      mojom::CompletionEvent::New(expected_response)));
  expected_history.push_back(mojom::ConversationTurn::New(
      std::nullopt, mojom::CharacterType::ASSISTANT,
      mojom::ActionType::RESPONSE, expected_response, std::nullopt,
      std::nullopt, std::move(response_events), base::Time::Now(), std::nullopt,
      std::nullopt, nullptr /* skill */, false, std::nullopt /* model_key */));

  NiceMock<MockConversationHandlerClient> client(conversation_handler_.get());
  EXPECT_CALL(client, OnAPIRequestInProgress(true)).Times(1);
  // Human and AI entries, and content event for AI response.
  EXPECT_CALL(client, OnConversationHistoryUpdate(
                          TurnEq(mojom::ConversationTurnPtr().get())))
      .Times(1);
  EXPECT_CALL(client,
              OnConversationHistoryUpdate(TurnEq(expected_history[1].get())))
      .Times(2);
  // Fired from OnEngineCompletionComplete.
  EXPECT_CALL(client, OnAPIRequestInProgress(false)).Times(1);
  // Ensure everything is sanitized.
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
      [&](std::vector<mojom::AssociatedContentPtr> site_info) {
        ASSERT_EQ(site_info.size(), 1u);
        EXPECT_EQ(site_info[0]->url, GURL("https://www.brave.com/"));
      }));

  // Should not be any LLM-generated suggested questions yet because they
  // weren't asked for
  const auto& questions = conversation_handler_->GetSuggestedQuestionsForTest();
  EXPECT_EQ(1u, questions.size());
  EXPECT_EQ(questions[0].title, "Summarize this page");

  const auto& history = conversation_handler_->GetConversationHistory();
  ExpectConversationHistoryEquals(FROM_HERE, history, expected_history, false);
}

TEST_F(ConversationHandlerUnitTest_NoAssociatedContent,
       MultiContentConversation_AddContent) {
  NiceMock<MockAssociatedContent> associated_content1{};
  associated_content1.SetContentId(1);
  associated_content1.SetTextContent("Content 1");

  NiceMock<MockAssociatedContent> associated_content2{};
  associated_content2.SetContentId(2);
  associated_content2.SetTextContent("Content 2");

  ConversationHandler* conversation = ai_chat_service_->CreateConversation();
  conversation->associated_content_manager()->AddContent(&associated_content1);
  EXPECT_EQ(
      conversation->associated_content_manager()->GetAssociatedContent().size(),
      1u);
  EXPECT_TRUE(
      conversation->associated_content_manager()->HasAssociatedContent());
  EXPECT_TRUE(conversation->associated_content_manager()->HasLiveContent());

  conversation->associated_content_manager()->AddContent(&associated_content2);
  EXPECT_EQ(
      conversation->associated_content_manager()->GetAssociatedContent().size(),
      2u);
  EXPECT_TRUE(
      conversation->associated_content_manager()->HasAssociatedContent());
  EXPECT_TRUE(conversation->associated_content_manager()->HasLiveContent());

  WaitForAssociatedContentFetch(conversation->associated_content_manager());
  auto cached_content =
      conversation->associated_content_manager()->GetCachedContents();
  EXPECT_EQ(cached_content.size(), 2u);
  EXPECT_EQ(cached_content[0].get().content, "Content 1");
  EXPECT_EQ(cached_content[1].get().content, "Content 2");
}

TEST_F(ConversationHandlerUnitTest_NoAssociatedContent,
       MultiContentConversation_AddingContentMultipleTimesDoesNotCrash) {
  NiceMock<MockAssociatedContent> associated_content1{};
  associated_content1.SetContentId(1);
  associated_content1.SetTextContent("Content 1");

  ConversationHandler* conversation = ai_chat_service_->CreateConversation();
  conversation->associated_content_manager()->AddContent(&associated_content1);
  EXPECT_EQ(
      conversation->associated_content_manager()->GetAssociatedContent().size(),
      1u);
  EXPECT_TRUE(
      conversation->associated_content_manager()->HasAssociatedContent());
  EXPECT_TRUE(conversation->associated_content_manager()->HasLiveContent());

  conversation->associated_content_manager()->AddContent(&associated_content1);
  EXPECT_EQ(
      conversation->associated_content_manager()->GetAssociatedContent().size(),
      1u);
  EXPECT_TRUE(
      conversation->associated_content_manager()->HasAssociatedContent());
  EXPECT_TRUE(conversation->associated_content_manager()->HasLiveContent());

  WaitForAssociatedContentFetch(conversation->associated_content_manager());
  auto cached_content =
      conversation->associated_content_manager()->GetCachedContents();
  EXPECT_EQ(cached_content.size(), 1u);
  EXPECT_EQ(cached_content[0].get().content, "Content 1");
}

TEST_F(ConversationHandlerUnitTest_NoAssociatedContent,
       MultiContentConversation_RemoveContent) {
  NiceMock<MockAssociatedContent> associated_content1{};
  associated_content1.SetContentId(1);
  associated_content1.SetTextContent("Content 1");

  NiceMock<MockAssociatedContent> associated_content2{};
  associated_content2.SetContentId(2);
  associated_content2.SetTextContent("Content 2");

  conversation_handler_->associated_content_manager()->AddContent(
      &associated_content1);
  EXPECT_EQ(conversation_handler_->associated_content_manager()
                ->GetAssociatedContent()
                .size(),
            1u);
  EXPECT_TRUE(conversation_handler_->associated_content_manager()
                  ->HasAssociatedContent());
  EXPECT_TRUE(
      conversation_handler_->associated_content_manager()->HasLiveContent());

  conversation_handler_->associated_content_manager()->AddContent(
      &associated_content2);
  EXPECT_EQ(conversation_handler_->associated_content_manager()
                ->GetAssociatedContent()
                .size(),
            2u);
  EXPECT_TRUE(conversation_handler_->associated_content_manager()
                  ->HasAssociatedContent());
  EXPECT_TRUE(
      conversation_handler_->associated_content_manager()->HasLiveContent());

  WaitForAssociatedContentFetch(
      conversation_handler_->associated_content_manager());
  auto cached_content =
      conversation_handler_->associated_content_manager()->GetCachedContents();
  EXPECT_EQ(cached_content.size(), 2u);
  EXPECT_EQ(cached_content[0].get().content, "Content 1");
  EXPECT_EQ(cached_content[1].get().content, "Content 2");

  conversation_handler_->associated_content_manager()->RemoveContent(
      &associated_content1);
  EXPECT_EQ(conversation_handler_->associated_content_manager()
                ->GetAssociatedContent()
                .size(),
            1u);
  EXPECT_TRUE(conversation_handler_->associated_content_manager()
                  ->HasAssociatedContent());
  EXPECT_TRUE(
      conversation_handler_->associated_content_manager()->HasLiveContent());
  WaitForAssociatedContentFetch(
      conversation_handler_->associated_content_manager());
  cached_content =
      conversation_handler_->associated_content_manager()->GetCachedContents();
  EXPECT_EQ(cached_content.size(), 1u);
  EXPECT_EQ(cached_content[0].get().content, "Content 2");
}

TEST_F(ConversationHandlerUnitTest_NoAssociatedContent,
       MultiContentConversation_RemoveArchivedContent) {
  NiceMock<MockAssociatedContent> associated_content1{};
  associated_content1.SetContentId(1);
  associated_content1.SetTextContent("Content 1");

  conversation_handler_->associated_content_manager()->AddContent(
      &associated_content1);
  EXPECT_EQ(conversation_handler_->associated_content_manager()
                ->GetAssociatedContent()
                .size(),
            1u);
  EXPECT_TRUE(conversation_handler_->associated_content_manager()
                  ->HasAssociatedContent());

  WaitForAssociatedContentFetch(
      conversation_handler_->associated_content_manager());
  EXPECT_EQ(conversation_handler_->associated_content_manager()
                ->GetCachedContents()[0]
                .get()
                .content,
            "Content 1");
  conversation_handler_->associated_content_manager()->CreateArchiveContent(
      &associated_content1);

  // Should not be able to remove the content via
  // RemoveContent(associated_content1) now.
  conversation_handler_->associated_content_manager()->RemoveContent(
      &associated_content1);
  EXPECT_EQ(conversation_handler_->associated_content_manager()
                ->GetAssociatedContent()
                .size(),
            1u);

  conversation_handler_->associated_content_manager()->RemoveContent(
      associated_content1.uuid());

  EXPECT_EQ(conversation_handler_->associated_content_manager()
                ->GetAssociatedContent()
                .size(),
            0u);
}

TEST_F(ConversationHandlerUnitTest_NoAssociatedContent,
       MultiContentConversation_AddingContentSetsShouldSend) {
  NiceMock<MockAssociatedContent> associated_content1{};
  associated_content1.SetContentId(1);
  associated_content1.SetTextContent("Content 1");

  NiceMock<MockAssociatedContent> associated_content2{};
  associated_content2.SetContentId(2);
  associated_content2.SetTextContent("Content 2");

  conversation_handler_->associated_content_manager()->AddContent(
      &associated_content1);
  EXPECT_TRUE(conversation_handler_->associated_content_manager()
                  ->HasAssociatedContent());

  conversation_handler_->associated_content_manager()->AddContent(
      &associated_content2);
  EXPECT_TRUE(conversation_handler_->associated_content_manager()
                  ->HasAssociatedContent());
}

TEST_F(
    ConversationHandlerUnitTest_NoAssociatedContent,
    MultiContentConversation_RemovingContentShouldSetShouldSendIfHasAssociatedContent) {
  NiceMock<MockAssociatedContent> associated_content1{};
  associated_content1.SetContentId(1);
  associated_content1.SetTextContent("Content 1");

  NiceMock<MockAssociatedContent> associated_content2{};
  associated_content2.SetContentId(2);
  associated_content2.SetTextContent("Content 2");

  conversation_handler_->associated_content_manager()->AddContent(
      &associated_content1);
  conversation_handler_->associated_content_manager()->AddContent(
      &associated_content2);

  conversation_handler_->associated_content_manager()->RemoveContent(
      &associated_content1);
  EXPECT_TRUE(conversation_handler_->associated_content_manager()
                  ->HasAssociatedContent());

  conversation_handler_->associated_content_manager()->RemoveContent(
      &associated_content2);
  EXPECT_FALSE(conversation_handler_->associated_content_manager()
                   ->HasAssociatedContent());
}

TEST_F(ConversationHandlerUnitTest_NoAssociatedContent,
       MultiContentConversation_ArchiveContent) {
  NiceMock<MockAssociatedContent> associated_content1{};
  associated_content1.SetContentId(1);
  associated_content1.SetTextContent("Content 1");

  NiceMock<MockAssociatedContent> associated_content2{};
  associated_content2.SetContentId(2);
  associated_content2.SetTextContent("Content 2");

  conversation_handler_->associated_content_manager()->AddContent(
      &associated_content1);
  conversation_handler_->associated_content_manager()->AddContent(
      &associated_content2);

  EXPECT_TRUE(
      conversation_handler_->associated_content_manager()->HasLiveContent());
  WaitForAssociatedContentFetch(
      conversation_handler_->associated_content_manager());
  auto cached_content =
      conversation_handler_->associated_content_manager()->GetCachedContents();
  EXPECT_EQ(cached_content.size(), 2u);
  EXPECT_EQ(cached_content[0].get().content, "Content 1");
  EXPECT_EQ(cached_content[1].get().content, "Content 2");

  conversation_handler_->associated_content_manager()->OnRequestArchive(
      &associated_content1);
  EXPECT_TRUE(
      conversation_handler_->associated_content_manager()->HasLiveContent());
  WaitForAssociatedContentFetch(
      conversation_handler_->associated_content_manager());
  cached_content =
      conversation_handler_->associated_content_manager()->GetCachedContents();
  EXPECT_EQ(cached_content.size(), 2u);
  EXPECT_EQ(cached_content[0].get().content, "Content 1");
  EXPECT_EQ(cached_content[1].get().content, "Content 2");

  conversation_handler_->associated_content_manager()->OnRequestArchive(
      &associated_content2);
  // Everything should be archived now
  EXPECT_FALSE(
      conversation_handler_->associated_content_manager()->HasLiveContent());
  WaitForAssociatedContentFetch(
      conversation_handler_->associated_content_manager());
  cached_content =
      conversation_handler_->associated_content_manager()->GetCachedContents();
  EXPECT_EQ(cached_content.size(), 2u);
  EXPECT_EQ(cached_content[0].get().content, "Content 1");
  EXPECT_EQ(cached_content[1].get().content, "Content 2");
}

TEST_F(ConversationHandlerUnitTest_NoAssociatedContent,
       MultiContentConversation_LoadArchivedContent) {
  auto metadata = mojom::Conversation::New();
  metadata->associated_content.push_back(mojom::AssociatedContent::New(
      "1", mojom::ContentType::PageContent, "Content 1", 1,
      GURL("https://one.com"), 100, "turn-1"));
  metadata->associated_content.push_back(mojom::AssociatedContent::New(
      "2", mojom::ContentType::PageContent, "Content 2", 2,
      GURL("https://two.com"), 100, "turn-1"));

  auto conversation_archive = mojom::ConversationArchive::New();
  conversation_archive->associated_content.push_back(
      mojom::ContentArchive::New("1", "The content of one", "turn-1"));
  conversation_archive->associated_content.push_back(
      mojom::ContentArchive::New("2", "The content of two", "turn-1"));

  conversation_handler_->associated_content_manager()->LoadArchivedContent(
      metadata.get(), conversation_archive);

  EXPECT_EQ(conversation_handler_->associated_content_manager()
                ->GetAssociatedContent()
                .size(),
            2u);
  auto cached_content =
      conversation_handler_->associated_content_manager()->GetCachedContents();
  EXPECT_EQ(cached_content.size(), 2u);
  EXPECT_EQ(cached_content[0].get().content, "The content of one");
  EXPECT_EQ(cached_content[1].get().content, "The content of two");
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
    auto result = EngineConsumer::GenerationResultData(
        mojom::ConversationEntryEvent::NewCompletionEvent(
            mojom::CompletionEvent::New("This")),
        std::nullopt /* model_key */);
    conversation_handler_->UpdateOrCreateLastAssistantEntry(std::move(result));

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
    auto result = EngineConsumer::GenerationResultData(
        mojom::ConversationEntryEvent::NewCompletionEvent(
            mojom::CompletionEvent::New(" is ")),
        std::nullopt /* model_key */);
    conversation_handler_->UpdateOrCreateLastAssistantEntry(std::move(result));

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
    auto result = EngineConsumer::GenerationResultData(
        mojom::ConversationEntryEvent::NewCompletionEvent(
            mojom::CompletionEvent::New("successful.")),
        std::nullopt /* model_key */);
    conversation_handler_->UpdateOrCreateLastAssistantEntry(std::move(result));

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
    auto result = EngineConsumer::GenerationResultData(
        mojom::ConversationEntryEvent::NewSearchStatusEvent(
            mojom::SearchStatusEvent::New()),
        std::nullopt /* model_key */);
    conversation_handler_->UpdateOrCreateLastAssistantEntry(std::move(result));
    const std::vector<mojom::ConversationTurnPtr>& history =
        conversation_handler_->GetConversationHistory();
    EXPECT_EQ(history.size(), 1u);
    auto& events = history.back()->events;
    EXPECT_EQ(events->size(), 1u);
  }
  {
    // Leading space on the first message should be removed
    auto result = EngineConsumer::GenerationResultData(
        mojom::ConversationEntryEvent::NewCompletionEvent(
            mojom::CompletionEvent::New(" This is")),
        std::nullopt /* model_key */);
    conversation_handler_->UpdateOrCreateLastAssistantEntry(std::move(result));

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
    auto result = EngineConsumer::GenerationResultData(
        mojom::ConversationEntryEvent::NewCompletionEvent(
            mojom::CompletionEvent::New(" successful.")),
        std::nullopt /* model_key */);
    conversation_handler_->UpdateOrCreateLastAssistantEntry(std::move(result));

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
    auto result = EngineConsumer::GenerationResultData(
        mojom::ConversationEntryEvent::NewCompletionEvent(
            mojom::CompletionEvent::New("This")),
        std::nullopt /* model_key */);
    conversation_handler_->UpdateOrCreateLastAssistantEntry(std::move(result));

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
    auto result = EngineConsumer::GenerationResultData(
        mojom::ConversationEntryEvent::NewCompletionEvent(
            mojom::CompletionEvent::New(" This is ")),
        std::nullopt /* model_key */);
    conversation_handler_->UpdateOrCreateLastAssistantEntry(std::move(result));

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
    auto result = EngineConsumer::GenerationResultData(
        mojom::ConversationEntryEvent::NewCompletionEvent(
            mojom::CompletionEvent::New("This is successful.")),
        std::nullopt /* model_key */);
    conversation_handler_->UpdateOrCreateLastAssistantEntry(std::move(result));

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
    auto result = EngineConsumer::GenerationResultData(
        mojom::ConversationEntryEvent::NewSearchStatusEvent(
            mojom::SearchStatusEvent::New()),
        std::nullopt /* model_key */);
    conversation_handler_->UpdateOrCreateLastAssistantEntry(std::move(result));
    const std::vector<mojom::ConversationTurnPtr>& history =
        conversation_handler_->GetConversationHistory();
    EXPECT_EQ(history.size(), 1u);
    auto& events = history.back()->events;
    EXPECT_EQ(events->size(), 1u);
  }
  {
    // Leading space should be removed for every partial message
    auto result = EngineConsumer::GenerationResultData(
        mojom::ConversationEntryEvent::NewCompletionEvent(
            mojom::CompletionEvent::New(" This is ")),
        std::nullopt /* model_key */);
    conversation_handler_->UpdateOrCreateLastAssistantEntry(std::move(result));

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
    auto result = EngineConsumer::GenerationResultData(
        mojom::ConversationEntryEvent::NewCompletionEvent(
            mojom::CompletionEvent::New("This is successful.")),
        std::nullopt /* model_key */);
    conversation_handler_->UpdateOrCreateLastAssistantEntry(std::move(result));

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

// TODO(https://github.com/brave/brave-browser/issues/47838)
#if BUILDFLAG(IS_IOS)
#define MAYBE_ModifyConversation DISABLED_ModifyConversation
#else
#define MAYBE_ModifyConversation ModifyConversation
#endif  // BUILDFLAG(IS_IOS)
TEST_F(ConversationHandlerUnitTest, MAYBE_ModifyConversation) {
  conversation_handler_->MaybeUnlinkAssociatedContent();

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
  EXPECT_CALL(*engine, GenerateAssistantResponse(_, LastTurnHasText("prompt2"),
                                                 StrEq(""), _, _, _, _, _, _))
      // Mock the response from the engine
      .WillOnce(::testing::DoAll(
          base::test::RunOnceCallback<7>(EngineConsumer::GenerationResultData(
              expected_new_completion_event->Clone(),
              "chat-basic" /* model_key */)),
          base::test::RunOnceCallback<8>(
              base::ok(EngineConsumer::GenerationResultData(
                  mojom::ConversationEntryEvent::NewCompletionEvent(
                      mojom::CompletionEvent::New("")),
                  "chat-basic" /* model_key */)))));
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
  conversation_handler_->ModifyConversation(history[0]->uuid.value(),
                                            "prompt2");
  testing::Mock::VerifyAndClearExpectations(&observer);

  // Create the entries events in the way we're expecting to look
  // post-modification.
  auto first_edit_expected_history = CloneHistory(history);
  auto first_edit = history[0]->Clone();
  first_edit->uuid = "ignore_me";
  first_edit->selected_text = std::nullopt;
  first_edit->text = "prompt2";
  first_edit->created_time = base::Time::Now();

  first_edit_expected_history[0]->edits.emplace();
  first_edit_expected_history[0]->edits->push_back(first_edit->Clone());

  first_edit_expected_history[1]->text = "new answer";
  first_edit_expected_history[1]->created_time = base::Time::Now();
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
  EXPECT_CALL(*engine, GenerateAssistantResponse(_, LastTurnHasText("prompt3"),
                                                 StrEq(""), _, _, _, _, _, _))
      // Mock the response from the engine
      .WillOnce(::testing::DoAll(
          base::test::RunOnceCallback<7>(EngineConsumer::GenerationResultData(
              expected_new_completion_event->Clone(),
              "chat-basic" /* model_key */)),
          base::test::RunOnceCallback<8>(
              base::ok(EngineConsumer::GenerationResultData(
                  mojom::ConversationEntryEvent::NewCompletionEvent(
                      mojom::CompletionEvent::New("")),
                  "chat-basic" /* model_key */)))));

  conversation_handler_->ModifyConversation(
      conversation_history[0]->uuid.value(), "prompt3");

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
  EXPECT_CALL(*engine, GenerateAssistantResponse(_, _, _, _, _, _, _, _, _))
      .Times(0);
  conversation_handler_->ModifyConversation(
      conversation_history[1]->uuid.value(), " answer2 ");

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

TEST_F(ConversationHandlerUnitTest, RegenerateAnswer) {
  conversation_handler_->MaybeUnlinkAssociatedContent();

  MockEngineConsumer* engine = static_cast<MockEngineConsumer*>(
      conversation_handler_->GetEngineForTesting());

  // Setup history with 4 turns: human1, assistant1, human2, assistant2
  std::vector<mojom::ConversationTurnPtr> history = CreateSampleChatHistory(2);
  conversation_handler_->SetChatHistoryForTesting(CloneHistory(history));

  NiceMock<MockConversationHandlerObserver> observer;
  observer.Observe(conversation_handler_.get());

  // Add mock client to track API request progress
  NiceMock<MockConversationHandlerClient> client(conversation_handler_.get());
  EXPECT_CALL(client, OnAPIRequestInProgress(true)).Times(1);

  // Setup expectations
  const std::string new_model_key = "new_model_key";
  const std::string assistant_turn_uuid = history[1]->uuid.value();
  const std::string human_turn_uuid = history[0]->uuid.value();

  // We should call GenerateAssistantResponse with the human question
  // that came before the assistant turn we're regenerating
  EXPECT_CALL(*engine,
              GenerateAssistantResponse(_, LastTurnHasText(history[0]->text),
                                        StrEq(""), _, _, _, _, _, _))
      .WillOnce(::testing::DoAll(
          base::test::RunOnceCallback<7>(EngineConsumer::GenerationResultData(
              mojom::ConversationEntryEvent::NewCompletionEvent(
                  mojom::CompletionEvent::New("regenerated answer")),
              new_model_key)),
          base::test::RunOnceCallback<8>(
              base::ok(EngineConsumer::GenerationResultData(
                  mojom::ConversationEntryEvent::NewCompletionEvent(
                      mojom::CompletionEvent::New("")),
                  new_model_key)))));

  // Verify all four entries are removed (the target assistant turn and all
  // turns after it)
  EXPECT_CALL(observer, OnConversationEntryRemoved(conversation_handler_.get(),
                                                   human_turn_uuid))
      .Times(1);
  EXPECT_CALL(observer, OnConversationEntryRemoved(conversation_handler_.get(),
                                                   assistant_turn_uuid))
      .Times(1);
  EXPECT_CALL(observer, OnConversationEntryRemoved(conversation_handler_.get(),
                                                   history[2]->uuid.value()))
      .Times(1);
  EXPECT_CALL(observer, OnConversationEntryRemoved(conversation_handler_.get(),
                                                   history[3]->uuid.value()))
      .Times(1);

  // Verify the human question and new assistant answer are added back
  EXPECT_CALL(observer,
              OnConversationEntryAdded(conversation_handler_.get(), _, _))
      .Times(2);

  // Call RegenerateAnswer with the assistant turn UUID and new model key
  conversation_handler_->RegenerateAnswer(assistant_turn_uuid, new_model_key);

  // Add a RunLoop to wait for async operations to complete
  base::RunLoop run_loop;
  EXPECT_CALL(client, OnAPIRequestInProgress(false))
      .WillOnce(testing::InvokeWithoutArgs(&run_loop, &base::RunLoop::Quit));
  run_loop.Run();

  testing::Mock::VerifyAndClearExpectations(&client);
  testing::Mock::VerifyAndClearExpectations(&observer);
  testing::Mock::VerifyAndClearExpectations(engine);

  // Get the updated conversation history
  const auto& conversation_history =
      conversation_handler_->GetConversationHistory();

  // Verify the conversation has only 2 entries now (original question + new
  // answer)
  EXPECT_EQ(conversation_history.size(), 2u);

  // Verify the human entry model_key was set correctly
  EXPECT_EQ(conversation_history[0]->model_key.value_or(""), new_model_key);

  // Verify the assistant entry has the new model_key
  EXPECT_EQ(conversation_history[1]->model_key.value(), new_model_key);

  // Verify the answer content
  EXPECT_EQ(conversation_history[1]->text, "regenerated answer");
}

TEST_F(ConversationHandlerUnitTest, RegenerateAnswer_ErrorCases) {
  conversation_handler_->MaybeUnlinkAssociatedContent();

  MockEngineConsumer* engine = static_cast<MockEngineConsumer*>(
      conversation_handler_->GetEngineForTesting());

  // Setup history with 4 turns: human1, assistant1, human2, assistant2
  std::vector<mojom::ConversationTurnPtr> history = CreateSampleChatHistory(2);
  conversation_handler_->SetChatHistoryForTesting(CloneHistory(history));

  const std::string assistant_turn_uuid = history[1]->uuid.value();
  const std::string human_turn_uuid = history[0]->uuid.value();
  const std::string new_model_key = "new_model_key";

  NiceMock<MockConversationHandlerObserver> observer;
  NiceMock<MockConversationHandlerClient> client(conversation_handler_.get());
  observer.Observe(conversation_handler_.get());

  EXPECT_CALL(observer, OnConversationEntryRemoved).Times(0);
  EXPECT_CALL(*engine, GenerateAssistantResponse).Times(0);
  EXPECT_CALL(client, OnAPIRequestInProgress).Times(0);
  EXPECT_CALL(client, OnConversationHistoryUpdate).Times(0);

  // Verify edge cases
  // Invalid UUID should not modify history
  conversation_handler_->RegenerateAnswer("invalid_uuid", new_model_key);
  EXPECT_EQ(conversation_handler_->GetConversationHistory(), history);

  // Can't regenerate a human entry
  conversation_handler_->RegenerateAnswer(human_turn_uuid, new_model_key);
  EXPECT_EQ(conversation_handler_->GetConversationHistory(), history);

  // Test regenerating a conversation with just a single assistant entry
  std::vector<mojom::ConversationTurnPtr> single_entry_history;
  single_entry_history.push_back(mojom::ConversationTurn::New(
      "assistant_uuid", mojom::CharacterType::ASSISTANT,
      mojom::ActionType::RESPONSE, "original answer", std::nullopt,
      std::nullopt, std::nullopt, base::Time::Now(), std::nullopt, std::nullopt,
      nullptr /* skill */, false, std::nullopt /* model_key */));

  conversation_handler_->SetChatHistoryForTesting(
      CloneHistory(single_entry_history));

  // Should not regenerate an assistant entry at position 0 (no human question
  // to use)
  conversation_handler_->RegenerateAnswer("assistant_uuid", new_model_key);
  EXPECT_EQ(conversation_handler_->GetConversationHistory(),
            single_entry_history);
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
  EXPECT_CALL(client, OnConversationHistoryUpdate(
                          TurnEq(mojom::ConversationTurnPtr().get())))
      .Times(2);

  conversation_handler_->GetAssociatedContentInfo(base::BindLambdaForTesting(
      [&](std::vector<mojom::AssociatedContentPtr> site_info) {
        EXPECT_FALSE(site_info.empty());
      }));

  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(associated_content_.get());
  testing::Mock::VerifyAndClearExpectations(&observer);
  testing::Mock::VerifyAndClearExpectations(&client);

  auto& history = conversation_handler_->GetConversationHistory();
  std::vector<mojom::ConversationTurnPtr> expected_history;
  expected_history.push_back(mojom::ConversationTurn::New(
      "turn-1", mojom::CharacterType::HUMAN, mojom::ActionType::QUERY, "query",
      std::nullopt, std::nullopt, std::nullopt, base::Time::Now(), std::nullopt,
      std::nullopt, nullptr /* skill */, true, std::nullopt /* model_key */));
  std::vector<mojom::ConversationEntryEventPtr> events;
  events.push_back(mojom::ConversationEntryEvent::NewCompletionEvent(
      mojom::CompletionEvent::New("summary")));
  expected_history.push_back(mojom::ConversationTurn::New(
      "turn-2", mojom::CharacterType::ASSISTANT, mojom::ActionType::RESPONSE,
      "summary", std::nullopt, std::nullopt, std::move(events),
      base::Time::Now(), std::nullopt, std::nullopt, nullptr /* skill */, true,
      std::nullopt /* model_key */));
  ASSERT_EQ(history.size(), expected_history.size());
  for (size_t i = 0; i < history.size(); i++) {
    expected_history[i]->created_time = history[i]->created_time;
    ExpectConversationEntryEquals(FROM_HERE, history[i], expected_history[i],
                                  false);
  }
  // HasAnyHistory should still return false since all entries are staged
  EXPECT_FALSE(conversation_handler_->HasAnyHistory());

  // Verify turning off content association clears the conversation history.
  EXPECT_CALL(client, OnConversationHistoryUpdate(
                          TurnEq(mojom::ConversationTurnPtr().get())))
      .Times(1);
  // Shouldn't ask for staged entries if user doesn't want to be associated
  // with content. This verifies that even with existing staged entries,
  // MaybeFetchOrClearContentStagedConversation will always early return.
  EXPECT_CALL(*associated_content_, GetStagedEntriesFromContent).Times(0);

  conversation_handler_->associated_content_manager()->ClearContent();

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
  EXPECT_CALL(client, OnConversationHistoryUpdate(
                          TurnEq(mojom::ConversationTurnPtr().get())))
      .Times(testing::AtLeast(1));
  EXPECT_TRUE(conversation_handler_->IsAnyClientConnected());
  conversation_handler_->GetAssociatedContentInfo(base::BindLambdaForTesting(
      [&](std::vector<mojom::AssociatedContentPtr> site_info) {
        EXPECT_FALSE(site_info.empty());
      }));

  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(associated_content_.get());
  testing::Mock::VerifyAndClearExpectations(&observer);
  testing::Mock::VerifyAndClearExpectations(&client);

  auto& history = conversation_handler_->GetConversationHistory();
  std::vector<mojom::ConversationTurnPtr> expected_history;
  expected_history.push_back(mojom::ConversationTurn::New(
      std::nullopt, mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
      "query", std::nullopt, std::nullopt, std::nullopt, base::Time::Now(),
      std::nullopt, std::nullopt, nullptr /* skill */, true,
      std::nullopt /* model_key */));
  std::vector<mojom::ConversationEntryEventPtr> events;
  events.push_back(mojom::ConversationEntryEvent::NewCompletionEvent(
      mojom::CompletionEvent::New("summary")));
  expected_history.push_back(mojom::ConversationTurn::New(
      std::nullopt, mojom::CharacterType::ASSISTANT,
      mojom::ActionType::RESPONSE, "summary", std::nullopt, std::nullopt,
      std::move(events), base::Time::Now(), std::nullopt, std::nullopt,
      nullptr /* skill */, true, std::nullopt /* model_key */));

  expected_history.push_back(mojom::ConversationTurn::New(
      std::nullopt, mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
      "query2", std::nullopt, std::nullopt, std::nullopt, base::Time::Now(),
      std::nullopt, std::nullopt, nullptr /* skill */, true,
      std::nullopt /* model_key */));
  std::vector<mojom::ConversationEntryEventPtr> events2;
  events2.push_back(mojom::ConversationEntryEvent::NewCompletionEvent(
      mojom::CompletionEvent::New("summary2")));
  expected_history.push_back(mojom::ConversationTurn::New(
      std::nullopt, mojom::CharacterType::ASSISTANT,
      mojom::ActionType::RESPONSE, "summary2", std::nullopt, std::nullopt,
      std::move(events2), base::Time::Now(), std::nullopt, std::nullopt,
      nullptr /* skill */, true, std::nullopt /* model_key */));

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
  associated_content_->SetTextContent("page content");
  EXPECT_CALL(*engine, GenerateAssistantResponse)
      // Mock the response from the engine
      .WillOnce(::testing::DoAll(
          base::test::RunOnceCallback<7>(EngineConsumer::GenerationResultData(
              mojom::ConversationEntryEvent::NewCompletionEvent(
                  mojom::CompletionEvent::New("new answer")),
              std::nullopt /* model_key */)),
          base::test::RunOnceCallback<8>(
              base::ok(EngineConsumer::GenerationResultData(
                  mojom::ConversationEntryEvent::NewCompletionEvent(
                      mojom::CompletionEvent::New("")),
                  std::nullopt /* model_key */)))));

  EXPECT_CALL(observer, OnConversationEntryAdded).Times(6);
  EXPECT_CALL(client, OnConversationHistoryUpdate(
                          TurnEq(mojom::ConversationTurnPtr().get())))
      .Times(testing::AtLeast(1));
  std::vector<mojom::ConversationEntryEventPtr> events3;
  events3.push_back(mojom::ConversationEntryEvent::NewCompletionEvent(
      mojom::CompletionEvent::New("new answer")));
  auto expected_turn = mojom::ConversationTurn::New(
      std::nullopt, mojom::CharacterType::ASSISTANT,
      mojom::ActionType::RESPONSE, "new answer", std::nullopt, std::nullopt,
      std::move(events3), base::Time::Now(), std::nullopt, std::nullopt,
      nullptr /* skill */, false, std::nullopt /* model_key */);
  EXPECT_CALL(client, OnConversationHistoryUpdate(TurnEq(expected_turn.get())))
      .Times(testing::AtLeast(2));

  conversation_handler_->SubmitHumanConversationEntry("query3", std::nullopt);

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
  EXPECT_CALL(client, OnConversationHistoryUpdate(_)).Times(0);
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

  // Fill history with staged and non-staged entries.
  auto expected_history =
      SetupHistory({{"old query" /* text */, true /*from_brave_search_SERP */},
                    {"old summary", "true"},
                    {"normal query", false},
                    {"normal response", false}});
  // 4 from SetupHistory and 4 from adding
  // new entries in OnGetStagedEntriesFromContent.
  EXPECT_CALL(client, OnConversationHistoryUpdate(
                          TurnEq(mojom::ConversationTurnPtr().get())))
      .Times(6);
  EXPECT_CALL(client,
              OnConversationHistoryUpdate(TurnEq(expected_history[2].get())))
      .Times(1);
  EXPECT_CALL(client,
              OnConversationHistoryUpdate(TurnEq(expected_history[3].get())))
      .Times(1);

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
  conversation_handler_->MaybeUnlinkAssociatedContent();
  conversation_handler_->OnGetStagedEntriesFromContent(entries);
  task_environment_.RunUntilIdle();
  EXPECT_EQ(conversation_handler_->GetConversationHistory().size(), 0u);
}

TEST_F(ConversationHandlerUnitTest, OnGetStagedEntriesFromContent) {
  NiceMock<MockConversationHandlerClient> client(conversation_handler_.get());
  ASSERT_TRUE(conversation_handler_->IsAnyClientConnected());

  // Fill history with staged and non-staged entries.
  auto expected_history =
      SetupHistory({{"q1" /* text */, true /*from_brave_search_SERP */},
                    {"s1", "true"},
                    {"q2", false},
                    {"r1", false}});
  EXPECT_CALL(client, OnConversationHistoryUpdate(
                          TurnEq(mojom::ConversationTurnPtr().get())))
      .Times(6);
  EXPECT_CALL(client,
              OnConversationHistoryUpdate(TurnEq(expected_history[2].get())))
      .Times(1);
  EXPECT_CALL(client,
              OnConversationHistoryUpdate(TurnEq(expected_history[3].get())))
      .Times(1);

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
      [&](std::vector<mojom::AssociatedContentPtr> site_info) {
        EXPECT_FALSE(site_info.empty());
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
  conversation_handler_->MaybeUnlinkAssociatedContent();
  conversation_handler_->GetAssociatedContentInfo(base::BindLambdaForTesting(
      [&](std::vector<mojom::AssociatedContentPtr> site_info) {
        EXPECT_TRUE(site_info.empty());
      }));

  // Client connecting will trigger content staging
  NiceMock<MockConversationHandlerClient> client(conversation_handler_.get());
  EXPECT_CALL(client, OnConversationHistoryUpdate(_)).Times(0);
  EXPECT_TRUE(conversation_handler_->IsAnyClientConnected());

  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(conversation_handler_.get());
  testing::Mock::VerifyAndClearExpectations(&client);

  EXPECT_TRUE(conversation_handler_->GetConversationHistory().empty());
}

TEST_F(ConversationHandlerUnitTest, UploadFile) {
  conversation_handler_->MaybeUnlinkAssociatedContent();

  // Switch to a model without vision support.
  base::RunLoop loop_for_change_model;
  NiceMock<MockConversationHandlerClient> client(conversation_handler_.get());
  EXPECT_CALL(client, OnModelDataChanged)
      .WillOnce(testing::InvokeWithoutArgs(&loop_for_change_model,
                                           &base::RunLoop::Quit));
  conversation_handler_->ChangeModel("chat-basic");
  loop_for_change_model.Run();
  testing::Mock::VerifyAndClearExpectations(&client);

  // Re-setting a mock engine because it was replaced due to ChangeModel call.
  conversation_handler_->SetEngineForTesting(
      std::make_unique<NiceMock<MockEngineConsumer>>());
  MockEngineConsumer* engine = static_cast<MockEngineConsumer*>(
      conversation_handler_->GetEngineForTesting());

  constexpr char kTestPrompt[] = "What is this?";
  EXPECT_CALL(*engine, GenerateAssistantResponse)
      .WillRepeatedly(
          [](PageContentsMap page_contents,
             const std::vector<mojom::ConversationTurnPtr>& history,
             const std::string& selected_language, bool is_temporary_chat,
             const std::vector<base::WeakPtr<Tool>>& tools,
             std::optional<std::string_view> preferred_tool_name,
             mojom::ConversationCapability conversation_capability,
             EngineConsumer::GenerationDataCallback callback,
             EngineConsumer::GenerationCompletedCallback done_callback) {
            std::move(done_callback)
                .Run(base::ok(EngineConsumer::GenerationResultData(
                    mojom::ConversationEntryEvent::NewCompletionEvent(
                        mojom::CompletionEvent::New("This is a lion.")),
                    std::nullopt /* model_key */)));
          });
  ASSERT_FALSE(conversation_handler_->GetCurrentModel().vision_support);

  // No uploaded files
  base::RunLoop loop;
  EXPECT_CALL(client, OnModelDataChanged).Times(0);
  EXPECT_CALL(client, OnAPIRequestInProgress(true)).Times(1);
  EXPECT_CALL(client, OnAPIRequestInProgress(false))
      .WillOnce(testing::InvokeWithoutArgs(&loop, &base::RunLoop::Quit));
  conversation_handler_->SubmitHumanConversationEntry(kTestPrompt,
                                                      std::nullopt);
  loop.Run();
  EXPECT_FALSE(
      conversation_handler_->GetConversationHistory().back()->uploaded_files);
  testing::Mock::VerifyAndClearExpectations(&client);

  // Empty uploaded files
  base::RunLoop loop2;
  EXPECT_CALL(client, OnModelDataChanged).Times(0);
  EXPECT_CALL(client, OnAPIRequestInProgress(true)).Times(1);
  EXPECT_CALL(client, OnAPIRequestInProgress(false))
      .WillOnce(testing::InvokeWithoutArgs(&loop2, &base::RunLoop::Quit));
  conversation_handler_->SubmitHumanConversationEntry(
      kTestPrompt, std::vector<mojom::UploadedFilePtr>());
  loop2.Run();
  EXPECT_FALSE(
      conversation_handler_->GetConversationHistory().back()->uploaded_files);
  testing::Mock::VerifyAndClearExpectations(&client);

  // Create files for each UploadedFileType to exhaustively test all types
  auto uploaded_files = std::vector<mojom::UploadedFilePtr>();
  for (int type_int = base::to_underlying(mojom::UploadedFileType::kMinValue);
       type_int <= base::to_underlying(mojom::UploadedFileType::kMaxValue);
       ++type_int) {
    auto type = static_cast<mojom::UploadedFileType>(type_int);
    auto files = CreateSampleUploadedFiles(1, type);
    uploaded_files.insert(uploaded_files.end(),
                          std::make_move_iterator(files.begin()),
                          std::make_move_iterator(files.end()));
  }

  // There are uploaded images.
  // Note that this will need to be put at the end of this test suite
  // because currently there is no perfect timing to call
  // SetEngineForTesting() after auto model switch.
  base::RunLoop loop3;
  if (std::ranges::any_of(
          uploaded_files, [](const mojom::UploadedFilePtr& file) {
            return file->type == mojom::UploadedFileType::kImage ||
                   file->type == mojom::UploadedFileType::kScreenshot;
          })) {
    EXPECT_CALL(client, OnModelDataChanged)
        .WillOnce(base::test::RunClosure(base::BindLambdaForTesting([&]() {
          // verify auto switched to vision support model
          EXPECT_TRUE(conversation_handler_->GetCurrentModel().vision_support);
          loop3.Quit();
        })));
  } else {
    EXPECT_CALL(client, OnAPIRequestInProgress(false))
        .WillOnce(testing::InvokeWithoutArgs(&loop3, &base::RunLoop::Quit));
  }

  conversation_handler_->SubmitHumanConversationEntry(kTestPrompt,
                                                      Clone(uploaded_files));
  loop3.Run();
  testing::Mock::VerifyAndClearExpectations(&client);
  // verify image in history
  auto& last_entry = conversation_handler_->GetConversationHistory().back();
  EXPECT_TRUE(last_entry->uploaded_files);
  const auto& files = last_entry->uploaded_files.value();
  for (size_t i = 0; i < files.size(); ++i) {
    EXPECT_EQ(files[i]->filename, uploaded_files[i]->filename);
    EXPECT_EQ(files[i]->filesize, uploaded_files[i]->filesize);
    EXPECT_EQ(files[i]->data, uploaded_files[i]->data);
    EXPECT_EQ(files[i]->type, uploaded_files[i]->type);
  }
}

TEST_F(ConversationHandlerUnitTest_NoAssociatedContent,
       MaybeFetchOrClearSearchQuerySummary) {
  // Ensure nothing gets staged when there's no associated content.
  conversation_handler_->GetAssociatedContentInfo(base::BindLambdaForTesting(
      [&](std::vector<mojom::AssociatedContentPtr> site_info) {
        EXPECT_TRUE(site_info.empty());
      }));
  // Client connecting would trigger content staging
  NiceMock<MockConversationHandlerClient> client(conversation_handler_.get());
  EXPECT_CALL(client, OnConversationHistoryUpdate(_)).Times(0);
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
  conversation_handler_->MaybeUnlinkAssociatedContent();
  conversation_handler_->associated_content_manager()->AddContent(
      associated_content_.get());

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
  std::ranges::copy(questions, std::back_inserter(expected_results));

  EXPECT_TRUE(conversation_handler_->associated_content_manager()
                  ->HasAssociatedContent());
  associated_content_->SetUrl(GURL("https://www.example.com"));
  associated_content_->SetTextContent(page_content);

  // Mock engine response
  MockEngineConsumer* engine = static_cast<MockEngineConsumer*>(
      conversation_handler_->GetEngineForTesting());
  EXPECT_CALL(*engine, GenerateQuestionSuggestions(_, StrEq(""), _))
      .WillOnce(base::test::RunOnceCallback<2>(questions));

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
      .Times(testing::AtLeast(1))
      .InSequence(s);
  conversation_handler_->GenerateQuestions();
  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&client);
  testing::Mock::VerifyAndClearExpectations(associated_content_.get());
  testing::Mock::VerifyAndClearExpectations(engine);
}

TEST_F(ConversationHandlerUnitTest,
       MaybeSeedOrClearSuggestions_UpdatesWithAssociatedContentType) {
  associated_content_->SetUrl(GURL("https://www.example.com/"));
  associated_content_->SetTextContent("Content");
  associated_content_->SetIsVideo(true);

  base::RunLoop loop1;
  conversation_handler_->associated_content_manager()->GetContent(
      loop1.QuitClosure());
  loop1.Run();

  conversation_handler_->OnAssociatedContentUpdated();

  const auto& suggestions =
      conversation_handler_->GetSuggestedQuestionsForTest();
  ASSERT_EQ(suggestions.size(), 1u);
  EXPECT_EQ(suggestions[0].action_type, mojom::ActionType::SUMMARIZE_VIDEO);
  EXPECT_EQ(suggestions[0].title,
            l10n_util::GetStringUTF8(IDS_CHAT_UI_SUMMARIZE_VIDEO));
  EXPECT_EQ(suggestions[0].prompt,
            l10n_util::GetStringUTF8(IDS_AI_CHAT_QUESTION_SUMMARIZE_VIDEO));

  associated_content_->SetIsVideo(false);

  base::RunLoop loop2;
  conversation_handler_->associated_content_manager()->GetContent(
      loop2.QuitClosure());
  loop2.Run();
  conversation_handler_->OnAssociatedContentUpdated();

  const auto& suggestions2 =
      conversation_handler_->GetSuggestedQuestionsForTest();
  ASSERT_EQ(suggestions2.size(), 1u);
  EXPECT_EQ(suggestions2[0].action_type, mojom::ActionType::SUMMARIZE_PAGE);
  EXPECT_EQ(suggestions2[0].title,
            l10n_util::GetPluralStringFUTF8(
                IDS_CHAT_UI_SUMMARIZE_PAGES_SUGGESTION, 1));
  EXPECT_EQ(suggestions2[0].prompt,
            l10n_util::GetStringUTF8(IDS_AI_CHAT_QUESTION_SUMMARIZE_PAGE));
}

TEST_F(ConversationHandlerUnitTest, SubmitSuggestion) {
  // Test suggestion removal with associated content because ConversationHandler
  // removes all suggestions after the first query when there is no associated
  // content. When there is associated content, only the submitted suggestion
  // should be removed.
  associated_content_->SetUrl(GURL("https://www.example.com"));
  associated_content_->SetTextContent("content");

  MockEngineConsumer* engine = static_cast<MockEngineConsumer*>(
      conversation_handler_->GetEngineForTesting());

  const std::vector<std::string> questions = {"Question 1?", "Question 2?",
                                              "Question 3?", "Question 4?"};
  base::RunLoop run_loop;
  // ConversationHandler requires a client to be connected when generating
  // questions.
  NiceMock<MockConversationHandlerClient> client(conversation_handler_.get());
  // Respond with questions and quit run_lop
  EXPECT_CALL(*engine, GenerateQuestionSuggestions(_, _, _))
      .WillOnce(testing::WithArg<2>(
          [&](EngineConsumer::SuggestedQuestionsCallback callback) {
            std::move(callback).Run(questions);
            run_loop.Quit();
          }));

  conversation_handler_->GenerateQuestions();
  run_loop.Run();

  const auto& suggestions1 =
      conversation_handler_->GetSuggestedQuestionsForTest();
  EXPECT_EQ(5u, suggestions1.size());

  conversation_handler_->SubmitSuggestion("Question 2?");

  const auto& suggestions2 =
      conversation_handler_->GetSuggestedQuestionsForTest();

  // Submitted suggestion only should be removed
  EXPECT_EQ(4u, suggestions2.size());
  auto match_it = std::ranges::find_if(
      suggestions2, [](const auto& s) { return s.title == "Question 2?"; });
  EXPECT_EQ(match_it, suggestions2.end())
      << "Question 2? should not be found in suggestions2";

  // Generated conversation entry should have suggestion action type
  auto& history = conversation_handler_->GetConversationHistory();
  EXPECT_EQ(1u, history.size());
  EXPECT_EQ(mojom::ActionType::SUGGESTION, history[0]->action_type);
}

TEST_F(ConversationHandlerUnitTest, GenerateQuestions_DisableSendPageContent) {
  conversation_handler_->MaybeUnlinkAssociatedContent();
  conversation_handler_->GetAssociatedContentInfo(base::BindLambdaForTesting(
      [&](std::vector<mojom::AssociatedContentPtr> site_info) {
        EXPECT_TRUE(site_info.empty());
      }));
  associated_content_->SetUrl(GURL("https://www.example.com"));
  associated_content_->SetTextContent("content");

  // Mock engine response
  MockEngineConsumer* engine = static_cast<MockEngineConsumer*>(
      conversation_handler_->GetEngineForTesting());
  EXPECT_CALL(*engine, GenerateQuestionSuggestions(_, _, _)).Times(0);

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
  EXPECT_CALL(*engine, GenerateQuestionSuggestions(_, _, _)).Times(0);

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
  // A conversation not associated with content should have conversation
  // starter suggestions.
  const auto& suggestions1 =
      conversation_handler_->GetSuggestedQuestionsForTest();
  EXPECT_EQ(4u, suggestions1.size());

  auto submitted_suggestion = suggestions1[1].title;

  conversation_handler_->SubmitSuggestion(submitted_suggestion);
  const auto& suggestions2 =
      conversation_handler_->GetSuggestedQuestionsForTest();

  // All suggestions should be removed
  EXPECT_EQ(0u, suggestions2.size());

  auto& history = conversation_handler_->GetConversationHistory();
  EXPECT_EQ(1u, history.size());
  auto& history_entry = history[0];

  // Generated conversation entry should have conversation starter action type
  EXPECT_EQ(mojom::ActionType::CONVERSATION_STARTER,
            history_entry->action_type);
  // Prompt should be different
  EXPECT_EQ(history_entry->text, submitted_suggestion);
  EXPECT_TRUE(history_entry->prompt.has_value());
  EXPECT_NE(history_entry->prompt, submitted_suggestion);
}

TEST_F(ConversationHandlerUnitTest_NoAssociatedContent,
       SelectingDefaultQuestionSendsPrompt) {
  // Suggested question which has a different prompt and title
  conversation_handler_->SetSuggestedQuestionForTest("the thing",
                                                     "do the thing!");
  const auto& suggestions =
      conversation_handler_->GetSuggestedQuestionsForTest();
  EXPECT_EQ(1u, suggestions.size());

  // Mock engine response
  MockEngineConsumer* engine = static_cast<MockEngineConsumer*>(
      conversation_handler_->GetEngineForTesting());

  base::RunLoop loop;
  // The prompt should be submitted to the engine, not the title.
  EXPECT_CALL(*engine,
              GenerateAssistantResponse(_, LastTurnHasText("do the thing!"),
                                        StrEq(""), _, _, _, _, _, _))
      .WillOnce(testing::InvokeWithoutArgs(&loop, &base::RunLoop::Quit));

  conversation_handler_->SubmitSuggestion("the thing");
  loop.Run();
  testing::Mock::VerifyAndClearExpectations(engine);

  // Suggestion should be removed
  EXPECT_EQ(0u, conversation_handler_->GetSuggestedQuestionsForTest().size());
}

TEST_F(ConversationHandlerUnitTest_NoAssociatedContent, SelectedLanguage) {
  MockEngineConsumer* engine = static_cast<MockEngineConsumer*>(
      conversation_handler_->GetEngineForTesting());
  NiceMock<MockConversationHandlerClient> client(conversation_handler_.get());

  std::string expected_input1 = "Now stand aside, worthy adversary.";
  std::string expected_input2 = "A scratch? Your arm's off!";
  std::string expected_selected_language = "fr";

  EXPECT_CALL(*engine,
              GenerateAssistantResponse(_, LastTurnHasText(expected_input1),
                                        StrEq(""), _, _, _, _, _, _))
      .Times(1)
      .WillOnce(::testing::DoAll(
          base::test::RunOnceCallback<7>(EngineConsumer::GenerationResultData(
              mojom::ConversationEntryEvent::NewCompletionEvent(
                  mojom::CompletionEvent::New("Tis but a scratch.")),
              std::nullopt /* model_key */)),
          base::test::RunOnceCallback<7>(EngineConsumer::GenerationResultData(
              mojom::ConversationEntryEvent::NewSelectedLanguageEvent(
                  mojom::SelectedLanguageEvent::New(
                      expected_selected_language)),
              std::nullopt /* model_key */)),
          base::test::RunOnceCallback<8>(
              base::ok(EngineConsumer::GenerationResultData(
                  mojom::ConversationEntryEvent::NewCompletionEvent(
                      mojom::CompletionEvent::New("")),
                  std::nullopt /* model_key */)))));

  EXPECT_CALL(client, OnAPIRequestInProgress(true)).Times(testing::AtLeast(1));

  base::RunLoop loop;
  EXPECT_CALL(client, OnAPIRequestInProgress(false))
      .WillOnce(testing::InvokeWithoutArgs(&loop, &base::RunLoop::Quit));

  conversation_handler_->SubmitHumanConversationEntry(expected_input1,
                                                      std::nullopt);

  loop.Run();

  EXPECT_CALL(*engine, GenerateAssistantResponse(
                           _, LastTurnHasText(expected_input2),
                           StrEq(expected_selected_language), _, _, _, _, _, _))
      .WillOnce(::testing::DoAll(
          base::test::RunOnceCallback<7>(EngineConsumer::GenerationResultData(
              mojom::ConversationEntryEvent::NewCompletionEvent(
                  mojom::CompletionEvent::New("No, it isn't.")),
              std::nullopt /* model_key */)),
          base::test::RunOnceCallback<8>(
              base::ok(EngineConsumer::GenerationResultData(
                  mojom::ConversationEntryEvent::NewCompletionEvent(
                      mojom::CompletionEvent::New("")),
                  std::nullopt /* model_key */)))));

  base::RunLoop loop2;
  EXPECT_CALL(client, OnAPIRequestInProgress(false))
      .WillOnce(testing::InvokeWithoutArgs(&loop2, &base::RunLoop::Quit));

  conversation_handler_->SubmitHumanConversationEntry(expected_input2,
                                                      std::nullopt);
  loop2.Run();

  // Selected Language events should not be added to the conversation events
  // history
  const auto& conversation_history =
      conversation_handler_->GetConversationHistory();
  ASSERT_FALSE(conversation_history.empty());
  bool has_selected_language_event =
      std::ranges::any_of(conversation_history, [](const auto& entry) {
        return entry->events.has_value() &&
               std::ranges::any_of(*entry->events, [](const auto& event) {
                 return event->is_selected_language_event();
               });
      });
  EXPECT_FALSE(has_selected_language_event)
      << "There is an 'is_selected_language_event' present.";

  // And internally the conversation handler should know the selected language
  // was set
  EXPECT_EQ(conversation_handler_->selected_language_,
            expected_selected_language);

  testing::Mock::VerifyAndClearExpectations(engine);
}

TEST_F(ConversationHandlerUnitTest_NoAssociatedContent, ContentReceipt) {
  MockEngineConsumer* engine = static_cast<MockEngineConsumer*>(
      conversation_handler_->GetEngineForTesting());
  NiceMock<MockConversationHandlerClient> client(conversation_handler_.get());
  testing::NiceMock<MockConversationHandlerObserver> observer;
  observer.Observe(conversation_handler_.get());

  auto delegate = std::make_unique<AssociatedArchiveContent>(
      GURL("https://example.com"), "This is the way - page contents",
      u"The way",
      /*is_video=*/false, "my-uuid");
  conversation_handler_->associated_content_manager()->AddContent(
      delegate.get(), /*notify_updated=*/true,
      /*detach_existing_content=*/true);

  std::string expected_input = "What is the way?";
  uint64_t expected_total_tokens = 1000;
  uint64_t expected_trimmed_tokens = 200;

  EXPECT_CALL(*engine,
              GenerateAssistantResponse(_, LastTurnHasText(expected_input),
                                        StrEq(""), _, _, _, _, _, _))
      .Times(1)
      .WillOnce(::testing::DoAll(
          base::test::RunOnceCallback<7>(EngineConsumer::GenerationResultData(
              mojom::ConversationEntryEvent::NewCompletionEvent(
                  mojom::CompletionEvent::New(
                      "That may be your way, but it's not mine.")),
              std::nullopt /* model_key */)),
          base::test::RunOnceCallback<7>(EngineConsumer::GenerationResultData(
              mojom::ConversationEntryEvent::NewContentReceiptEvent(
                  mojom::ContentReceiptEvent::New(expected_total_tokens,
                                                  expected_trimmed_tokens)),
              std::nullopt /* model_key */)),
          base::test::RunOnceCallback<8>(
              base::ok(EngineConsumer::GenerationResultData(
                  mojom::ConversationEntryEvent::NewCompletionEvent(
                      mojom::CompletionEvent::New("")),
                  std::nullopt /* model_key */)))));

  EXPECT_CALL(client, OnAPIRequestInProgress(true)).Times(testing::AtLeast(1));

  base::RunLoop loop;
  EXPECT_CALL(client, OnAPIRequestInProgress(false))
      .WillOnce(testing::InvokeWithoutArgs(&loop, &base::RunLoop::Quit));

  // OnConversationTokenInfoChanged should be called
  EXPECT_CALL(observer, OnConversationTokenInfoChanged(
                            conversation_handler_->metadata_->uuid,
                            expected_total_tokens, expected_trimmed_tokens))
      .Times(1);

  conversation_handler_->SubmitHumanConversationEntry(expected_input,
                                                      std::nullopt);

  loop.Run();

  // ContentReceipt events should not be added to the conversation events
  // history
  const auto& conversation_history =
      conversation_handler_->GetConversationHistory();
  ASSERT_FALSE(conversation_history.empty());
  bool has_content_receipt_event =
      std::ranges::any_of(conversation_history, [](const auto& entry) {
        return entry->events.has_value() &&
               std::ranges::any_of(*entry->events, [](const auto& event) {
                 return event->is_content_receipt_event();
               });
      });
  EXPECT_FALSE(has_content_receipt_event)
      << "There is an is_content_receipt_event present.";

  testing::Mock::VerifyAndClearExpectations(engine);

  // Remove content so we don't get a dangling pointer when we try and access
  // ArchiveContent during destruction.
  conversation_handler_->associated_content_manager()->ClearContent();
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

TEST_F(ConversationHandlerUnitTest, RateMessage) {
  // Create a sample chat history with 2 turns (human, assistant)
  std::vector<mojom::ConversationTurnPtr> history = CreateSampleChatHistory(1);
  ASSERT_EQ(history.size(), 2u);

  // Store UUIDs for easy access
  const std::string human_turn_uuid = history[0]->uuid.value();
  const std::string assistant_turn_uuid = history[1]->uuid.value();

  // Initialize the conversation handler with test history
  conversation_handler_->SetChatHistoryForTesting(CloneHistory(history));

  // Test when model_key is null (should use current model)
  {
    // Clear the model_key on the assistant turn
    conversation_handler_->GetConversationHistory().back()->model_key =
        std::nullopt;

    const std::string current_model_key =
        conversation_handler_->GetCurrentModel().key;
    auto model_name = model_service_->GetLeoModelNameByKey(current_model_key);
    ASSERT_TRUE(model_name);
    // Should use the current model from GetCurrentModel()
    EXPECT_CALL(*mock_feedback_api_,
                SendRating(true, false, _, *model_name, _, _))
        .WillOnce(
            [&](bool is_liked, bool is_premium,
                const base::span<const mojom::ConversationTurnPtr>&
                    history_span,
                const std::string& model_name,
                const std::string& selected_language,
                api_request_helper::APIRequestHelper::ResultCallback callback) {
              // Verify the history being sent contains the human and assistant
              // turns
              EXPECT_EQ(history_span.size(), 2u);

              // Create a mock response with an ID
              base::Value::Dict response_dict;
              response_dict.Set("id", "test-rating-current-model");
              base::Value response(std::move(response_dict));

              // Return the response via callback
              std::move(callback).Run(api_request_helper::APIRequestResult(
                  200,                  // response_code
                  std::move(response),  // value_body
                  {},                   // empty headers
                  net::OK,              // error_code
                  GURL()));             // empty final_url
            });

    // Call RateMessage
    base::test::TestFuture<const std::optional<std::string>&> future_rating_id;
    conversation_handler_->RateMessage(true, assistant_turn_uuid,
                                       future_rating_id.GetCallback());
    testing::Mock::VerifyAndClearExpectations(mock_feedback_api_.get());

    // Verify the rating ID was returned
    EXPECT_EQ(future_rating_id.Take(), "test-rating-current-model");
  }

  // Test with an invalid model_key that returns nullptr
  {
    // Set an invalid model_key on the assistant turn
    conversation_handler_->GetConversationHistory().back()->model_key =
        "non-existent-model";

    // Set expectations for the mock - SendRating should not be called
    EXPECT_CALL(*mock_feedback_api_, SendRating).Times(0);

    // Call RateMessage
    base::test::TestFuture<const std::optional<std::string>&> future_rating_id;
    conversation_handler_->RateMessage(true, assistant_turn_uuid,
                                       future_rating_id.GetCallback());
    testing::Mock::VerifyAndClearExpectations(mock_feedback_api_.get());

    // Verify no rating ID was returned for an invalid model
    EXPECT_FALSE(future_rating_id.Take().has_value());
  }

  // Test regular case with model_key present in turn
  {
    // Set the model_key for the assistant turn to be a "chat-basic" model
    conversation_handler_->GetConversationHistory().back()->model_key =
        "chat-basic";
    auto model_name = model_service_->GetLeoModelNameByKey("chat-basic");
    ASSERT_TRUE(model_name);
    EXPECT_CALL(*mock_feedback_api_,
                SendRating(true, false, _, *model_name, _, _))
        .WillOnce(
            [&](bool is_liked, bool is_premium,
                const base::span<const mojom::ConversationTurnPtr>&
                    history_span,
                const std::string& model_name,
                const std::string& selected_language,
                api_request_helper::APIRequestHelper::ResultCallback callback) {
              // Verify the history being sent contains the human and assistant
              // turns
              EXPECT_EQ(history_span.size(), 2u);

              // Create a mock response with an ID
              base::Value::Dict response_dict;
              response_dict.Set("id", "test-rating-123");
              base::Value response(std::move(response_dict));

              // Return the response via callback
              std::move(callback).Run(api_request_helper::APIRequestResult(
                  200,                  // response_code
                  std::move(response),  // value_body
                  {},                   // empty headers
                  net::OK,              // error_code
                  GURL()));             // empty final_url
            });

    // Call RateMessage with a like
    base::test::TestFuture<const std::optional<std::string>&> future_rating_id;
    conversation_handler_->RateMessage(true, assistant_turn_uuid,
                                       future_rating_id.GetCallback());
    testing::Mock::VerifyAndClearExpectations(mock_feedback_api_.get());

    // Verify the rating ID was returned
    EXPECT_EQ(future_rating_id.Take(), "test-rating-123");
  }

  // Test with an error response
  {
    EXPECT_CALL(*mock_feedback_api_, SendRating)
        .WillOnce(
            [&](bool is_liked, bool is_premium,
                const base::span<const mojom::ConversationTurnPtr>&
                    history_span,
                const std::string& model_name,
                const std::string& selected_language,
                api_request_helper::APIRequestHelper::ResultCallback callback) {
              // Return an error
              std::move(callback).Run(api_request_helper::APIRequestResult(
                  500,              // response_code
                  base::Value(),    // empty value_body
                  {},               // empty headers
                  net::ERR_FAILED,  // error_code
                  GURL()));         // empty final_url
            });

    // Call RateMessage
    base::test::TestFuture<const std::optional<std::string>&> future_rating_id;
    conversation_handler_->RateMessage(true, assistant_turn_uuid,
                                       future_rating_id.GetCallback());
    testing::Mock::VerifyAndClearExpectations(mock_feedback_api_.get());

    // Verify no rating ID was returned
    EXPECT_FALSE(future_rating_id.Take().has_value());
  }
}

TEST_F(ConversationHandlerUnitTest,
       SubmitHumanConversationEntry_NoNewEntrySubmitHuman) {
  conversation_handler_->associated_content_manager()->ClearContent();
  // Tests what happens when the engine returns a success but there was no new
  // entry. We should avoid re-adding the most recent entry.

  MockEngineConsumer* engine = static_cast<MockEngineConsumer*>(
      conversation_handler_->GetEngineForTesting());

  testing::NiceMock<MockConversationHandlerObserver> observer;
  observer.Observe(conversation_handler_.get());

  // We should only add a new entry for the human entry
  EXPECT_CALL(observer,
              OnConversationEntryAdded(_, TurnHasText("Test question"), _))
      .Times(1);

  // Mock engine to return no new entry
  base::RunLoop run_loop;
  EXPECT_CALL(*engine, GenerateAssistantResponse)
      .WillOnce(testing::DoAll(
          // Complete the request
          testing::WithArg<8>(
              [&](EngineConsumer::GenerationCompletedCallback callback) {
                std::move(callback).Run(
                    base::ok(EngineConsumer::GenerationResultData(
                        mojom::ConversationEntryEvent::NewCompletionEvent(
                            mojom::CompletionEvent::New("")),
                        std::nullopt)));
                run_loop.QuitWhenIdle();
              })));

  // Submit a human entry to trigger the mocked response
  conversation_handler_->SubmitHumanConversationEntry("Test question",
                                                      std::nullopt);
  run_loop.Run();

  // Verify the conversation history doens't have an extra entry
  const auto& current_history = conversation_handler_->GetConversationHistory();
  EXPECT_EQ(current_history.size(), 1u);

  EXPECT_EQ(conversation_handler_->current_error(),
            mojom::APIError::ConnectionIssue);
}

TEST_F(ConversationHandlerUnitTest,
       SubmitHumanConversationEntry_NoNewEntryToolUse) {
  conversation_handler_->associated_content_manager()->ClearContent();
  // Tests what happens when the engine returns a success but there was no new
  // entry after a tool use response.
  MockEngineConsumer* engine = static_cast<MockEngineConsumer*>(
      conversation_handler_->GetEngineForTesting());

  testing::NiceMock<MockConversationHandlerObserver> observer;
  observer.Observe(conversation_handler_.get());

  auto tool1 =
      std::make_unique<NiceMock<MockTool>>("weather_tool", "Get weather");
  tool1->set_requires_user_interaction_before_handling(false);
  ON_CALL(*mock_tool_provider_, GetTools()).WillByDefault([&]() {
    std::vector<base::WeakPtr<Tool>> tools;
    tools.push_back(tool1->GetWeakPtr());
    return tools;
  });

  bool tool_response_generation_started = false;

  // We should only add the initial human entry and the first assistant response
  // but no further human or assistant entries.
  // Verify it's never called after the tool response generation has started.
  EXPECT_CALL(observer, OnConversationEntryAdded)
      .Times(2)
      .WillRepeatedly(testing::InvokeWithoutArgs(
          [&]() { EXPECT_FALSE(tool_response_generation_started); }));

  base::RunLoop run_loop;
  testing::Sequence seq;

  // First call to engine mocks the use tool request
  EXPECT_CALL(*engine, GenerateAssistantResponse)
      .InSequence(seq)
      .WillOnce(testing::DoAll(
          testing::WithArg<7>(
              [](EngineConsumer::GenerationDataCallback callback) {
                callback.Run(EngineConsumer::GenerationResultData(
                    mojom::ConversationEntryEvent::NewCompletionEvent(
                        mojom::CompletionEvent::New("Ok, going to check...")),
                    std::nullopt));
              }),
          testing::WithArg<7>(
              [](EngineConsumer::GenerationDataCallback callback) {
                callback.Run(EngineConsumer::GenerationResultData(
                    mojom::ConversationEntryEvent::NewToolUseEvent(
                        mojom::ToolUseEvent::New("weather_tool", "tool_id_1",
                                                 "{\"location\":\"New York\"}",
                                                 std::nullopt, nullptr)),
                    std::nullopt));
              }),
          testing::WithArg<8>(
              [&](EngineConsumer::GenerationCompletedCallback callback) {
                std::move(callback).Run(
                    base::ok(EngineConsumer::GenerationResultData(
                        mojom::ConversationEntryEvent::NewCompletionEvent(
                            mojom::CompletionEvent::New("")),
                        std::nullopt)));
              })));

  EXPECT_CALL(*tool1, UseTool(StrEq("{\"location\":\"New York\"}"), _))
      .WillOnce(testing::WithArg<1>([&](Tool::UseToolCallback callback) {
        std::vector<mojom::ContentBlockPtr> result;
        result.push_back(mojom::ContentBlock::NewTextContentBlock(
            mojom::TextContentBlock::New("Weather in New York: 72°F")));
        std::move(callback).Run(std::move(result));
      }));

  // Mock engine to return no new entry after the tool is used
  EXPECT_CALL(*engine, GenerateAssistantResponse)
      .InSequence(seq)
      .WillOnce(
          // Complete the request
          testing::WithArg<8>(
              [&](EngineConsumer::GenerationCompletedCallback callback) {
                tool_response_generation_started = true;
                std::move(callback).Run(
                    base::ok(EngineConsumer::GenerationResultData(
                        mojom::ConversationEntryEvent::NewCompletionEvent(
                            mojom::CompletionEvent::New("")),
                        std::nullopt)));
                run_loop.Quit();
              }));

  // Submit a human entry to trigger the mocked response
  conversation_handler_->SubmitHumanConversationEntry("Test question",
                                                      std::nullopt);
  run_loop.Run();

  // Verify the conversation history doens't have an extra entry
  const auto& current_history = conversation_handler_->GetConversationHistory();
  EXPECT_EQ(current_history.size(), 2u);
  EXPECT_EQ(current_history[0]->character_type, mojom::CharacterType::HUMAN);
  EXPECT_EQ(current_history[1]->character_type,
            mojom::CharacterType::ASSISTANT);
  EXPECT_EQ(current_history[0]->text, "Test question");
  auto& response_events = current_history[1]->events.value();
  EXPECT_EQ(response_events.size(), 2u);
  EXPECT_EQ(response_events[0]->get_completion_event()->completion,
            "Ok, going to check...");
  EXPECT_EQ(response_events[1]->get_tool_use_event()->tool_name,
            "weather_tool");
  EXPECT_EQ(response_events[1]->get_tool_use_event()->arguments_json,
            "{\"location\":\"New York\"}");

  EXPECT_EQ(conversation_handler_->current_error(),
            mojom::APIError::ConnectionIssue);
}

TEST_F(ConversationHandlerUnitTest, GetTools_FiltersUnsupportedTools) {
  conversation_handler_->associated_content_manager()->ClearContent();
  ASSERT_FALSE(conversation_handler_->associated_content_manager()
                   ->HasAssociatedContent());

  auto tool1 =
      std::make_unique<NiceMock<MockTool>>("not_supported_by_model", "");
  auto tool2 = std::make_unique<NiceMock<MockTool>>("supported", "");
  auto tool3 =
      std::make_unique<NiceMock<MockTool>>("not_supports_conversation", "");

  tool1->set_is_supported_by_model(false);
  tool3->set_supports_conversation(false);

  ON_CALL(*mock_tool_provider_, GetTools()).WillByDefault([&]() {
    std::vector<base::WeakPtr<Tool>> tools;
    tools.push_back(tool1->GetWeakPtr());
    tools.push_back(tool2->GetWeakPtr());
    tools.push_back(tool3->GetWeakPtr());
    return tools;
  });

  auto tools = conversation_handler_->GetToolsForTesting();
  EXPECT_EQ(tools.size(), 1u);
  EXPECT_EQ(tools[0]->Name(), "supported");
}

TEST_F(ConversationHandlerUnitTest, ToolUseEvents_PartialEventsGetCombined) {
  conversation_handler_->associated_content_manager()->ClearContent();
  MockEngineConsumer* engine = static_cast<MockEngineConsumer*>(
      conversation_handler_->GetEngineForTesting());

  // Set up test tools that match the tool names used in the test
  auto tool1 = std::make_unique<NiceMock<MockTool>>("test_tool", "Test tool");
  auto tool2 =
      std::make_unique<NiceMock<MockTool>>("test_tool2", "Test tool 2");
  tool1->set_requires_user_interaction_before_handling(false);
  tool2->set_requires_user_interaction_before_handling(false);

  ON_CALL(*mock_tool_provider_, GetTools()).WillByDefault([&]() {
    std::vector<base::WeakPtr<Tool>> tools;
    tools.push_back(tool1->GetWeakPtr());
    tools.push_back(tool2->GetWeakPtr());
    return tools;
  });

  NiceMock<MockConversationHandlerClient> client(conversation_handler_.get());

  // Mock engine to return partial tool use events
  base::RunLoop run_loop;
  EXPECT_CALL(*engine, GenerateAssistantResponse)
      .WillOnce(testing::DoAll(
          // First send a tool use event with a name
          testing::WithArg<7>(
              [](EngineConsumer::GenerationDataCallback callback) {
                callback.Run(EngineConsumer::GenerationResultData(
                    mojom::ConversationEntryEvent::NewToolUseEvent(
                        mojom::ToolUseEvent::New("test_tool", "id1",
                                                 "{\"param\":", std::nullopt,
                                                 nullptr)),
                    std::nullopt));
              }),
          // Then send a partial tool use event with no name
          testing::WithArg<7>(
              [](EngineConsumer::GenerationDataCallback callback) {
                callback.Run(EngineConsumer::GenerationResultData(
                    mojom::ConversationEntryEvent::NewToolUseEvent(
                        mojom::ToolUseEvent::New("", "", "\"value\"}",
                                                 std::nullopt, nullptr)),
                    std::nullopt));
              }),
          // Then send another tool use event with a name
          testing::WithArg<7>(
              [](EngineConsumer::GenerationDataCallback callback) {
                callback.Run(EngineConsumer::GenerationResultData(
                    mojom::ConversationEntryEvent::NewToolUseEvent(
                        mojom::ToolUseEvent::New("test_tool2", "id2",
                                                 "{\"other\":true}",
                                                 std::nullopt, nullptr)),
                    std::nullopt));
              }),
          // Complete the request
          testing::WithArg<8>(
              [&](EngineConsumer::GenerationCompletedCallback callback) {
                std::move(callback).Run(
                    base::ok(EngineConsumer::GenerationResultData(
                        mojom::ConversationEntryEvent::NewCompletionEvent(
                            mojom::CompletionEvent::New("")),
                        std::nullopt)));
                run_loop.Quit();
              })));

  // Submit a human entry to trigger the mocked response
  conversation_handler_->SubmitHumanConversationEntry("Test question",
                                                      std::nullopt);
  run_loop.Run();

  // Verify the conversation history
  const std::vector<mojom::ConversationTurnPtr>& history =
      conversation_handler_->GetConversationHistory();
  EXPECT_EQ(history.size(), 2u);

  auto& assistant_entry = history.back();
  ASSERT_TRUE(assistant_entry->events.has_value());
  auto& events = assistant_entry->events.value();
  EXPECT_EQ(events.size(), 2u);  // combined event + separate event

  // First event should have combined arguments
  EXPECT_TRUE(events[0]->is_tool_use_event());
  EXPECT_EQ(events[0]->get_tool_use_event()->tool_name, "test_tool");
  EXPECT_EQ(events[0]->get_tool_use_event()->arguments_json,
            "{\"param\":\"value\"}");

  // Second event should be separate
  EXPECT_TRUE(events[1]->is_tool_use_event());
  EXPECT_EQ(events[1]->get_tool_use_event()->tool_name, "test_tool2");
  EXPECT_EQ(events[1]->get_tool_use_event()->arguments_json,
            "{\"other\":true}");
}

TEST_F(ConversationHandlerUnitTest, ToolUseEvents_CorrectToolCalled) {
  conversation_handler_->associated_content_manager()->ClearContent();
  // Setup multiple tools with only 1 being called
  auto tool1 =
      std::make_unique<NiceMock<MockTool>>("weather_tool", "Get weather");
  auto tool2 = std::make_unique<NiceMock<MockTool>>("calculator", "Do math");

  tool1->set_requires_user_interaction_before_handling(false);
  tool2->set_requires_user_interaction_before_handling(false);

  ON_CALL(*mock_tool_provider_, GetTools()).WillByDefault([&]() {
    std::vector<base::WeakPtr<Tool>> tools;
    tools.push_back(tool1->GetWeakPtr());
    tools.push_back(tool2->GetWeakPtr());
    return tools;
  });

  MockEngineConsumer* engine = static_cast<MockEngineConsumer*>(
      conversation_handler_->GetEngineForTesting());

  NiceMock<MockConversationHandlerClient> client(conversation_handler_.get());
  NiceMock<MockUntrustedConversationHandlerClient> untrusted_client(
      conversation_handler_.get());
  testing::NiceMock<MockConversationHandlerObserver> observer;
  observer.Observe(conversation_handler_.get());

  base::RunLoop run_loop;
  testing::Sequence seq;
  bool second_generation_started = false;

  // First call to engine mocks the use tool requests
  EXPECT_CALL(*engine, GenerateAssistantResponse)
      .InSequence(seq)
      .WillOnce(testing::DoAll(
          testing::WithArg<7>(
              [](EngineConsumer::GenerationDataCallback callback) {
                callback.Run(EngineConsumer::GenerationResultData(
                    mojom::ConversationEntryEvent::NewCompletionEvent(
                        mojom::CompletionEvent::New("Ok, going to check...")),
                    std::nullopt));
              }),
          testing::WithArg<7>(
              [](EngineConsumer::GenerationDataCallback callback) {
                callback.Run(EngineConsumer::GenerationResultData(
                    mojom::ConversationEntryEvent::NewToolUseEvent(
                        mojom::ToolUseEvent::New("weather_tool", "tool_id_1",
                                                 "{\"location\":\"New York\"}",
                                                 std::nullopt, nullptr)),
                    std::nullopt));
              }),
          testing::WithArg<8>(
              [&](EngineConsumer::GenerationCompletedCallback callback) {
                std::move(callback).Run(
                    base::ok(EngineConsumer::GenerationResultData(
                        mojom::ConversationEntryEvent::NewCompletionEvent(
                            mojom::CompletionEvent::New("")),
                        std::nullopt)));
              })));

  // We will still be "in progress" whilst any automatic tools are being called
  EXPECT_CALL(untrusted_client, OnEntriesUIStateChanged(
                                    ConversationEntriesStateIsGenerating(true)))
      .Times(testing::AtLeast(1));

  // Client and observer should be given the tool use event output when it's
  // available.
  auto expected_tool_use_event = mojom::ToolUseEvent::New(
      "weather_tool", "tool_id_1", "{\"location\":\"New York\"}",
      CreateContentBlocksForText("Weather in New York: 72°F"), nullptr);

  EXPECT_CALL(untrusted_client, OnToolUseEventOutput)
      .WillOnce(testing::WithArg<1>([&](mojom::ToolUseEventPtr tool_use_event) {
        EXPECT_MOJOM_EQ(*tool_use_event, *expected_tool_use_event);
      }));

  EXPECT_CALL(observer, OnToolUseEventOutput(_, _, 1, _))
      .WillOnce(testing::WithArg<3>([&](mojom::ToolUseEventPtr tool_use_event) {
        EXPECT_MOJOM_EQ(*tool_use_event, *expected_tool_use_event);
      }));

  // Only the weather_tool UseTool should be called
  EXPECT_CALL(*tool1, UseTool(StrEq("{\"location\":\"New York\"}"), _))
      .InSequence(seq)
      .WillOnce(testing::WithArg<1>([&](Tool::UseToolCallback callback) {
        std::vector<mojom::ContentBlockPtr> result;
        result.push_back(mojom::ContentBlock::NewTextContentBlock(
            mojom::TextContentBlock::New("Weather in New York: 72°F")));
        std::move(callback).Run(std::move(result));
      }));

  // Second call to engine receives the tool output and provides the next
  // assistant response iteration.
  EXPECT_CALL(*engine, GenerateAssistantResponse)
      .InSequence(seq)
      .WillOnce(testing::DoAll(
          testing::InvokeWithoutArgs(
              [&]() { second_generation_started = true; }),
          testing::WithArg<7>(
              [](EngineConsumer::GenerationDataCallback callback) {
                callback.Run(EngineConsumer::GenerationResultData(
                    mojom::ConversationEntryEvent::NewCompletionEvent(
                        mojom::CompletionEvent::New(
                            "Based on the weather data, it's 72F")),
                    std::nullopt));
              }),
          testing::WithArg<8>(
              [&](EngineConsumer::GenerationCompletedCallback callback) {
                std::move(callback).Run(
                    base::ok(EngineConsumer::GenerationResultData(
                        mojom::ConversationEntryEvent::NewCompletionEvent(
                            mojom::CompletionEvent::New("")),
                        std::nullopt)));
                // Wait for async mojom events to be completed
                run_loop.QuitWhenIdle();
              })));

  // We should see the final "generation in progress" change to false
  EXPECT_CALL(
      untrusted_client,
      OnEntriesUIStateChanged(ConversationEntriesStateIsGenerating(false)))
      .WillRepeatedly(testing::InvokeWithoutArgs([&]() {
        // This should only be called after the second generation has started
        EXPECT_TRUE(second_generation_started);
      }));

  EXPECT_CALL(*tool2, UseTool).Times(0);

  // Submit a human entry to trigger the tool use
  conversation_handler_->SubmitHumanConversationEntry(
      "What's the weather in New York?", std::nullopt);

  run_loop.Run();

  const auto& history = conversation_handler_->GetConversationHistory();
  // human entry + assistant entry with tool + assistant entry with response
  ASSERT_EQ(history.size(), 3u);
  auto& assistant_entry = history[1];
  ASSERT_TRUE(assistant_entry->events.has_value());
  auto& events = assistant_entry->events.value();
  ASSERT_EQ(events.size(), 2u);
  EXPECT_TRUE(events[1]->is_tool_use_event());
  auto& tool_event = events[1]->get_tool_use_event();
  EXPECT_TRUE(tool_event->output.has_value());
  EXPECT_EQ(tool_event->output->size(), 1u);
  EXPECT_MOJOM_EQ(
      tool_event->output->at(0),
      mojom::ContentBlock::NewTextContentBlock(
          mojom::TextContentBlock::New("Weather in New York: 72°F")));
}

TEST_F(ConversationHandlerUnitTest, ToolUseEvents_MultipleToolsCalled) {
  conversation_handler_->associated_content_manager()->ClearContent();
  MockEngineConsumer* engine = static_cast<MockEngineConsumer*>(
      conversation_handler_->GetEngineForTesting());

  // Setup multiple tools with both being called
  auto tool1 = std::make_unique<NiceMock<MockTool>>("test_tool", "Test tool");
  auto tool2 =
      std::make_unique<NiceMock<MockTool>>("test_tool2", "Test tool 2");

  tool1->set_requires_user_interaction_before_handling(false);
  tool2->set_requires_user_interaction_before_handling(false);

  ON_CALL(*mock_tool_provider_, GetTools()).WillByDefault([&]() {
    std::vector<base::WeakPtr<Tool>> tools;
    tools.push_back(tool1->GetWeakPtr());
    tools.push_back(tool2->GetWeakPtr());
    return tools;
  });

  NiceMock<MockConversationHandlerClient> client(conversation_handler_.get());

  // Expect two calls to GenerateAssistantResponse:
  // 1. First call returns tool use event
  // 2. Second call (after tool completes) returns final response

  base::RunLoop run_loop;

  testing::Sequence seq;
  EXPECT_CALL(*engine, GenerateAssistantResponse)
      .InSequence(seq)
      .WillOnce(testing::DoAll(
          testing::WithArg<7>(
              [](EngineConsumer::GenerationDataCallback callback) {
                callback.Run(EngineConsumer::GenerationResultData(
                    mojom::ConversationEntryEvent::NewToolUseEvent(
                        mojom::ToolUseEvent::New("test_tool", "tool_id_1",
                                                 "{\"location\":\"NYC\"}",
                                                 std::nullopt, nullptr)),
                    std::nullopt));
              }),
          testing::WithArg<7>(
              [](EngineConsumer::GenerationDataCallback callback) {
                callback.Run(EngineConsumer::GenerationResultData(
                    mojom::ConversationEntryEvent::NewToolUseEvent(
                        mojom::ToolUseEvent::New("test_tool2", "tool_id_2",
                                                 "{\"input1\":\"val1\"}",
                                                 std::nullopt, nullptr)),
                    std::nullopt));
              }),
          testing::WithArg<8>(
              [&](EngineConsumer::GenerationCompletedCallback callback) {
                std::move(callback).Run(
                    base::ok(EngineConsumer::GenerationResultData(
                        mojom::ConversationEntryEvent::NewCompletionEvent(
                            mojom::CompletionEvent::New("")),
                        std::nullopt)));
              })));

  // Setup tool use results
  EXPECT_CALL(*tool1, UseTool(StrEq("{\"location\":\"NYC\"}"), _))
      .InSequence(seq)
      .WillOnce(testing::WithArg<1>([&](Tool::UseToolCallback callback) {
        std::vector<mojom::ContentBlockPtr> result;
        result.push_back(mojom::ContentBlock::NewTextContentBlock(
            mojom::TextContentBlock::New("Result from tool1")));
        std::move(callback).Run(std::move(result));
      }));

  EXPECT_CALL(*tool2, UseTool(StrEq("{\"input1\":\"val1\"}"), _))
      .InSequence(seq)
      .WillOnce(testing::WithArg<1>([&](Tool::UseToolCallback callback) {
        std::vector<mojom::ContentBlockPtr> result;
        result.push_back(mojom::ContentBlock::NewTextContentBlock(
            mojom::TextContentBlock::New("Result from tool2")));
        std::move(callback).Run(std::move(result));
      }));

  EXPECT_CALL(*engine, GenerateAssistantResponse)
      .InSequence(seq)
      .WillOnce(testing::DoAll(
          testing::WithArg<7>(
              [](EngineConsumer::GenerationDataCallback callback) {
                callback.Run(EngineConsumer::GenerationResultData(
                    mojom::ConversationEntryEvent::NewCompletionEvent(
                        mojom::CompletionEvent::New(
                            "Based on the weather data, it's 72F")),
                    std::nullopt));
              }),
          testing::WithArg<8>(
              [&](EngineConsumer::GenerationCompletedCallback callback) {
                std::move(callback).Run(
                    base::ok(EngineConsumer::GenerationResultData(
                        mojom::ConversationEntryEvent::NewCompletionEvent(
                            mojom::CompletionEvent::New("")),
                        std::nullopt)));
                run_loop.Quit();
              })));

  // Submit human entry to start the flow
  conversation_handler_->SubmitHumanConversationEntry("What's the weather?",
                                                      std::nullopt);

  run_loop.Run();

  const auto& history = conversation_handler_->GetConversationHistory();
  // human entry + assistant entry with tool + assistant entry with response
  EXPECT_EQ(history.size(), 3u);

  // Check the final response
  EXPECT_EQ(history.back()->text, "Based on the weather data, it's 72F");

  // Check that all tool use events have the correct input and output
  auto& assistant_entry = history[1];
  ASSERT_TRUE(assistant_entry->events.has_value());
  auto& events = assistant_entry->events.value();
  ASSERT_EQ(events.size(), 2u);
  EXPECT_TRUE(events[0]->is_tool_use_event());
  auto& tool_event = events[0]->get_tool_use_event();
  EXPECT_TRUE(events[1]->is_tool_use_event());
  auto& tool_event2 = events[1]->get_tool_use_event();
  EXPECT_TRUE(tool_event->output.has_value());
  EXPECT_EQ(tool_event->tool_name, "test_tool");
  EXPECT_EQ(tool_event->id, "tool_id_1");
  EXPECT_EQ(tool_event->arguments_json, "{\"location\":\"NYC\"}");
  EXPECT_EQ(tool_event->output->size(), 1u);
  EXPECT_MOJOM_EQ(tool_event->output->at(0),
                  mojom::ContentBlock::NewTextContentBlock(
                      mojom::TextContentBlock::New("Result from tool1")));
  EXPECT_TRUE(tool_event2->output.has_value());
  EXPECT_EQ(tool_event2->tool_name, "test_tool2");
  EXPECT_EQ(tool_event2->id, "tool_id_2");
  EXPECT_EQ(tool_event2->arguments_json, "{\"input1\":\"val1\"}");
  EXPECT_EQ(tool_event2->output->size(), 1u);
  EXPECT_MOJOM_EQ(tool_event2->output->at(0),
                  mojom::ContentBlock::NewTextContentBlock(
                      mojom::TextContentBlock::New("Result from tool2")));
}

TEST_F(ConversationHandlerUnitTest,
       ToolUseEvents_RequiresUserInteractionBeforeHandling) {
  conversation_handler_->associated_content_manager()->ClearContent();
  MockEngineConsumer* engine = static_cast<MockEngineConsumer*>(
      conversation_handler_->GetEngineForTesting());

  // Setup a tool that requires user interaction before handling
  auto tool1 = std::make_unique<NiceMock<MockTool>>("test_tool", "Test tool");
  tool1->set_requires_user_interaction_before_handling(true);

  ON_CALL(*mock_tool_provider_, GetTools()).WillByDefault([&]() {
    std::vector<base::WeakPtr<Tool>> tools;
    tools.push_back(tool1->GetWeakPtr());
    return tools;
  });

  NiceMock<MockConversationHandlerClient> client(conversation_handler_.get());

  // Expect first GenerateAssistantResponse to return tool use event
  base::RunLoop first_generation_loop;
  testing::Sequence seq;
  EXPECT_CALL(*engine, GenerateAssistantResponse)
      .InSequence(seq)
      .WillOnce(testing::DoAll(
          testing::WithArg<7>(
              [](EngineConsumer::GenerationDataCallback callback) {
                callback.Run(EngineConsumer::GenerationResultData(
                    mojom::ConversationEntryEvent::NewToolUseEvent(
                        mojom::ToolUseEvent::New("test_tool", "tool_id_1",
                                                 "{\"param\":\"value\"}",
                                                 std::nullopt, nullptr)),
                    std::nullopt));
              }),
          testing::WithArg<8>(
              [&](EngineConsumer::GenerationCompletedCallback callback) {
                std::move(callback).Run(
                    base::ok(EngineConsumer::GenerationResultData(
                        mojom::ConversationEntryEvent::NewCompletionEvent(
                            mojom::CompletionEvent::New("")),
                        std::nullopt)));
                first_generation_loop.Quit();
              })));

  // Tool should not be called since there is no explicit call via user
  // interaction.
  EXPECT_CALL(*tool1, UseTool).Times(0);

  // When the user instead decides to send a new human entry, before the tool
  // use request is handled, the tool use request should be discarded.
  base::RunLoop second_generation_loop;
  EXPECT_CALL(*engine, GenerateAssistantResponse)
      .InSequence(seq)
      .WillOnce(testing::DoAll(
          testing::WithArg<7>(
              [](EngineConsumer::GenerationDataCallback callback) {
                callback.Run(EngineConsumer::GenerationResultData(
                    mojom::ConversationEntryEvent::NewCompletionEvent(
                        mojom::CompletionEvent::New("Here's a new response")),
                    std::nullopt));
              }),
          testing::WithArg<8>(
              [&](EngineConsumer::GenerationCompletedCallback callback) {
                std::move(callback).Run(
                    base::ok(EngineConsumer::GenerationResultData(
                        mojom::ConversationEntryEvent::NewCompletionEvent(
                            mojom::CompletionEvent::New("")),
                        std::nullopt)));
                second_generation_loop.Quit();
              })));

  // Submit first human entry to get tool use event
  conversation_handler_->SubmitHumanConversationEntry("First question",
                                                      std::nullopt);
  first_generation_loop.Run();

  // Verify the tool use event exists and has no output
  const auto& history_before = conversation_handler_->GetConversationHistory();
  ASSERT_EQ(history_before.size(), 2u);  // human + assistant with tool
  auto& assistant_before = history_before.back();
  ASSERT_TRUE(assistant_before->events.has_value());
  auto& events_before = assistant_before->events.value();
  ASSERT_EQ(events_before.size(), 1u);
  EXPECT_TRUE(events_before[0]->is_tool_use_event());
  EXPECT_FALSE(events_before[0]->get_tool_use_event()->output.has_value());

  // Submit a new human entry which should cancel the pending tool use event
  conversation_handler_->SubmitHumanConversationEntry("New question",
                                                      std::nullopt);
  second_generation_loop.Run();

  // Verify the pending tool use event was removed
  const auto& history_after = conversation_handler_->GetConversationHistory();
  // original human + assistant (with tools removed) + new human + new assistant
  EXPECT_EQ(history_after.size(), 4u);

  // Check that the first assistant entry no longer has the pending tool use
  // event
  auto& assistant_after = history_after[1];
  ASSERT_TRUE(assistant_after->events.has_value());
  auto& events_after = assistant_after->events.value();
  EXPECT_TRUE(events_after.empty());  // Tool use event should be removed
}

TEST_F(ConversationHandlerUnitTest, ToolUseEvents_MultipleToolIterations) {
  conversation_handler_->associated_content_manager()->ClearContent();
  MockEngineConsumer* engine = static_cast<MockEngineConsumer*>(
      conversation_handler_->GetEngineForTesting());

  // Setup multiple tools
  auto tool1 = std::make_unique<NiceMock<MockTool>>("tool1", "First tool");
  auto tool2 = std::make_unique<NiceMock<MockTool>>("tool2", "Second tool");

  tool1->set_requires_user_interaction_before_handling(false);
  tool2->set_requires_user_interaction_before_handling(false);

  ON_CALL(*mock_tool_provider_, GetTools()).WillByDefault([&]() {
    std::vector<base::WeakPtr<Tool>> tools;
    tools.push_back(tool1->GetWeakPtr());
    tools.push_back(tool2->GetWeakPtr());
    return tools;
  });

  // Expect our tool provider will be informed of the new generation loop
  // starting.
  EXPECT_CALL(*mock_tool_provider_, OnNewGenerationLoop).Times(1);

  NiceMock<MockConversationHandlerClient> client(conversation_handler_.get());

  // Expect three calls to GenerateAssistantResponse:
  // 1. First call returns first tool use event
  // 2. Second call returns second tool use event response
  // 3. Third call returns final response
  base::RunLoop run_loop;

  testing::Sequence seq;
  EXPECT_CALL(*engine, GenerateAssistantResponse)
      .InSequence(seq)
      .WillOnce(testing::DoAll(
          testing::WithArg<7>(
              [](EngineConsumer::GenerationDataCallback callback) {
                callback.Run(EngineConsumer::GenerationResultData(
                    mojom::ConversationEntryEvent::NewToolUseEvent(
                        mojom::ToolUseEvent::New("tool1", "tool_id_1",
                                                 "{\"param1\":\"value1\"}",
                                                 std::nullopt, nullptr)),
                    std::nullopt));
              }),
          testing::WithArg<8>(
              [&](EngineConsumer::GenerationCompletedCallback callback) {
                std::move(callback).Run(
                    base::ok(EngineConsumer::GenerationResultData(
                        mojom::ConversationEntryEvent::NewCompletionEvent(
                            mojom::CompletionEvent::New("")),
                        std::nullopt)));
              })));

  EXPECT_CALL(*tool1, UseTool(StrEq("{\"param1\":\"value1\"}"), _))
      .WillOnce(testing::WithArg<1>([&](Tool::UseToolCallback callback) {
        std::vector<mojom::ContentBlockPtr> result;
        result.push_back(mojom::ContentBlock::NewTextContentBlock(
            mojom::TextContentBlock::New("Result from tool1")));
        std::move(callback).Run(std::move(result));
      }));

  EXPECT_CALL(*tool2, UseTool(StrEq("{\"param2\":\"value2\"}"), _))
      .WillOnce(testing::WithArg<1>([&](Tool::UseToolCallback callback) {
        std::vector<mojom::ContentBlockPtr> result;
        result.push_back(mojom::ContentBlock::NewTextContentBlock(
            mojom::TextContentBlock::New("Result from tool2")));
        std::move(callback).Run(std::move(result));
      }));

  // Second assistant response should ask for the second tool to be run
  EXPECT_CALL(*engine, GenerateAssistantResponse)
      .InSequence(seq)
      .WillOnce(testing::DoAll(
          testing::WithArg<7>(
              [](EngineConsumer::GenerationDataCallback callback) {
                callback.Run(EngineConsumer::GenerationResultData(
                    mojom::ConversationEntryEvent::NewToolUseEvent(
                        mojom::ToolUseEvent::New("tool2", "tool_id_2",
                                                 "{\"param2\":\"value2\"}",
                                                 std::nullopt, nullptr)),
                    std::nullopt));
              }),
          testing::WithArg<8>(
              [&](EngineConsumer::GenerationCompletedCallback callback) {
                std::move(callback).Run(
                    base::ok(EngineConsumer::GenerationResultData(
                        mojom::ConversationEntryEvent::NewCompletionEvent(
                            mojom::CompletionEvent::New("")),
                        std::nullopt)));
              })));

  // Third assistant response should return the final response
  EXPECT_CALL(*engine, GenerateAssistantResponse)
      .InSequence(seq)
      .WillOnce(testing::DoAll(
          testing::WithArg<7>(
              [](EngineConsumer::GenerationDataCallback callback) {
                callback.Run(EngineConsumer::GenerationResultData(
                    mojom::ConversationEntryEvent::NewCompletionEvent(
                        mojom::CompletionEvent::New(
                            "Final response after tools")),
                    std::nullopt));
              }),
          testing::WithArg<8>(
              [&](EngineConsumer::GenerationCompletedCallback callback) {
                std::move(callback).Run(
                    base::ok(EngineConsumer::GenerationResultData(
                        mojom::ConversationEntryEvent::NewCompletionEvent(
                            mojom::CompletionEvent::New("")),
                        std::nullopt)));
                run_loop.Quit();
              })));

  // Submit human entry to start the flow
  conversation_handler_->SubmitHumanConversationEntry("Use multiple tools",
                                                      std::nullopt);

  // Wait for the final response to complete
  run_loop.Run();

  // Verify both tools have output and we have a final response
  const auto& history = conversation_handler_->GetConversationHistory();
  // human + assistant with tool1 + assistant with tool2 + assistant with final
  // response
  EXPECT_EQ(history.size(), 4u);

  auto& assistant_with_tool1 = history[1];
  ASSERT_TRUE(assistant_with_tool1->events.has_value());
  auto& events = assistant_with_tool1->events.value();
  ASSERT_EQ(events.size(), 1u);
  EXPECT_TRUE(events[0]->is_tool_use_event());
  EXPECT_TRUE(events[0]->get_tool_use_event()->output.has_value());
  EXPECT_EQ(events[0]->get_tool_use_event()->tool_name, "tool1");
  EXPECT_EQ(events[0]->get_tool_use_event()->id, "tool_id_1");
  EXPECT_EQ(events[0]->get_tool_use_event()->arguments_json,
            "{\"param1\":\"value1\"}");
  EXPECT_EQ(events[0]->get_tool_use_event()->output->size(), 1u);
  EXPECT_MOJOM_EQ(events[0]->get_tool_use_event()->output->at(0),
                  mojom::ContentBlock::NewTextContentBlock(
                      mojom::TextContentBlock::New("Result from tool1")));

  auto& assistant_with_tool2 = history[2];
  ASSERT_TRUE(assistant_with_tool2->events.has_value());
  auto& events2 = assistant_with_tool2->events.value();
  ASSERT_EQ(events2.size(), 1u);
  EXPECT_TRUE(events2[0]->is_tool_use_event());
  EXPECT_TRUE(events2[0]->get_tool_use_event()->output.has_value());
  EXPECT_EQ(events2[0]->get_tool_use_event()->tool_name, "tool2");
  EXPECT_EQ(events2[0]->get_tool_use_event()->id, "tool_id_2");
  EXPECT_EQ(events2[0]->get_tool_use_event()->arguments_json,
            "{\"param2\":\"value2\"}");
  EXPECT_EQ(events2[0]->get_tool_use_event()->output->size(), 1u);
  EXPECT_MOJOM_EQ(events2[0]->get_tool_use_event()->output->at(0),
                  mojom::ContentBlock::NewTextContentBlock(
                      mojom::TextContentBlock::New("Result from tool2")));

  // Final response should be present
  EXPECT_EQ(history.back()->text, "Final response after tools");
}

TEST_F(ConversationHandlerUnitTest, ToolUseEvents_ToolNotFound) {
  // Test that requesting a non-existent tool returns proper error message
  conversation_handler_->associated_content_manager()->ClearContent();
  auto* engine = static_cast<MockEngineConsumer*>(
      conversation_handler_->GetEngineForTesting());

  MockUntrustedConversationHandlerClient untrusted_client(
      conversation_handler_.get());

  base::RunLoop run_loop;

  // Set up engine to return a response with a tool use request for non-existent
  // tool, then expect a second call after tool error is handled
  testing::Sequence seq;

  // First call: returns tool use event for non-existent tool
  EXPECT_CALL(*engine, GenerateAssistantResponse)
      .InSequence(seq)
      .WillOnce(testing::DoAll(
          // Send completion event first (like working test)
          testing::WithArg<7>(
              [](EngineConsumer::GenerationDataCallback data_callback) {
                data_callback.Run(EngineConsumer::GenerationResultData(
                    mojom::ConversationEntryEvent::NewCompletionEvent(
                        mojom::CompletionEvent::New("Let me help you...")),
                    std::nullopt));
              }),
          // Then send tool use event via data callback
          testing::WithArg<7>(
              [](EngineConsumer::GenerationDataCallback data_callback) {
                data_callback.Run(EngineConsumer::GenerationResultData(
                    mojom::ConversationEntryEvent::NewToolUseEvent(
                        mojom::ToolUseEvent::New("nonexistent_tool",
                                                 "test_tool_id", "{}",
                                                 std::nullopt, nullptr)),
                    std::nullopt));
              }),
          // Complete with empty completion event (like working test)
          testing::WithArg<8>([](EngineConsumer::GenerationCompletedCallback
                                     completion_callback) {
            std::move(completion_callback)
                .Run(base::ok(EngineConsumer::GenerationResultData(
                    mojom::ConversationEntryEvent::NewCompletionEvent(
                        mojom::CompletionEvent::New("")),
                    std::nullopt)));
          })));

  // Second call: after tool error is handled, should continue generation
  EXPECT_CALL(*engine, GenerateAssistantResponse)
      .InSequence(seq)
      .WillOnce(testing::DoAll(
          testing::WithArg<7>(
              [](EngineConsumer::GenerationDataCallback data_callback) {
                data_callback.Run(EngineConsumer::GenerationResultData(
                    mojom::ConversationEntryEvent::NewCompletionEvent(
                        mojom::CompletionEvent::New(
                            "Final response after handling tool error")),
                    std::nullopt));
              }),
          testing::WithArg<8>([&](EngineConsumer::GenerationCompletedCallback
                                      completion_callback) {
            std::move(completion_callback)
                .Run(base::ok(EngineConsumer::GenerationResultData(
                    mojom::ConversationEntryEvent::NewCompletionEvent(
                        mojom::CompletionEvent::New("")),
                    std::nullopt)));
            run_loop.QuitWhenIdle();
          })));

  // Expect the error message when tool is not found
  EXPECT_CALL(untrusted_client, OnToolUseEventOutput)
      .WillOnce(testing::WithArg<1>([&](mojom::ToolUseEventPtr tool_use_event) {
        EXPECT_EQ(tool_use_event->tool_name, "nonexistent_tool");
        EXPECT_EQ(tool_use_event->id, "test_tool_id");

        ASSERT_TRUE(tool_use_event->output.has_value());
        const std::string& error_text =
            tool_use_event->output->at(0)->get_text_content_block()->text;
        EXPECT_EQ(error_text, "The nonexistent_tool tool is not available.");
      }));

  // Trigger generation which will cause tool lookup
  conversation_handler_->SubmitHumanConversationEntry("Help me with something",
                                                      std::nullopt);
  run_loop.Run();
}

TEST_F(ConversationHandlerUnitTest, ToolUseEvents_OnContentTaskStarted) {
  conversation_handler_->associated_content_manager()->ClearContent();

  int32_t test_tab_id = 1;

  EXPECT_EQ(0u, conversation_handler_->get_task_tab_ids().size());

  MockEngineConsumer* engine = static_cast<MockEngineConsumer*>(
      conversation_handler_->GetEngineForTesting());

  // This test verifies that the conversation client is informed of the start
  // of a content task from a ToolProvider.
  NiceMock<MockUntrustedConversationHandlerClient> untrusted_client(
      conversation_handler_.get());
  EXPECT_CALL(untrusted_client, ContentTaskStarted(test_tab_id));

  base::RunLoop run_loop;
  // Call to engine mocks the use tool request when the tool is first used.
  // We do not need to complete the request as this test is verifying that
  // the observation is made by the conversation client whilst the request
  // is still in progress so that the UI may follow the progress of the action.
  EXPECT_CALL(*engine, GenerateAssistantResponse)
      .WillOnce(testing::WithArg<7>(
          [&](EngineConsumer::GenerationDataCallback callback) {
            mock_tool_provider_->StartContentTask(test_tab_id);
            run_loop.QuitWhenIdle();  // QuitWhenIdle due to mojo connection
          }));

  // Submit a human entry to trigger the tool use
  conversation_handler_->SubmitHumanConversationEntry(".", std::nullopt);
  run_loop.Run();

  EXPECT_EQ(1u, conversation_handler_->get_task_tab_ids().size());
  EXPECT_EQ(test_tab_id, *conversation_handler_->get_task_tab_ids().begin());
}

TEST_F(ConversationHandlerUnitTest, AssociatingContentTriggersGetContent) {
  MockAssociatedContent content;
  content.SetTextContent("content");

  // We shouldn't have any content yet (because we haven't called |GetContent|).
  EXPECT_EQ(content.cached_page_content().content, "");
  conversation_handler_->associated_content_manager()->AddContent(&content);
  EXPECT_EQ(content.cached_page_content().content, "content");
}

struct EmptyContentTestData {
  std::string name;
  std::string content;
};

class ConversationHandlerUnitTest_AutoScreenshot
    : public ConversationHandlerUnitTest,
      public testing::WithParamInterface<EmptyContentTestData> {
 public:
  static std::vector<EmptyContentTestData> GetTestCases() {
    return {
        {"EmptyString", ""},
        {"StandardWhitespace", "   \t\n\r  "},
        {"MixedWhitespace", "\n\t \r\n  \t\r  "},
    };
  }
};

// Test that screenshots are automatically taken when page content is
// empty/whitespace-only
TEST_P(ConversationHandlerUnitTest_AutoScreenshot,
       AutoScreenshotOnEmptyContent) {
#if BUILDFLAG(IS_IOS)
  // Set a vision support model to prevent model switching
  // Remove this model switch once iOS set automatic as default
  model_service_->SetDefaultModelKeyWithoutValidationForTesting(
      kClaudeHaikuModelKey);
#endif
  const EmptyContentTestData& test_data = GetParam();

  // Mock associated content to return the test content
  associated_content_->SetTextContent(test_data.content);

  // Mock GetScreenshots to return sample screenshots
  std::vector<mojom::UploadedFilePtr> mock_screenshots =
      CreateSampleUploadedFiles(2, mojom::UploadedFileType::kScreenshot);
  EXPECT_CALL(*associated_content_, GetScreenshots)
      .WillOnce(base::test::RunOnceCallback<0>(Clone(mock_screenshots)));

  // Mock engine response
  MockEngineConsumer* engine = static_cast<MockEngineConsumer*>(
      conversation_handler_->GetEngineForTesting());
  EXPECT_CALL(*engine, GenerateAssistantResponse)
      .WillOnce(base::test::RunOnceCallback<8>(
          base::ok(EngineConsumer::GenerationResultData(
              mojom::ConversationEntryEvent::NewCompletionEvent(
                  mojom::CompletionEvent::New("Response with screenshots")),
              std::nullopt /* model_key */))));

  NiceMock<MockConversationHandlerClient> client(conversation_handler_.get());

  // Submit a conversation entry
  base::RunLoop loop;
  EXPECT_CALL(client, OnAPIRequestInProgress(true)).Times(1);
  EXPECT_CALL(client, OnAPIRequestInProgress(false))
      .WillOnce(testing::InvokeWithoutArgs(&loop, &base::RunLoop::Quit));

  conversation_handler_->SubmitHumanConversationEntry("Test question",
                                                      std::nullopt);
  loop.Run();

  // Verify that screenshots were attached to the conversation turn
  const auto& history = conversation_handler_->GetConversationHistory();
  ASSERT_EQ(history.size(), 2u);        // Human turn + assistant turn
  const auto& human_turn = history[0];  // Human turn (index 0)
  EXPECT_TRUE(human_turn->uploaded_files.has_value());
  EXPECT_EQ(human_turn->uploaded_files->size(), 2u);

  // Verify that the files are screenshots
  for (const auto& file : *human_turn->uploaded_files) {
    EXPECT_EQ(file->type, mojom::UploadedFileType::kScreenshot);
  }

  testing::Mock::VerifyAndClearExpectations(associated_content_.get());
  testing::Mock::VerifyAndClearExpectations(engine);
}

INSTANTIATE_TEST_SUITE_P(
    EmptyContentVariations,
    ConversationHandlerUnitTest_AutoScreenshot,
    testing::ValuesIn(
        ConversationHandlerUnitTest_AutoScreenshot::GetTestCases()),
    [](const testing::TestParamInfo<EmptyContentTestData>& info) {
      return info.param.name;
    });

// Test that screenshots are NOT taken when page content exists
TEST_F(ConversationHandlerUnitTest, NoScreenshotWhenContentExists) {
  // Mock associated content to return non-empty text content
  associated_content_->SetTextContent("Some page content");

  // GetScreenshots should NOT be called
  EXPECT_CALL(*associated_content_, GetScreenshots).Times(0);

  // Mock engine response
  MockEngineConsumer* engine = static_cast<MockEngineConsumer*>(
      conversation_handler_->GetEngineForTesting());
  EXPECT_CALL(*engine, GenerateAssistantResponse)
      .WillOnce(base::test::RunOnceCallback<8>(
          base::ok(EngineConsumer::GenerationResultData(
              mojom::ConversationEntryEvent::NewCompletionEvent(
                  mojom::CompletionEvent::New("Response without screenshots")),
              std::nullopt /* model_key */))));

  NiceMock<MockConversationHandlerClient> client(conversation_handler_.get());

  // Submit a conversation entry
  base::RunLoop loop;
  EXPECT_CALL(client, OnAPIRequestInProgress(true)).Times(1);
  EXPECT_CALL(client, OnAPIRequestInProgress(false))
      .WillOnce(testing::InvokeWithoutArgs(&loop, &base::RunLoop::Quit));

  conversation_handler_->SubmitHumanConversationEntry("Test question",
                                                      std::nullopt);
  loop.Run();

  // Verify that no screenshots were attached
  const auto& history = conversation_handler_->GetConversationHistory();
  ASSERT_FALSE(history.empty());
  const auto& last_turn = history.back();
  EXPECT_FALSE(last_turn->uploaded_files.has_value());

  testing::Mock::VerifyAndClearExpectations(associated_content_.get());
  testing::Mock::VerifyAndClearExpectations(engine);
}

// Test that screenshots are NOT taken when screenshots already exist in
// conversation
TEST_F(ConversationHandlerUnitTest, NoScreenshotWhenScreenshotsAlreadyExist) {
  // Mock associated content to return empty text content
  associated_content_->SetTextContent("");

  // Add existing screenshots to conversation history
  std::vector<mojom::ConversationTurnPtr> history;
  auto turn_with_screenshots = mojom::ConversationTurn::New(
      "turn-screenshots", mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
      "Previous question", std::nullopt, std::nullopt, std::nullopt,
      base::Time::Now(), std::nullopt,
      CreateSampleUploadedFiles(1, mojom::UploadedFileType::kScreenshot),
      nullptr /* skill */, false, std::nullopt);
  history.push_back(std::move(turn_with_screenshots));
  conversation_handler_->SetChatHistoryForTesting(std::move(history));

  // GetScreenshots should NOT be called because screenshots already exist
  EXPECT_CALL(*associated_content_, GetScreenshots).Times(0);

  // Mock engine response
  MockEngineConsumer* engine = static_cast<MockEngineConsumer*>(
      conversation_handler_->GetEngineForTesting());
  EXPECT_CALL(*engine, GenerateAssistantResponse)
      .WillOnce(base::test::RunOnceCallback<8>(
          base::ok(EngineConsumer::GenerationResultData(
              mojom::ConversationEntryEvent::NewCompletionEvent(
                  mojom::CompletionEvent::New(
                      "Response without new screenshots")),
              std::nullopt /* model_key */))));

  NiceMock<MockConversationHandlerClient> client(conversation_handler_.get());

  // Submit a conversation entry
  base::RunLoop loop;
  EXPECT_CALL(client, OnAPIRequestInProgress(true)).Times(1);
  EXPECT_CALL(client, OnAPIRequestInProgress(false))
      .WillOnce(testing::InvokeWithoutArgs(&loop, &base::RunLoop::Quit));

  conversation_handler_->SubmitHumanConversationEntry("Test question",
                                                      std::nullopt);
  loop.Run();

  // Verify that no new screenshots were attached
  const auto& new_history = conversation_handler_->GetConversationHistory();
  ASSERT_EQ(new_history.size(),
            3u);  // Previous turn + human turn + assistant turn
  const auto& new_turn = new_history.back();
  EXPECT_FALSE(new_turn->uploaded_files.has_value());

  testing::Mock::VerifyAndClearExpectations(associated_content_.get());
  testing::Mock::VerifyAndClearExpectations(engine);
}

// Test that screenshots are appended to existing uploaded files
TEST_F(ConversationHandlerUnitTest, ScreenshotsAppendToExistingFiles) {
#if BUILDFLAG(IS_IOS)
  // Set a vision support model to prevent model switching
  // Remove this model switch once iOS set automatic as default
  model_service_->SetDefaultModelKeyWithoutValidationForTesting(
      kClaudeHaikuModelKey);
#endif
  // Mock associated content to return empty text content
  associated_content_->SetTextContent("");

  // Mock GetScreenshots to return sample screenshots
  std::vector<mojom::UploadedFilePtr> mock_screenshots =
      CreateSampleUploadedFiles(1, mojom::UploadedFileType::kScreenshot);
  EXPECT_CALL(*associated_content_, GetScreenshots)
      .WillOnce(base::test::RunOnceCallback<0>(Clone(mock_screenshots)));

  // Mock engine response
  MockEngineConsumer* engine = static_cast<MockEngineConsumer*>(
      conversation_handler_->GetEngineForTesting());
  EXPECT_CALL(*engine, GenerateAssistantResponse)
      .WillOnce(base::test::RunOnceCallback<8>(
          base::ok(EngineConsumer::GenerationResultData(
              mojom::ConversationEntryEvent::NewCompletionEvent(
                  mojom::CompletionEvent::New("Response with mixed content")),
              std::nullopt /* model_key */))));

  NiceMock<MockConversationHandlerClient> client(conversation_handler_.get());

  // Submit a conversation entry with existing images
  base::RunLoop loop;
  EXPECT_CALL(client, OnAPIRequestInProgress(true)).Times(1);
  EXPECT_CALL(client, OnAPIRequestInProgress(false))
      .WillOnce(testing::InvokeWithoutArgs(&loop, &base::RunLoop::Quit));

  auto existing_images =
      CreateSampleUploadedFiles(2, mojom::UploadedFileType::kImage);
  conversation_handler_->SubmitHumanConversationEntry("Test question",
                                                      Clone(existing_images));
  loop.Run();

  // Verify that screenshots were appended to existing files
  const auto& new_history = conversation_handler_->GetConversationHistory();
  ASSERT_EQ(new_history.size(), 2u);        // Human turn + assistant turn
  const auto& human_turn = new_history[0];  // Human turn (index 0)
  EXPECT_TRUE(human_turn->uploaded_files.has_value());
  EXPECT_EQ(human_turn->uploaded_files->size(), 3u);  // 2 images + 1 screenshot

  // Verify that the first two files are images and the last is a screenshot
  EXPECT_EQ((*human_turn->uploaded_files)[0]->type,
            mojom::UploadedFileType::kImage);
  EXPECT_EQ((*human_turn->uploaded_files)[1]->type,
            mojom::UploadedFileType::kImage);
  EXPECT_EQ((*human_turn->uploaded_files)[2]->type,
            mojom::UploadedFileType::kScreenshot);

  testing::Mock::VerifyAndClearExpectations(associated_content_.get());
  testing::Mock::VerifyAndClearExpectations(engine);
}

// Test that vision model is automatically switched when screenshots are taken
TEST_F(ConversationHandlerUnitTest, VisionModelSwitchOnScreenshots) {
  // Switch to a model without vision support first
  base::RunLoop loop_for_change_model;
  NiceMock<MockConversationHandlerClient> client(conversation_handler_.get());
  EXPECT_CALL(client, OnModelDataChanged)
      .WillOnce(testing::InvokeWithoutArgs(&loop_for_change_model,
                                           &base::RunLoop::Quit));
  conversation_handler_->ChangeModel("chat-basic");
  loop_for_change_model.Run();
  testing::Mock::VerifyAndClearExpectations(&client);

  // Re-setting a mock engine because it was replaced due to ChangeModel call.
  conversation_handler_->SetEngineForTesting(
      std::make_unique<NiceMock<MockEngineConsumer>>());
  MockEngineConsumer* engine = static_cast<MockEngineConsumer*>(
      conversation_handler_->GetEngineForTesting());

  EXPECT_FALSE(conversation_handler_->GetCurrentModel().vision_support);

  // Mock associated content to return empty text content
  associated_content_->SetTextContent("");

  // Mock GetScreenshots to return sample screenshots
  std::vector<mojom::UploadedFilePtr> mock_screenshots =
      CreateSampleUploadedFiles(1, mojom::UploadedFileType::kScreenshot);
  EXPECT_CALL(*associated_content_, GetScreenshots)
      .WillOnce(base::test::RunOnceCallback<0>(Clone(mock_screenshots)));

  // Mock engine response
  EXPECT_CALL(*engine, GenerateAssistantResponse)
      .WillRepeatedly(
          [](PageContentsMap page_contents,
             const std::vector<mojom::ConversationTurnPtr>& history,
             const std::string& selected_language, bool is_temporary_chat,
             const std::vector<base::WeakPtr<Tool>>& tools,
             std::optional<std::string_view> preferred_tool_name,
             mojom::ConversationCapability conversation_capability,
             EngineConsumer::GenerationDataCallback callback,
             EngineConsumer::GenerationCompletedCallback done_callback) {
            std::move(done_callback)
                .Run(base::ok(EngineConsumer::GenerationResultData(
                    mojom::ConversationEntryEvent::NewCompletionEvent(
                        mojom::CompletionEvent::New(
                            "Response with vision model")),
                    std::nullopt /* model_key */)));
          });

  base::RunLoop loop;
  // Note: OnModelDataChanged expectation is set at the end for auto model
  // switch
  EXPECT_CALL(client, OnModelDataChanged)
      .WillOnce(base::test::RunClosure(base::BindLambdaForTesting([&]() {
        // Verify auto switched to vision support model
        EXPECT_TRUE(conversation_handler_->GetCurrentModel().vision_support);
        loop.Quit();
      })));

  // Submit a conversation entry
  conversation_handler_->SubmitHumanConversationEntry("Test question",
                                                      std::nullopt);
  loop.Run();

  // Verify that screenshots were attached and model has vision support
  const auto& history = conversation_handler_->GetConversationHistory();
  ASSERT_EQ(history.size(), 1u);        // Only human turn (assistant turn won't
                                        // complete due to model switch)
  const auto& human_turn = history[0];  // Human turn (index 0)
  EXPECT_TRUE(human_turn->uploaded_files.has_value());
  EXPECT_EQ(human_turn->uploaded_files->size(), 1u);
  EXPECT_EQ((*human_turn->uploaded_files)[0]->type,
            mojom::UploadedFileType::kScreenshot);
  EXPECT_TRUE(conversation_handler_->GetCurrentModel().vision_support);

  testing::Mock::VerifyAndClearExpectations(&client);
  testing::Mock::VerifyAndClearExpectations(associated_content_.get());
  testing::Mock::VerifyAndClearExpectations(engine);
}

// Test that screenshots are NOT taken when there's no associated content
TEST_F(ConversationHandlerUnitTest_NoAssociatedContent,
       NoScreenshotWhenNoAssociatedContent) {
  // Note: We can't mock associated_content_ here because it's null in this test
  // class

  // Mock engine response
  MockEngineConsumer* engine = static_cast<MockEngineConsumer*>(
      conversation_handler_->GetEngineForTesting());
  EXPECT_CALL(*engine, GenerateAssistantResponse)
      .WillOnce(base::test::RunOnceCallback<8>(
          base::ok(EngineConsumer::GenerationResultData(
              mojom::ConversationEntryEvent::NewCompletionEvent(
                  mojom::CompletionEvent::New("Response without screenshots")),
              std::nullopt /* model_key */))));

  NiceMock<MockConversationHandlerClient> client(conversation_handler_.get());

  // Submit a conversation entry
  base::RunLoop loop;
  EXPECT_CALL(client, OnAPIRequestInProgress(true)).Times(1);
  EXPECT_CALL(client, OnAPIRequestInProgress(false))
      .WillOnce(testing::InvokeWithoutArgs(&loop, &base::RunLoop::Quit));

  conversation_handler_->SubmitHumanConversationEntry("Test question",
                                                      std::nullopt);
  loop.Run();

  // Verify that no screenshots were attached
  const auto& history = conversation_handler_->GetConversationHistory();
  ASSERT_FALSE(history.empty());
  const auto& last_turn = history.back();
  EXPECT_FALSE(last_turn->uploaded_files.has_value());

  testing::Mock::VerifyAndClearExpectations(engine);
}

// Test that auto-screenshots apply MAX_IMAGES limit and trigger UI state change
TEST_F(ConversationHandlerUnitTest,
       OnAutoScreenshotsTaken_AppliesMaxImagesLimit) {
#if BUILDFLAG(IS_IOS)
  // Set a vision support model to prevent model switching
  model_service_->SetDefaultModelKeyWithoutValidationForTesting(
      kClaudeHaikuModelKey);
#endif
  // Mock associated content to return empty text content to trigger
  // auto-screenshots
  associated_content_->SetTextContent("");

  // Create a large number of screenshots to exceed MAX_IMAGES
  const size_t total_screenshots = mojom::MAX_IMAGES + 10;
  std::vector<mojom::UploadedFilePtr> mock_screenshots;
  for (size_t i = 0; i < total_screenshots; ++i) {
    mock_screenshots.push_back(mojom::UploadedFile::New(
        absl::StrFormat("screenshot_%zu.png", i), 1024,
        std::vector<uint8_t>(1024, 0), mojom::UploadedFileType::kScreenshot));
  }

  // Mock GetScreenshots to return screenshots that exceed MAX_IMAGES
  EXPECT_CALL(*associated_content_, GetScreenshots)
      .WillOnce(base::test::RunOnceCallback<0>(Clone(mock_screenshots)));

  // Mock engine response
  MockEngineConsumer* engine = static_cast<MockEngineConsumer*>(
      conversation_handler_->GetEngineForTesting());
  EXPECT_CALL(*engine, GenerateAssistantResponse)
      .WillOnce(base::test::RunOnceCallback<8>(
          base::ok(EngineConsumer::GenerationResultData(
              mojom::ConversationEntryEvent::NewCompletionEvent(
                  mojom::CompletionEvent::New("Response with screenshots")),
              std::nullopt /* model_key */))));

  // Create mock clients to verify API request progress and UI state changes
  NiceMock<MockConversationHandlerClient> client(conversation_handler_.get());
  NiceMock<MockUntrustedConversationHandlerClient> untrusted_client(
      conversation_handler_.get());

  // Expected visual content percentage: (20 * 100) / 30 = 66.67 -> 66
  const uint32_t expected_percentage = static_cast<uint32_t>(
      static_cast<float>(mojom::MAX_IMAGES) * 100.0f / total_screenshots);
  EXPECT_EQ(expected_percentage, 66u);

  // Allow any number of calls that don't match our specific expectation
  EXPECT_CALL(untrusted_client, OnEntriesUIStateChanged(testing::_))
      .Times(testing::AnyNumber());

  // Verify that OnEntriesUIStateChanged is called with the expected visual
  // content percentage
  EXPECT_CALL(untrusted_client,
              OnEntriesUIStateChanged(
                  ConversationEntriesStateHasVisualContentPercentage(
                      expected_percentage)))
      .Times(testing::AtLeast(1));

  // Submit empty string to trigger auto-screenshots
  base::RunLoop loop;
  EXPECT_CALL(client, OnAPIRequestInProgress(true)).Times(1);
  EXPECT_CALL(client, OnAPIRequestInProgress(false))
      .WillOnce(testing::InvokeWithoutArgs(&loop, &base::RunLoop::Quit));

  conversation_handler_->SubmitHumanConversationEntry("", std::nullopt);
  loop.Run();

  // Verify that the conversation history has screenshots limited to MAX_IMAGES
  const auto& history = conversation_handler_->GetConversationHistory();
  ASSERT_EQ(history.size(), 2u);        // Human turn + assistant turn
  const auto& human_turn = history[0];  // Human turn (index 0)
  EXPECT_TRUE(human_turn->uploaded_files.has_value());
  EXPECT_EQ(human_turn->uploaded_files->size(), mojom::MAX_IMAGES);

  // Verify all uploaded files are screenshots and in correct order
  for (size_t i = 0; i < human_turn->uploaded_files->size(); ++i) {
    const auto& file = (*human_turn->uploaded_files)[i];
    EXPECT_EQ(file->type, mojom::UploadedFileType::kScreenshot);
    EXPECT_EQ(file->filename, absl::StrFormat("screenshot_%zu.png", i));
  }

  // Verify that visual_content_used_percentage was set correctly
  auto entries_state = conversation_handler_->GetStateForConversationEntries();
  ASSERT_TRUE(entries_state->visual_content_used_percentage.has_value());
  EXPECT_EQ(entries_state->visual_content_used_percentage.value(),
            expected_percentage);
}

// Test that auto-screenshots don't trigger UI state change when under
// MAX_IMAGES
TEST_F(ConversationHandlerUnitTest,
       OnAutoScreenshotsTaken_NoLimitWhenUnderMax) {
#if BUILDFLAG(IS_IOS)
  // Set a vision support model to prevent model switching
  model_service_->SetDefaultModelKeyWithoutValidationForTesting(
      kClaudeHaikuModelKey);
#endif
  // Mock associated content to return empty text content to trigger
  // auto-screenshots
  associated_content_->SetTextContent("");

  // Create fewer screenshots than MAX_IMAGES
  const size_t total_screenshots = mojom::MAX_IMAGES - 5;
  std::vector<mojom::UploadedFilePtr> mock_screenshots;
  for (size_t i = 0; i < total_screenshots; ++i) {
    mock_screenshots.push_back(mojom::UploadedFile::New(
        absl::StrFormat("screenshot_%zu.png", i), 1024,
        std::vector<uint8_t>(1024, 0), mojom::UploadedFileType::kScreenshot));
  }

  // Mock GetScreenshots to return screenshots under MAX_IMAGES
  EXPECT_CALL(*associated_content_, GetScreenshots)
      .WillOnce(base::test::RunOnceCallback<0>(Clone(mock_screenshots)));

  // Mock engine response
  MockEngineConsumer* engine = static_cast<MockEngineConsumer*>(
      conversation_handler_->GetEngineForTesting());
  EXPECT_CALL(*engine, GenerateAssistantResponse)
      .WillOnce(base::test::RunOnceCallback<8>(
          base::ok(EngineConsumer::GenerationResultData(
              mojom::ConversationEntryEvent::NewCompletionEvent(
                  mojom::CompletionEvent::New("Response with screenshots")),
              std::nullopt /* model_key */))));

  // Create mock clients to verify API request progress and UI state changes
  NiceMock<MockConversationHandlerClient> client(conversation_handler_.get());
  NiceMock<MockUntrustedConversationHandlerClient> untrusted_client(
      conversation_handler_.get());

  // Allow normal OnEntriesUIStateChanged calls during API flow
  EXPECT_CALL(untrusted_client, OnEntriesUIStateChanged(testing::_))
      .Times(testing::AnyNumber());

  // Verify that OnEntriesUIStateChanged is NEVER called with any visual content
  // percentage when screenshots are under the limit (since no truncation occurs
  // and percentage stays nullopt)
  EXPECT_CALL(untrusted_client,
              OnEntriesUIStateChanged(
                  ConversationEntriesStateHasAnyVisualContentPercentage()))
      .Times(0);

  // Submit empty string to trigger auto-screenshots
  base::RunLoop loop;
  EXPECT_CALL(client, OnAPIRequestInProgress(true)).Times(1);
  EXPECT_CALL(client, OnAPIRequestInProgress(false))
      .WillOnce(testing::InvokeWithoutArgs(&loop, &base::RunLoop::Quit));

  conversation_handler_->SubmitHumanConversationEntry("", std::nullopt);
  loop.Run();

  // Verify that the conversation history has all screenshots preserved
  const auto& history = conversation_handler_->GetConversationHistory();
  ASSERT_EQ(history.size(), 2u);        // Human turn + assistant turn
  const auto& human_turn = history[0];  // Human turn (index 0)
  EXPECT_TRUE(human_turn->uploaded_files.has_value());
  EXPECT_EQ(human_turn->uploaded_files->size(), total_screenshots);

  // Verify all uploaded files are screenshots and in correct order
  for (size_t i = 0; i < human_turn->uploaded_files->size(); ++i) {
    const auto& file = (*human_turn->uploaded_files)[i];
    EXPECT_EQ(file->type, mojom::UploadedFileType::kScreenshot);
    EXPECT_EQ(file->filename, absl::StrFormat("screenshot_%zu.png", i));
  }

  // Verify that visual_content_used_percentage is not set (since we're under
  // limit)
  auto entries_state = conversation_handler_->GetStateForConversationEntries();
  EXPECT_FALSE(entries_state->visual_content_used_percentage.has_value());
}

// Test that OnEntriesUIStateChanged is not called when visual content
// percentage doesn't change (optimization test)
TEST_F(ConversationHandlerUnitTest,
       OnAutoScreenshotsTaken_SamePercentageNoUIUpdate) {
#if BUILDFLAG(IS_IOS)
  // Set a vision support model to prevent model switching
  model_service_->SetDefaultModelKeyWithoutValidationForTesting(
      kClaudeHaikuModelKey);
#endif

  // Simulate that we already have a visual content percentage set to 66
  // This mimics the state after a previous auto-screenshot operation
  // Currently autoscreenshots won't be triggered twice if there are already
  // screenshots in the context.
  conversation_handler_->visual_content_used_percentage_ = 66;

  // Create a callback that calculates the same percentage (66%)
  // to test that no UI update is triggered when value doesn't change
  const size_t total_screenshots = mojom::MAX_IMAGES + 10;
  const uint32_t expected_same_percentage = 66;

  // Verify the calculation would result in the same percentage
  uint32_t calculated_percentage = static_cast<uint32_t>(
      static_cast<float>(mojom::MAX_IMAGES) * 100.0f / total_screenshots);
  EXPECT_EQ(calculated_percentage, expected_same_percentage);

  // Directly call OnAutoScreenshotsTaken with screenshots that would
  // result in the same percentage
  std::vector<mojom::UploadedFilePtr> screenshots;
  for (size_t i = 0; i < total_screenshots; ++i) {
    screenshots.push_back(mojom::UploadedFile::New(
        absl::StrFormat("screenshot_%zu.png", i), 1024,
        std::vector<uint8_t>(1024, 0), mojom::UploadedFileType::kScreenshot));
  }

  // Create mock clients
  NiceMock<MockUntrustedConversationHandlerClient> untrusted_client(
      conversation_handler_.get());

  // Expect that OnEntriesUIStateChanged is NOT called since the percentage
  // doesn't change (optimization test)
  EXPECT_CALL(untrusted_client, OnEntriesUIStateChanged(testing::_)).Times(0);

  // Call the callback directly with a no-op callback
  conversation_handler_->OnAutoScreenshotsTaken(
      base::DoNothing(), std::make_optional(std::move(screenshots)));

  // Verify the percentage is still the same and no UI update was triggered
  auto entries_state = conversation_handler_->GetStateForConversationEntries();
  EXPECT_TRUE(entries_state->visual_content_used_percentage.has_value());
  EXPECT_EQ(entries_state->visual_content_used_percentage.value(), 66u);
}

TEST_F(ConversationHandlerUnitTest,
       AutomaticallyAssociateContentUponConversationEntryAdded) {
  // This test verifies that human turns are automatically associated when added
  // to conversation history. Initially content should not be associated.
  auto initial_content = conversation_handler_->associated_content_manager()
                             ->GetAssociatedContent();
  ASSERT_EQ(1u, initial_content.size());
  EXPECT_FALSE(initial_content[0]->conversation_turn_uuid.has_value());

  // Submit a human turn - this should automatically associate content
  conversation_handler_->SubmitHumanConversationEntry("Test message",
                                                      std::nullopt);

  // Verify content is now associated with a turn
  auto associated_content = conversation_handler_->associated_content_manager()
                                ->GetAssociatedContent();
  ASSERT_EQ(1u, associated_content.size());
  EXPECT_TRUE(associated_content[0]->conversation_turn_uuid.has_value());
}

TEST_F(ConversationHandlerUnitTest,
       SubmitHumanConversationEntry_TriggersConversationTitle) {
  // Test the title generation would be triggered for engines requiring
  // title generation when submitting the first human turn.
  conversation_handler_->associated_content_manager()->ClearContent();

  MockEngineConsumer* engine = static_cast<MockEngineConsumer*>(
      conversation_handler_->GetEngineForTesting());

  NiceMock<MockConversationHandlerClient> client(conversation_handler_.get());
  testing::NiceMock<MockConversationHandlerObserver> observer;
  observer.Observe(conversation_handler_.get());

  // Engine requires title generation
  EXPECT_CALL(*engine, RequiresClientSideTitleGeneration())
      .WillRepeatedly(testing::Return(true));

  // Set up expectations with key sequence: assistant response → title
  // generation
  base::RunLoop run_loop;
  testing::Sequence assistant_title_seq;

  // API request progress callbacks
  EXPECT_CALL(client, OnAPIRequestInProgress(true)).Times(1);
  EXPECT_CALL(client, OnAPIRequestInProgress(false))
      .WillOnce(testing::InvokeWithoutArgs(&run_loop, &base::RunLoop::Quit));

  // Assistant response is generated first
  EXPECT_CALL(*engine, GenerateAssistantResponse)
      .InSequence(assistant_title_seq)
      .WillOnce(testing::DoAll(
          // Mock successful assistant response
          testing::WithArg<7>(
              [](EngineConsumer::GenerationDataCallback callback) {
                std::move(callback).Run(EngineConsumer::GenerationResultData(
                    mojom::ConversationEntryEvent::NewCompletionEvent(
                        mojom::CompletionEvent::New("Assistant response")),
                    std::nullopt));
              }),
          testing::WithArg<8>(
              [](EngineConsumer::GenerationCompletedCallback callback) {
                std::move(callback).Run(
                    base::ok(EngineConsumer::GenerationResultData(
                        mojom::ConversationEntryEvent::NewCompletionEvent(
                            mojom::CompletionEvent::New("")),
                        std::nullopt)));
              })));

  // Then title generation is triggered
  EXPECT_CALL(*engine, GenerateConversationTitle)
      .InSequence(assistant_title_seq)
      .WillOnce(testing::WithArg<2>(
          [](EngineConsumer::GenerationCompletedCallback callback) {
            // Mock successful title generation
            std::move(callback).Run(
                base::ok(EngineConsumer::GenerationResultData(
                    mojom::ConversationEntryEvent::NewConversationTitleEvent(
                        mojom::ConversationTitleEvent::New("Generated Title")),
                    std::nullopt)));
          }));

  // Title change notification
  EXPECT_CALL(observer, OnConversationTitleChanged(_, StrEq("Generated Title")))
      .Times(1);

  // Submit human entry to trigger the flow
  conversation_handler_->SubmitHumanConversationEntry("Test question",
                                                      std::nullopt);
  run_loop.Run();

  // Verify conversation has 2 turns (human + assistant)
  const auto& history = conversation_handler_->GetConversationHistory();
  EXPECT_EQ(history.size(), 2u);
  EXPECT_EQ(history[0]->character_type, mojom::CharacterType::HUMAN);
  EXPECT_EQ(history[1]->character_type, mojom::CharacterType::ASSISTANT);
}

TEST_F(ConversationHandlerUnitTest,
       SubmitHumanConversationEntry_NoTitleGenerationAfterFirstTurn) {
  conversation_handler_->associated_content_manager()->ClearContent();

  MockEngineConsumer* engine = static_cast<MockEngineConsumer*>(
      conversation_handler_->GetEngineForTesting());

  NiceMock<MockConversationHandlerClient> client(conversation_handler_.get());

  // Engine requires title generation
  EXPECT_CALL(*engine, RequiresClientSideTitleGeneration())
      .WillRepeatedly(testing::Return(true));

  // First, set up a complete conversation with 2 turns (human + assistant)
  base::RunLoop first_loop;
  EXPECT_CALL(*engine, GenerateAssistantResponse)
      .WillOnce(testing::DoAll(
          testing::WithArg<7>(
              [](EngineConsumer::GenerationDataCallback callback) {
                std::move(callback).Run(EngineConsumer::GenerationResultData(
                    mojom::ConversationEntryEvent::NewCompletionEvent(
                        mojom::CompletionEvent::New("First response")),
                    std::nullopt));
              }),
          testing::WithArg<8>(
              [](EngineConsumer::GenerationCompletedCallback callback) {
                std::move(callback).Run(
                    base::ok(EngineConsumer::GenerationResultData(
                        mojom::ConversationEntryEvent::NewCompletionEvent(
                            mojom::CompletionEvent::New("")),
                        std::nullopt)));
              })));

  // Title generation should be called for first conversation (2 turns)
  EXPECT_CALL(*engine, GenerateConversationTitle)
      .WillOnce(testing::WithArg<2>(
          [&first_loop](EngineConsumer::GenerationCompletedCallback callback) {
            std::move(callback).Run(
                base::ok(EngineConsumer::GenerationResultData(
                    mojom::ConversationEntryEvent::NewConversationTitleEvent(
                        mojom::ConversationTitleEvent::New("First Title")),
                    std::nullopt)));
            first_loop.QuitWhenIdle();
          }));

  conversation_handler_->SubmitHumanConversationEntry("First question",
                                                      std::nullopt);
  first_loop.Run();
  testing::Mock::VerifyAndClearExpectations(engine);

  // Now submit second human entry - this should NOT trigger title generation
  base::RunLoop second_loop;
  EXPECT_CALL(*engine, GenerateAssistantResponse)
      .WillOnce(testing::DoAll(
          testing::WithArg<7>(
              [](EngineConsumer::GenerationDataCallback callback) {
                std::move(callback).Run(EngineConsumer::GenerationResultData(
                    mojom::ConversationEntryEvent::NewCompletionEvent(
                        mojom::CompletionEvent::New("Second response")),
                    std::nullopt));
              }),
          testing::WithArg<8>(
              [&second_loop](
                  EngineConsumer::GenerationCompletedCallback callback) {
                std::move(callback).Run(
                    base::ok(EngineConsumer::GenerationResultData(
                        mojom::ConversationEntryEvent::NewCompletionEvent(
                            mojom::CompletionEvent::New("")),
                        std::nullopt)));
                second_loop.QuitWhenIdle();
              })));

  // Title generation should NOT be called after the first turn (only on 2nd
  // turn)
  EXPECT_CALL(*engine, GenerateConversationTitle).Times(0);
  EXPECT_CALL(*engine, RequiresClientSideTitleGeneration())
      .WillRepeatedly(testing::Return(true));

  conversation_handler_->SubmitHumanConversationEntry("Second question",
                                                      std::nullopt);
  second_loop.Run();

  // Verify conversation has 4 turns
  const auto& history = conversation_handler_->GetConversationHistory();
  EXPECT_EQ(history.size(), 4u);
}

TEST_F(ConversationHandlerUnitTest,
       SubmitHumanConversationEntry_NoTitleWhenEngineDoesntRequire) {
  conversation_handler_->associated_content_manager()->ClearContent();

  MockEngineConsumer* engine = static_cast<MockEngineConsumer*>(
      conversation_handler_->GetEngineForTesting());

  NiceMock<MockConversationHandlerClient> client(conversation_handler_.get());

  // Engine does NOT require title generation
  EXPECT_CALL(*engine, RequiresClientSideTitleGeneration())
      .WillRepeatedly(testing::Return(false));

  base::RunLoop run_loop;

  EXPECT_CALL(*engine, GenerateAssistantResponse)
      .WillOnce(testing::DoAll(
          testing::WithArg<7>(
              [](EngineConsumer::GenerationDataCallback callback) {
                std::move(callback).Run(EngineConsumer::GenerationResultData(
                    mojom::ConversationEntryEvent::NewCompletionEvent(
                        mojom::CompletionEvent::New("Assistant response")),
                    std::nullopt));
              }),
          testing::WithArg<8>(
              [&run_loop](
                  EngineConsumer::GenerationCompletedCallback callback) {
                std::move(callback).Run(
                    base::ok(EngineConsumer::GenerationResultData(
                        mojom::ConversationEntryEvent::NewCompletionEvent(
                            mojom::CompletionEvent::New("")),
                        std::nullopt)));
                run_loop.QuitWhenIdle();
              })));

  // Title generation should NOT be called
  EXPECT_CALL(*engine, GenerateConversationTitle).Times(0);

  EXPECT_CALL(client, OnAPIRequestInProgress(true)).Times(1);
  EXPECT_CALL(client, OnAPIRequestInProgress(false)).Times(1);

  conversation_handler_->SubmitHumanConversationEntry("Test question",
                                                      std::nullopt);
  run_loop.Run();

  // Verify conversation has 2 turns but no title generation occurred
  const auto& history = conversation_handler_->GetConversationHistory();
  EXPECT_EQ(history.size(), 2u);
}

TEST_F(ConversationHandlerUnitTest,
       SubmitHumanConversationEntry_TitleGenerationFailure) {
  conversation_handler_->associated_content_manager()->ClearContent();

  MockEngineConsumer* engine = static_cast<MockEngineConsumer*>(
      conversation_handler_->GetEngineForTesting());

  NiceMock<MockConversationHandlerClient> client(conversation_handler_.get());
  testing::NiceMock<MockConversationHandlerObserver> observer;
  observer.Observe(conversation_handler_.get());

  // Engine requires title generation
  EXPECT_CALL(*engine, RequiresClientSideTitleGeneration())
      .WillRepeatedly(testing::Return(true));

  base::RunLoop run_loop;

  EXPECT_CALL(*engine, GenerateAssistantResponse)
      .WillOnce(testing::DoAll(
          testing::WithArg<7>(
              [](EngineConsumer::GenerationDataCallback callback) {
                std::move(callback).Run(EngineConsumer::GenerationResultData(
                    mojom::ConversationEntryEvent::NewCompletionEvent(
                        mojom::CompletionEvent::New("Assistant response")),
                    std::nullopt));
              }),
          testing::WithArg<8>(
              [](EngineConsumer::GenerationCompletedCallback callback) {
                std::move(callback).Run(
                    base::ok(EngineConsumer::GenerationResultData(
                        mojom::ConversationEntryEvent::NewCompletionEvent(
                            mojom::CompletionEvent::New("")),
                        std::nullopt)));
              })));

  EXPECT_CALL(*engine, GenerateConversationTitle)
      .WillOnce(testing::WithArg<2>(
          [&run_loop](EngineConsumer::GenerationCompletedCallback callback) {
            // Mock title generation failure
            std::move(callback).Run(
                base::unexpected(mojom::APIError::ConnectionIssue));
            run_loop.QuitWhenIdle();
          }));

  // Title failure should be handled silently - no error should be set
  EXPECT_CALL(client, OnAPIRequestInProgress(true)).Times(1);
  EXPECT_CALL(client, OnAPIRequestInProgress(false)).Times(1);
  EXPECT_CALL(observer, OnConversationTitleChanged).Times(0);

  conversation_handler_->SubmitHumanConversationEntry("Test question",
                                                      std::nullopt);
  run_loop.Run();

  // Verify conversation still completes successfully despite title failure
  const auto& history = conversation_handler_->GetConversationHistory();
  EXPECT_EQ(history.size(), 2u);
  EXPECT_EQ(conversation_handler_->current_error(), mojom::APIError::None);
}

TEST_F(ConversationHandlerUnitTest,
       SubmitHumanConversationEntry_AssistantResponseFailure) {
  conversation_handler_->associated_content_manager()->ClearContent();

  MockEngineConsumer* engine = static_cast<MockEngineConsumer*>(
      conversation_handler_->GetEngineForTesting());

  NiceMock<MockConversationHandlerClient> client(conversation_handler_.get());

  // Engine requires title generation
  EXPECT_CALL(*engine, RequiresClientSideTitleGeneration())
      .WillRepeatedly(testing::Return(true));

  base::RunLoop run_loop;

  EXPECT_CALL(*engine, GenerateAssistantResponse)
      .WillOnce(testing::WithArg<8>(
          [&run_loop](EngineConsumer::GenerationCompletedCallback callback) {
            // Mock assistant response failure
            std::move(callback).Run(
                base::unexpected(mojom::APIError::ConnectionIssue));
            run_loop.QuitWhenIdle();
          }));

  // Title generation should NOT be called when assistant response fails
  EXPECT_CALL(*engine, GenerateConversationTitle).Times(0);

  EXPECT_CALL(client, OnAPIRequestInProgress(true)).Times(1);
  EXPECT_CALL(client, OnAPIRequestInProgress(false)).Times(1);

  conversation_handler_->SubmitHumanConversationEntry("Test question",
                                                      std::nullopt);
  run_loop.Run();

  // Verify error is set and conversation has only human entry
  const auto& history = conversation_handler_->GetConversationHistory();
  EXPECT_EQ(history.size(), 1u);
  EXPECT_EQ(history[0]->character_type, mojom::CharacterType::HUMAN);
  EXPECT_EQ(conversation_handler_->current_error(),
            mojom::APIError::ConnectionIssue);
}

TEST_F(ConversationHandlerUnitTest,
       SubmitHumanConversationEntryWithSkill_ValidSkill) {
  conversation_handler_->associated_content_manager()->ClearContent();

  // Add a skill to prefs
  prefs::AddSkillToPrefs("playlist", "Create a playlist of 10 songs",
                         std::nullopt /* model */, prefs_);
  auto skills = prefs::GetSkillsFromPrefs(prefs_);
  ASSERT_EQ(skills.size(), 1u);
  std::string skill_id = skills[0]->id;

  // Get initial timestamps
  base::Time created_time = skills[0]->created_time;
  base::Time initial_last_used = skills[0]->last_used;

  MockEngineConsumer* engine = static_cast<MockEngineConsumer*>(
      conversation_handler_->GetEngineForTesting());
  NiceMock<MockConversationHandlerClient> client(conversation_handler_.get());

  base::RunLoop run_loop;

  // Mock successful response
  EXPECT_CALL(*engine, GenerateAssistantResponse)
      .WillOnce(testing::WithArg<8>(
          [&run_loop](EngineConsumer::GenerationCompletedCallback callback) {
            std::move(callback).Run(EngineConsumer::GenerationResultData(
                mojom::ConversationEntryEvent::NewCompletionEvent(
                    mojom::CompletionEvent::New("Test response")),
                std::nullopt));
            run_loop.QuitWhenIdle();
          }));

  conversation_handler_->SubmitHumanConversationEntryWithSkill("/playlist 2000",
                                                               skill_id);

  run_loop.Run();

  // Verify conversation history contains skill data
  const auto& history = conversation_handler_->GetConversationHistory();
  EXPECT_EQ(history.size(), 2u);
  EXPECT_EQ(history[0]->text, "/playlist 2000");
  ASSERT_TRUE(history[0]->skill);
  EXPECT_EQ(history[0]->skill->shortcut, "playlist");
  EXPECT_EQ(history[0]->skill->prompt, "Create a playlist of 10 songs");

  // Verify last_used time was updated
  auto updated_mode = prefs::GetSkillFromPrefs(prefs_, skill_id);
  ASSERT_TRUE(updated_mode);
  EXPECT_NE(updated_mode->last_used, created_time);
  EXPECT_GT(updated_mode->last_used, initial_last_used);
  EXPECT_EQ(updated_mode->created_time, created_time);
}

TEST_F(ConversationHandlerUnitTest,
       SubmitHumanConversationEntryWithSkill_InvalidSkill) {
  // Test invalid skill will just be ignored and submit the input text
  // as plain text without skill message.
  conversation_handler_->associated_content_manager()->ClearContent();

  MockEngineConsumer* engine = static_cast<MockEngineConsumer*>(
      conversation_handler_->GetEngineForTesting());
  NiceMock<MockConversationHandlerClient> client(conversation_handler_.get());

  base::RunLoop run_loop;

  // Engine should still be called (invalid skill is silently ignored)
  EXPECT_CALL(*engine, GenerateAssistantResponse)
      .WillOnce(testing::WithArg<8>(
          [&run_loop](EngineConsumer::GenerationCompletedCallback callback) {
            std::move(callback).Run(EngineConsumer::GenerationResultData(
                mojom::ConversationEntryEvent::NewCompletionEvent(
                    mojom::CompletionEvent::New("Test response")),
                std::nullopt));
            run_loop.QuitWhenIdle();
          }));

  conversation_handler_->SubmitHumanConversationEntryWithSkill(
      "Test input", "invalid-mode-id");

  run_loop.Run();

  // Verify conversation history created but without skill data
  const auto& history = conversation_handler_->GetConversationHistory();
  EXPECT_EQ(history.size(), 2u);

  // Human entry should NOT have skill data (invalid ID was ignored)
  EXPECT_FALSE(history[0]->skill);
  EXPECT_EQ(history[0]->text, "Test input");

  // Assistant response should be normal
  EXPECT_EQ(history[1]->character_type, mojom::CharacterType::ASSISTANT);
}

TEST_F(ConversationHandlerUnitTest,
       SubmitHumanConversationEntryWithSkill_ModelSwitching) {
  conversation_handler_->associated_content_manager()->ClearContent();

  // Add a skill with different model
  std::string different_model = "chat-gemma";
  prefs::AddSkillToPrefs("translate", "Please translate the content",
                         different_model, prefs_);
  auto skills = prefs::GetSkillsFromPrefs(prefs_);
  ASSERT_EQ(skills.size(), 1u);
  std::string skill_id = skills[0]->id;

  NiceMock<MockConversationHandlerClient> client(conversation_handler_.get());
  base::RunLoop run_loop;

  // Wait for model change notification and verify model switched
  EXPECT_CALL(client, OnModelDataChanged)
      .WillOnce(base::test::RunClosure(base::BindLambdaForTesting([&]() {
        // Verify model was switched
        EXPECT_EQ(conversation_handler_->GetCurrentModel().key,
                  different_model);
        run_loop.Quit();
      })));

  conversation_handler_->SubmitHumanConversationEntryWithSkill("Test input",
                                                               skill_id);

  run_loop.Run();
}

TEST_F(ConversationHandlerUnitTest,
       SubmitHumanConversationEntryWithSkill_NoModelSwitchingSameModel) {
  conversation_handler_->associated_content_manager()->ClearContent();

  // Get current model key
  std::string current_model = conversation_handler_->GetCurrentModel().key;

  // Add a skill with same model as current
  prefs::AddSkillToPrefs("rewrite", "Please rewrite the content", current_model,
                         prefs_);
  auto skills = prefs::GetSkillsFromPrefs(prefs_);
  ASSERT_EQ(skills.size(), 1u);
  std::string skill_id = skills[0]->id;

  MockEngineConsumer* engine = static_cast<MockEngineConsumer*>(
      conversation_handler_->GetEngineForTesting());
  NiceMock<MockConversationHandlerClient> client(conversation_handler_.get());

  base::RunLoop run_loop;

  // Model change notification should NOT be called
  EXPECT_CALL(client, OnModelDataChanged).Times(0);

  // Mock successful response
  EXPECT_CALL(*engine, GenerateAssistantResponse)
      .WillOnce(testing::WithArg<8>(
          [&run_loop](EngineConsumer::GenerationCompletedCallback callback) {
            std::move(callback).Run(EngineConsumer::GenerationResultData(
                mojom::ConversationEntryEvent::NewCompletionEvent(
                    mojom::CompletionEvent::New("Test response")),
                std::nullopt));
            run_loop.QuitWhenIdle();
          }));

  conversation_handler_->SubmitHumanConversationEntryWithSkill("Test input",
                                                               skill_id);

  run_loop.Run();

  // Verify model remained the same
  EXPECT_EQ(conversation_handler_->GetCurrentModel().key, current_model);
}

TEST_F(ConversationHandlerUnitTest,
       PermissionChallenge_ExistingChallengeHaltsExecution) {
  // Test that if a ToolUseEvent already has a permission_challenge that hasn't
  // been granted, tool execution is halted at that tool.
  conversation_handler_->associated_content_manager()->ClearContent();
  MockEngineConsumer* engine = static_cast<MockEngineConsumer*>(
      conversation_handler_->GetEngineForTesting());

  auto tool1 = std::make_unique<NiceMock<MockTool>>("test_tool", "Test tool");

  ON_CALL(*mock_tool_provider_, GetTools()).WillByDefault([&]() {
    std::vector<base::WeakPtr<Tool>> tools;
    tools.push_back(tool1->GetWeakPtr());
    return tools;
  });

  NiceMock<MockConversationHandlerClient> client(conversation_handler_.get());

  base::RunLoop run_loop;

  // Engine returns tool use event with permission challenge already set
  // (simulating server alignment check blocking the tool)
  EXPECT_CALL(*engine, GenerateAssistantResponse)
      .WillOnce(testing::DoAll(
          testing::WithArg<7>(
              [](EngineConsumer::GenerationDataCallback callback) {
                auto tool_use = mojom::ToolUseEvent::New(
                    "test_tool", "tool_id_1", "{\"param\":\"value\"}",
                    std::nullopt,
                    mojom::PermissionChallenge::New(
                        false,  // user_allows
                        "Server determined this tool use "
                        "is off-topic",  // assessment
                        std::nullopt));  // plan
                callback.Run(EngineConsumer::GenerationResultData(
                    mojom::ConversationEntryEvent::NewToolUseEvent(
                        std::move(tool_use)),
                    std::nullopt));
              }),
          testing::WithArg<8>(
              [&](EngineConsumer::GenerationCompletedCallback callback) {
                std::move(callback).Run(
                    base::ok(EngineConsumer::GenerationResultData(
                        mojom::ConversationEntryEvent::NewCompletionEvent(
                            mojom::CompletionEvent::New("")),
                        std::nullopt)));
                run_loop.QuitWhenIdle();
              })));

  // Tool should NOT be called since permission challenge is blocking
  EXPECT_CALL(*tool1, UseTool).Times(0);

  conversation_handler_->SubmitHumanConversationEntry("Test question",
                                                      std::nullopt);
  run_loop.Run();

  // Verify the tool use event exists with permission challenge
  const auto& history = conversation_handler_->GetConversationHistory();
  ASSERT_EQ(history.size(), 2u);
  auto& assistant_entry = history.back();
  ASSERT_TRUE(assistant_entry->events.has_value());
  auto& events = assistant_entry->events.value();
  ASSERT_EQ(events.size(), 1u);
  EXPECT_TRUE(events[0]->is_tool_use_event());
  auto& tool_event = events[0]->get_tool_use_event();
  EXPECT_FALSE(tool_event->output.has_value());  // No output yet
  ASSERT_TRUE(tool_event->permission_challenge);
  EXPECT_FALSE(tool_event->permission_challenge->user_allows);
  EXPECT_EQ(tool_event->permission_challenge->assessment,
            "Server determined this tool use is off-topic");
}

TEST_F(ConversationHandlerUnitTest, PermissionChallenge_ToolReturnsChallenge) {
  // Test that when a tool's RequiresUserInteractionBeforeHandling returns
  // a permission challenge, tool execution is halted.
  conversation_handler_->associated_content_manager()->ClearContent();
  MockEngineConsumer* engine = static_cast<MockEngineConsumer*>(
      conversation_handler_->GetEngineForTesting());

  // Create a tool that returns a permission challenge
  auto tool1 = std::make_unique<NiceMock<MockTool>>("test_tool", "Test tool");

  // Mock RequiresUserInteractionBeforeHandling to return a challenge
  ON_CALL(*tool1, RequiresUserInteractionBeforeHandling)
      .WillByDefault([](const mojom::ToolUseEvent& tool_use) {
        return std::variant<bool, mojom::PermissionChallengePtr>(
            mojom::PermissionChallenge::New(
                false,                                    // user_allows
                std::nullopt,                             // assessment
                "This tool needs to manage your tabs"));  // plan
      });

  ON_CALL(*mock_tool_provider_, GetTools()).WillByDefault([&]() {
    std::vector<base::WeakPtr<Tool>> tools;
    tools.push_back(tool1->GetWeakPtr());
    return tools;
  });

  NiceMock<MockConversationHandlerClient> client(conversation_handler_.get());

  base::RunLoop run_loop;

  // Engine returns tool use event without permission challenge
  EXPECT_CALL(*engine, GenerateAssistantResponse)
      .WillOnce(testing::DoAll(
          testing::WithArg<7>(
              [](EngineConsumer::GenerationDataCallback callback) {
                callback.Run(EngineConsumer::GenerationResultData(
                    mojom::ConversationEntryEvent::NewToolUseEvent(
                        mojom::ToolUseEvent::New("test_tool", "tool_id_1",
                                                 "{\"param\":\"value\"}",
                                                 std::nullopt, nullptr)),
                    std::nullopt));
              }),
          testing::WithArg<8>(
              [&](EngineConsumer::GenerationCompletedCallback callback) {
                std::move(callback).Run(
                    base::ok(EngineConsumer::GenerationResultData(
                        mojom::ConversationEntryEvent::NewCompletionEvent(
                            mojom::CompletionEvent::New("")),
                        std::nullopt)));
                run_loop.QuitWhenIdle();
              })));

  // Tool should NOT be called since permission challenge is returned
  EXPECT_CALL(*tool1, UseTool).Times(0);

  conversation_handler_->SubmitHumanConversationEntry("Test question",
                                                      std::nullopt);
  run_loop.Run();

  // Verify the tool use event now has permission challenge
  const auto& history = conversation_handler_->GetConversationHistory();
  ASSERT_EQ(history.size(), 2u);
  auto& assistant_entry = history.back();
  ASSERT_TRUE(assistant_entry->events.has_value());
  auto& events = assistant_entry->events.value();
  ASSERT_EQ(events.size(), 1u);
  EXPECT_TRUE(events[0]->is_tool_use_event());
  auto& tool_event = events[0]->get_tool_use_event();
  EXPECT_FALSE(tool_event->output.has_value());  // No output yet
  ASSERT_TRUE(tool_event->permission_challenge);
  EXPECT_FALSE(tool_event->permission_challenge->user_allows);
  EXPECT_EQ(tool_event->permission_challenge->plan,
            "This tool needs to manage your tabs");
}

TEST_F(ConversationHandlerUnitTest, PermissionChallenge_UserDeniesPermission) {
  // Test that when user denies permission, a denial response is sent to the
  // engine and pending tool requests are not processed.
  conversation_handler_->associated_content_manager()->ClearContent();
  MockEngineConsumer* engine = static_cast<MockEngineConsumer*>(
      conversation_handler_->GetEngineForTesting());

  auto tool1 = std::make_unique<NiceMock<MockTool>>("tool1", "Tool 1");
  auto tool2 = std::make_unique<NiceMock<MockTool>>("tool2", "Tool 2");

  ON_CALL(*mock_tool_provider_, GetTools()).WillByDefault([&]() {
    std::vector<base::WeakPtr<Tool>> tools;
    tools.push_back(tool1->GetWeakPtr());
    tools.push_back(tool2->GetWeakPtr());
    return tools;
  });

  // Tool1 requires permission
  ON_CALL(*tool1, RequiresUserInteractionBeforeHandling)
      .WillByDefault([](const mojom::ToolUseEvent& tool_use) {
        return std::variant<bool, mojom::PermissionChallengePtr>(
            mojom::PermissionChallenge::New(false, std::nullopt,
                                            "Needs permission"));
      });

  NiceMock<MockConversationHandlerClient> client(conversation_handler_.get());

  base::RunLoop first_loop;
  testing::Sequence seq;

  // Engine returns two tool use events
  EXPECT_CALL(*engine, GenerateAssistantResponse)
      .InSequence(seq)
      .WillOnce(testing::DoAll(
          testing::WithArg<7>(
              [](EngineConsumer::GenerationDataCallback callback) {
                // First tool use
                callback.Run(EngineConsumer::GenerationResultData(
                    mojom::ConversationEntryEvent::NewToolUseEvent(
                        mojom::ToolUseEvent::New("tool1", "tool_id_1", "{}",
                                                 std::nullopt, nullptr)),
                    std::nullopt));
                // Second tool use
                callback.Run(EngineConsumer::GenerationResultData(
                    mojom::ConversationEntryEvent::NewToolUseEvent(
                        mojom::ToolUseEvent::New("tool2", "tool_id_2", "{}",
                                                 std::nullopt, nullptr)),
                    std::nullopt));
              }),
          testing::WithArg<8>(
              [&](EngineConsumer::GenerationCompletedCallback callback) {
                std::move(callback).Run(
                    base::ok(EngineConsumer::GenerationResultData(
                        mojom::ConversationEntryEvent::NewCompletionEvent(
                            mojom::CompletionEvent::New("")),
                        std::nullopt)));
                first_loop.Quit();
              })));

  conversation_handler_->SubmitHumanConversationEntry("Test", std::nullopt);
  first_loop.Run();

  // Verify first tool has permission challenge, second tool has no output yet
  const auto& history_before = conversation_handler_->GetConversationHistory();
  ASSERT_EQ(history_before.size(), 2u);
  auto& assistant_before = history_before.back();
  ASSERT_TRUE(assistant_before->events.has_value());
  auto& events_before = assistant_before->events.value();
  ASSERT_EQ(events_before.size(), 2u);
  EXPECT_TRUE(events_before[0]->get_tool_use_event()->permission_challenge);
  EXPECT_FALSE(events_before[1]->get_tool_use_event()->output.has_value());

  // User denies permission
  base::RunLoop second_loop;

  // Engine should be called with the denial response and perform next
  // generation
  EXPECT_CALL(*engine, GenerateAssistantResponse)
      .InSequence(seq)
      .WillOnce(testing::DoAll(
          testing::WithArg<7>(
              [](EngineConsumer::GenerationDataCallback callback) {
                callback.Run(EngineConsumer::GenerationResultData(
                    mojom::ConversationEntryEvent::NewCompletionEvent(
                        mojom::CompletionEvent::New(
                            "Understood, I won't proceed.")),
                    std::nullopt));
              }),
          testing::WithArg<8>(
              [&](EngineConsumer::GenerationCompletedCallback callback) {
                std::move(callback).Run(
                    base::ok(EngineConsumer::GenerationResultData(
                        mojom::ConversationEntryEvent::NewCompletionEvent(
                            mojom::CompletionEvent::New("")),
                        std::nullopt)));
                second_loop.Quit();
              })));

  // Neither tool should be called
  EXPECT_CALL(*tool1, UseTool).Times(0);
  EXPECT_CALL(*tool2, UseTool).Times(0);

  // User denies permission
  conversation_handler_->ProcessPermissionChallenge("tool_id_1", false);
  second_loop.Run();

  // Verify first tool has denial output, second tool was not processed
  const auto& history_after = conversation_handler_->GetConversationHistory();
  auto& assistant_after = history_after[1];
  ASSERT_TRUE(assistant_after->events.has_value());
  auto& events_after = assistant_after->events.value();

  // First tool should have denial output
  ASSERT_TRUE(events_after[0]->get_tool_use_event()->output.has_value());
  auto& output = events_after[0]->get_tool_use_event()->output.value();
  EXPECT_EQ(output.size(), 1u);
  EXPECT_TRUE(output[0]->is_text_content_block());
  EXPECT_EQ(output[0]->get_text_content_block()->text,
            "Permission to use this tool with these arguments was denied by "
            "the user.");

  // Second tool should not have output (was not processed)
  EXPECT_FALSE(events_after[1]->get_tool_use_event()->output.has_value());
}

TEST_F(ConversationHandlerUnitTest,
       PermissionChallenge_UserAllowsPermissionContinuesExecution) {
  // Test that when user allows permission, the tool is executed and
  // subsequent tool use requests are processed.
  conversation_handler_->associated_content_manager()->ClearContent();
  MockEngineConsumer* engine = static_cast<MockEngineConsumer*>(
      conversation_handler_->GetEngineForTesting());

  auto tool1 = std::make_unique<NiceMock<MockTool>>("tool1", "Tool 1");
  auto tool2 = std::make_unique<NiceMock<MockTool>>("tool2", "Tool 2");

  ON_CALL(*mock_tool_provider_, GetTools()).WillByDefault([&]() {
    std::vector<base::WeakPtr<Tool>> tools;
    tools.push_back(tool1->GetWeakPtr());
    tools.push_back(tool2->GetWeakPtr());
    return tools;
  });

  // Tool1 requires permission
  ON_CALL(*tool1, RequiresUserInteractionBeforeHandling)
      .WillByDefault([](const mojom::ToolUseEvent& tool_use) {
        return std::variant<bool, mojom::PermissionChallengePtr>(
            mojom::PermissionChallenge::New(false, std::nullopt,
                                            "Needs permission"));
      });

  NiceMock<MockConversationHandlerClient> client(conversation_handler_.get());

  base::RunLoop first_loop;
  testing::Sequence seq;

  // Engine returns two tool use events
  EXPECT_CALL(*engine, GenerateAssistantResponse)
      .InSequence(seq)
      .WillOnce(testing::DoAll(
          testing::WithArg<7>(
              [](EngineConsumer::GenerationDataCallback callback) {
                // First tool use
                callback.Run(EngineConsumer::GenerationResultData(
                    mojom::ConversationEntryEvent::NewToolUseEvent(
                        mojom::ToolUseEvent::New("tool1", "tool_id_1",
                                                 "{\"input\":\"test1\"}",
                                                 std::nullopt, nullptr)),
                    std::nullopt));
                // Second tool use
                callback.Run(EngineConsumer::GenerationResultData(
                    mojom::ConversationEntryEvent::NewToolUseEvent(
                        mojom::ToolUseEvent::New("tool2", "tool_id_2",
                                                 "{\"input\":\"test2\"}",
                                                 std::nullopt, nullptr)),
                    std::nullopt));
              }),
          testing::WithArg<8>(
              [&](EngineConsumer::GenerationCompletedCallback callback) {
                std::move(callback).Run(
                    base::ok(EngineConsumer::GenerationResultData(
                        mojom::ConversationEntryEvent::NewCompletionEvent(
                            mojom::CompletionEvent::New("")),
                        std::nullopt)));
                first_loop.Quit();
              })));

  conversation_handler_->SubmitHumanConversationEntry("Test", std::nullopt);
  first_loop.Run();

  // Setup tool responses
  base::RunLoop second_loop;

  // Tool1 should be called after permission is granted
  EXPECT_CALL(*tool1, UseTool(StrEq("{\"input\":\"test1\"}"), _))
      .InSequence(seq)
      .WillOnce(testing::WithArg<1>([](Tool::UseToolCallback callback) {
        std::vector<mojom::ContentBlockPtr> result;
        result.push_back(mojom::ContentBlock::NewTextContentBlock(
            mojom::TextContentBlock::New("Result from tool1")));
        std::move(callback).Run(std::move(result));
      }));

  // Tool2 should be called automatically after tool1 completes
  EXPECT_CALL(*tool2, UseTool(StrEq("{\"input\":\"test2\"}"), _))
      .InSequence(seq)
      .WillOnce(testing::WithArg<1>([](Tool::UseToolCallback callback) {
        std::vector<mojom::ContentBlockPtr> result;
        result.push_back(mojom::ContentBlock::NewTextContentBlock(
            mojom::TextContentBlock::New("Result from tool2")));
        std::move(callback).Run(std::move(result));
      }));

  // Final generation after both tools complete
  EXPECT_CALL(*engine, GenerateAssistantResponse)
      .InSequence(seq)
      .WillOnce(testing::DoAll(
          testing::WithArg<7>(
              [](EngineConsumer::GenerationDataCallback callback) {
                callback.Run(EngineConsumer::GenerationResultData(
                    mojom::ConversationEntryEvent::NewCompletionEvent(
                        mojom::CompletionEvent::New("Final response")),
                    std::nullopt));
              }),
          testing::WithArg<8>(
              [&](EngineConsumer::GenerationCompletedCallback callback) {
                std::move(callback).Run(
                    base::ok(EngineConsumer::GenerationResultData(
                        mojom::ConversationEntryEvent::NewCompletionEvent(
                            mojom::CompletionEvent::New("")),
                        std::nullopt)));
                second_loop.Quit();
              })));

  // User approves permission
  conversation_handler_->ProcessPermissionChallenge("tool_id_1", true);
  second_loop.Run();

  // Verify both tools were executed and have outputs
  const auto& history = conversation_handler_->GetConversationHistory();
  auto& assistant_entry = history[1];
  ASSERT_TRUE(assistant_entry->events.has_value());
  auto& events = assistant_entry->events.value();
  ASSERT_EQ(events.size(), 2u);

  // First tool should have output
  ASSERT_TRUE(events[0]->get_tool_use_event()->output.has_value());
  EXPECT_MOJOM_EQ(events[0]->get_tool_use_event()->output->at(0),
                  mojom::ContentBlock::NewTextContentBlock(
                      mojom::TextContentBlock::New("Result from tool1")));

  // Second tool should have output
  ASSERT_TRUE(events[1]->get_tool_use_event()->output.has_value());
  EXPECT_MOJOM_EQ(events[1]->get_tool_use_event()->output->at(0),
                  mojom::ContentBlock::NewTextContentBlock(
                      mojom::TextContentBlock::New("Result from tool2")));

  // Permission challenge should be marked as granted
  ASSERT_TRUE(events[0]->get_tool_use_event()->permission_challenge);
  EXPECT_TRUE(
      events[0]->get_tool_use_event()->permission_challenge->user_allows);

  // Final response should be present
  EXPECT_EQ(history.back()->text, "Final response");
}

}  // namespace ai_chat
