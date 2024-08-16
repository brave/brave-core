/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/conversation_driver.h"

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
#include "base/scoped_observation.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool.h"
#include "base/test/bind.h"
#include "base/test/gmock_callback_support.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "brave/components/ai_chat/core/browser/ai_chat_credential_manager.h"
#include "brave/components/ai_chat/core/browser/text_embedder.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "components/grit/brave_components_strings.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/url_loader_completion_status.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "services/network/test/test_url_loader_factory.h"
#include "services/network/test/test_utils.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/gurl.h"

using ConversationHistory = std::vector<ai_chat::mojom::ConversationTurn>;
using ::testing::_;

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

class MockEngineConsumer : public EngineConsumer {
 public:
  MOCK_METHOD(void,
              GenerateQuestionSuggestions,
              (const bool&, const std::string&, SuggestedQuestionsCallback),
              (override));
  MOCK_METHOD(void,
              GenerateAssistantResponse,
              (const bool&,
               const std::string&,
               const ConversationHistory&,
               const std::string&,
               GenerationDataCallback,
               GenerationCompletedCallback),
              (override));
  MOCK_METHOD(void,
              GenerateRewriteSuggestion,
              (std::string,
               const std::string&,
               GenerationDataCallback,
               GenerationCompletedCallback),
              (override));
  MOCK_METHOD(void, SanitizeInput, (std::string&), (override));
  MOCK_METHOD(void, ClearAllQueries, (), (override));

  bool SupportsDeltaTextResponses() const override {
    return supports_delta_text_responses_;
  }

  void SetSupportsDeltaTextResponses(bool supports_delta_text_responses) {
    supports_delta_text_responses_ = supports_delta_text_responses;
  }

  void UpdateModelOptions(const mojom::ModelOptions& options) override {}

 private:
  bool supports_delta_text_responses_ = false;
};

class MockAIChatCredentialManager : public AIChatCredentialManager {
 public:
  using AIChatCredentialManager::AIChatCredentialManager;
  MOCK_METHOD(void,
              GetPremiumStatus,
              (mojom::PageHandler::GetPremiumStatusCallback callback),
              (override));
};

class MockConversationDriverObserver : public ConversationDriver::Observer {
 public:
  explicit MockConversationDriverObserver(ConversationDriver* driver) {
    observation_.Observe(driver);
  }

  ~MockConversationDriverObserver() override = default;

  MOCK_METHOD(void, OnAPIRequestInProgress, (bool in_progress), (override));
  MOCK_METHOD(void, OnHistoryUpdate, (), (override));

 private:
  base::ScopedObservation<ConversationDriver, ConversationDriver::Observer>
      observation_{this};
};

class MockConversationDriver : public ConversationDriver {
 public:
  MockConversationDriver(
      PrefService* profile_prefs,
      PrefService* local_state,
      ModelService* model_service,
      AIChatMetrics* ai_chat_metrics,
      LeoLocalModelsUpdater* leo_local_models_updater,
      std::unique_ptr<AIChatCredentialManager> credential_manager,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      std::string_view channel_string)
      : ConversationDriver(profile_prefs,
                           local_state,
                           model_service,
                           ai_chat_metrics,
                           leo_local_models_updater,
                           std::move(credential_manager),
                           url_loader_factory,
                           channel_string) {}
  ~MockConversationDriver() override = default;

  MOCK_METHOD(GURL, GetPageURL, (), (const, override));
  MOCK_METHOD(std::u16string, GetPageTitle, (), (const, override));
  MOCK_METHOD(void,
              GetPageContent,
              (GetPageContentCallback, std::string_view),
              (override));
  MOCK_METHOD(void, PrintPreviewFallback, (GetPageContentCallback), (override));
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

class MockLeoLocalModelsUpdater : public LeoLocalModelsUpdater {
 public:
  MockLeoLocalModelsUpdater()
      : LeoLocalModelsUpdater(nullptr, base::FilePath()) {}
  ~MockLeoLocalModelsUpdater() override = default;

  MOCK_METHOD(void, Register, (), (override));
  MOCK_METHOD(const base::FilePath&,
              GetUniversalQAModel,
              (),
              (const, override));
};

}  // namespace

class ConversationDriverUnitTest : public testing::Test {
 public:
  ConversationDriverUnitTest() {
    // TODO(petemill): Mock the engine requests so that we are not dependent on
    // specific network API calls for any particular engine. This test only
    // specifies the completion API responses and so doesn't work with the
    // Conversation API engine.
    features_.InitAndEnableFeatureWithParameters(
        features::kAIChat, {{features::kConversationAPIEnabled.name, "false"}});
  }
  ~ConversationDriverUnitTest() override = default;

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
        .WillByDefault(
            [&](mojom::PageHandler::GetPremiumStatusCallback callback) {
              mojom::PremiumInfoPtr premium_info = mojom::PremiumInfo::New();
              std::move(callback).Run(is_premium_
                                          ? mojom::PremiumStatus::Active
                                          : mojom::PremiumStatus::Inactive,
                                      std::move(premium_info));
            });

    service_ = std::make_unique<ModelService>(&prefs_);

    if (!default_model_key_.empty()) {
      service_->SetDefaultModelKeyWithoutValidationForTesting(
          default_model_key_);
    }

    leo_local_models_updater_ = std::make_unique<MockLeoLocalModelsUpdater>();

    conversation_driver_ = std::make_unique<MockConversationDriver>(
        &prefs_, &local_state_, service_.get(), nullptr,
        leo_local_models_updater_.get(), std::move(credential_manager),
        shared_url_loader_factory_, "");
  }

  void EmulateUserOptedIn() {
    // Mimic opening panel and user opted in.
    conversation_driver_->OnConversationActiveChanged(true);
    conversation_driver_->SetUserOptedIn(true);
  }

  // Waiting for is_request_in_progress_ to be false.
  void WaitForOnEngineCompletionComplete() { task_environment_.RunUntilIdle(); }

  void TearDown() override {}

 protected:
  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<ModelService> service_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;
  base::test::ScopedFeatureList features_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
  std::unique_ptr<MockLeoLocalModelsUpdater> leo_local_models_updater_;
  std::unique_ptr<MockConversationDriver> conversation_driver_;
  std::string default_model_key_;
  bool is_premium_ = false;
};

// Test fixture which emulates premium user state
class ConversationDriverUnitTest_Premium : public ConversationDriverUnitTest {
 public:
  using ConversationDriverUnitTest::ConversationDriverUnitTest;

  void SetUp() override {
    is_premium_ = true;
    ConversationDriverUnitTest::SetUp();
  }
};

// Test fixture which emulates user pref for obsolete claude instant default
// model.
class ConversationDriverUnitTest_ClaudeInstant
    : public ConversationDriverUnitTest {
 public:
  using ConversationDriverUnitTest::ConversationDriverUnitTest;

  void SetUp() override {
    default_model_key_ = "chat-claude-instant";
    ConversationDriverUnitTest::SetUp();
  }
};

// Test fixture which emulates user pref for obsolete claude instant default
// model and a premium user state.
class ConversationDriverUnitTest_PremiumClaudeInstant
    : public ConversationDriverUnitTest_Premium {
 public:
  using ConversationDriverUnitTest_Premium::ConversationDriverUnitTest_Premium;

  void SetUp() override {
    default_model_key_ = "chat-claude-instant";
    ConversationDriverUnitTest_Premium::SetUp();
  }
};

TEST_F(ConversationDriverUnitTest_ClaudeInstant, Construction_Migrate) {
  // Test that obsolete "chat-claude-instant" is correctly migrated for
  // non-premium users.
  task_environment_.RunUntilIdle();
  EXPECT_EQ(conversation_driver_->GetCurrentModel().key, "chat-claude-haiku");
}

TEST_F(ConversationDriverUnitTest_PremiumClaudeInstant, Construction_Migrate) {
  // Test that obsolete "chat-claude-instant" is correctly migrated for premium
  // users.
  task_environment_.RunUntilIdle();
  EXPECT_EQ(conversation_driver_->GetCurrentModel().key, "chat-claude-sonnet");
}

TEST_F(ConversationDriverUnitTest, SubmitSelectedText) {
  bool is_page_content_linked = false;
  url_loader_factory_.SetInterceptor(
      base::BindLambdaForTesting([&](const network::ResourceRequest& request) {
        std::string body = network::GetUploadData(request);
        EXPECT_NE(body.find("I have spoken."), std::string::npos);
        EXPECT_NE(body.find(l10n_util::GetStringUTF8(
                      IDS_AI_CHAT_QUESTION_SUMMARIZE_SELECTED_TEXT)),
                  std::string::npos);
        if (is_page_content_linked) {
          EXPECT_NE(body.find("The child's name is Grogu."), std::string::npos);
        }
        // Set header for enabling SSE.
        auto head = network::mojom::URLResponseHead::New();
        head->mime_type = "text/event-stream";
        url_loader_factory_.ClearResponses();
        url_loader_factory_.AddResponse(
            request.url, std::move(head),
            R"(data: {"completion": "This is the way.", "stop": null})",
            network::URLLoaderCompletionStatus());
      }));

  EmulateUserOptedIn();

  // 1. Test without page contents.
  EXPECT_TRUE(conversation_driver_->GetShouldSendPageContents());
  conversation_driver_->SetShouldSendPageContents(false);

  MockConversationDriverObserver observer(conversation_driver_.get());
  // One from SubmitHumanConversationEntry, one from
  // OnEngineCompletionDataReceived.
  EXPECT_CALL(observer, OnAPIRequestInProgress(true)).Times(2);
  // Human and AI entries.
  EXPECT_CALL(observer, OnHistoryUpdate()).Times(2);
  // Fired from OnEngineCompletionComplete.
  EXPECT_CALL(observer, OnAPIRequestInProgress(false)).Times(1);

  conversation_driver_->SubmitSelectedText(
      "<question><excerpt>I have spoken.</excerpt></question>",
      mojom::ActionType::SUMMARIZE_SELECTED_TEXT);

  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&observer);
  // article_text_ and suggestions_ should be cleared when page content is
  // unlinked.
  EXPECT_FALSE(conversation_driver_->GetShouldSendPageContents());
  EXPECT_TRUE(conversation_driver_->GetArticleTextForTesting().empty());
  EXPECT_TRUE(conversation_driver_->IsSuggestionsEmptyForTesting());
  auto& history = conversation_driver_->GetConversationHistory();
  std::vector<mojom::ConversationTurnPtr> expected_history;
  expected_history.push_back(mojom::ConversationTurn::New(
      mojom::CharacterType::HUMAN, mojom::ActionType::SUMMARIZE_SELECTED_TEXT,
      mojom::ConversationTurnVisibility::VISIBLE,
      l10n_util::GetStringUTF8(IDS_AI_CHAT_QUESTION_SUMMARIZE_SELECTED_TEXT),
      "I have spoken.", std::nullopt, base::Time::Now(), std::nullopt));
  expected_history.push_back(mojom::ConversationTurn::New(
      mojom::CharacterType::ASSISTANT, mojom::ActionType::RESPONSE,
      mojom::ConversationTurnVisibility::VISIBLE, "This is the way.",
      std::nullopt, std::nullopt, base::Time::Now(), std::nullopt));
  EXPECT_EQ(history.size(), expected_history.size());
  for (size_t i = 0; i < history.size(); i++) {
    EXPECT_TRUE(CompareConversationTurn(history[i], expected_history[i]));
  }

  // 2. Test with page contents.
  is_page_content_linked = true;
  conversation_driver_->SetShouldSendPageContents(true);

  ON_CALL(*conversation_driver_, GetPageURL)
      .WillByDefault(testing::Return(GURL("https://www.brave.com")));
  EXPECT_CALL(*conversation_driver_, GetPageContent)
      .WillOnce(base::test::RunOnceCallback<0>("The child's name is Grogu.",
                                               false, ""));
  // One from SubmitHumanConversationEntry, one from
  // OnEngineCompletionDataReceived.
  EXPECT_CALL(observer, OnAPIRequestInProgress(true)).Times(2);
  // Human and AI entries.
  EXPECT_CALL(observer, OnHistoryUpdate()).Times(2);
  // Fired from OnEngineCompletionComplete.
  EXPECT_CALL(observer, OnAPIRequestInProgress(false)).Times(1);

  conversation_driver_->SubmitSelectedText(
      "<question><excerpt>I have spoken again.</excerpt></question>",
      mojom::ActionType::SUMMARIZE_SELECTED_TEXT);

  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&observer);

  EXPECT_TRUE(conversation_driver_->GetShouldSendPageContents());
  EXPECT_FALSE(conversation_driver_->GetArticleTextForTesting().empty());
  EXPECT_TRUE(conversation_driver_->IsSuggestionsEmptyForTesting());
  auto& history2 = conversation_driver_->GetConversationHistory();
  std::vector<mojom::ConversationTurnPtr> expected_history2;
  expected_history2.push_back(mojom::ConversationTurn::New(
      mojom::CharacterType::HUMAN, mojom::ActionType::SUMMARIZE_SELECTED_TEXT,
      mojom::ConversationTurnVisibility::VISIBLE,
      l10n_util::GetStringUTF8(IDS_AI_CHAT_QUESTION_SUMMARIZE_SELECTED_TEXT),
      "I have spoken.", std::nullopt, base::Time::Now(), std::nullopt));
  expected_history2.push_back(mojom::ConversationTurn::New(
      mojom::CharacterType::ASSISTANT, mojom::ActionType::RESPONSE,
      mojom::ConversationTurnVisibility::VISIBLE, "This is the way.",
      std::nullopt, std::nullopt, base::Time::Now(), std::nullopt));
  expected_history2.push_back(mojom::ConversationTurn::New(
      mojom::CharacterType::HUMAN, mojom::ActionType::SUMMARIZE_SELECTED_TEXT,
      mojom::ConversationTurnVisibility::VISIBLE,
      l10n_util::GetStringUTF8(IDS_AI_CHAT_QUESTION_SUMMARIZE_SELECTED_TEXT),
      "I have spoken again.", std::nullopt, base::Time::Now(), std::nullopt));
  expected_history2.push_back(mojom::ConversationTurn::New(
      mojom::CharacterType::ASSISTANT, mojom::ActionType::RESPONSE,
      mojom::ConversationTurnVisibility::VISIBLE, "This is the way.",
      std::nullopt, std::nullopt, base::Time::Now(), std::nullopt));
  EXPECT_EQ(history2.size(), expected_history2.size());
  for (size_t i = 0; i < history2.size(); i++) {
    EXPECT_TRUE(CompareConversationTurn(history2[i], expected_history2[i]));
  }
}

TEST_F(ConversationDriverUnitTest, PrintPreviewFallback) {
  // The only purpose of this interceptor is to make sure
  // ConversationDriver::OnEngineCompletionDataReceived is called so we can run
  // next SubmitSummarizationRequest
  url_loader_factory_.SetInterceptor(
      base::BindLambdaForTesting([&](const network::ResourceRequest& request) {
        std::string body = network::GetUploadData(request);
        // Set header for enabling SSE.
        auto head = network::mojom::URLResponseHead::New();
        head->mime_type = "text/event-stream";
        url_loader_factory_.ClearResponses();
        url_loader_factory_.AddResponse(
            request.url, std::move(head),
            R"(data: {"completion": "...", "stop": null})",
            network::URLLoaderCompletionStatus());
      }));
  constexpr char expected_text[] = "This is the way.";
  ON_CALL(*conversation_driver_, GetPageURL)
      .WillByDefault(testing::Return(GURL("https://www.brave.com")));
  EmulateUserOptedIn();

  // Fallback iniatiated on empty string then succeeded.
  EXPECT_CALL(*conversation_driver_, GetPageContent)
      .WillOnce(base::test::RunOnceCallback<0>("", false, ""));
  EXPECT_CALL(*conversation_driver_, PrintPreviewFallback)
      .WillOnce(base::test::RunOnceCallback<0>(expected_text, false, ""));
  conversation_driver_->SubmitSummarizationRequest();
  EXPECT_EQ(conversation_driver_->GetArticleTextForTesting(), expected_text);
  WaitForOnEngineCompletionComplete();

  // Fallback iniatiated on white spaces and line breaks then succeeded.
  EXPECT_CALL(*conversation_driver_, GetPageContent)
      .WillOnce(
          base::test::RunOnceCallback<0>("       \n     \n  ", false, ""));
  EXPECT_CALL(*conversation_driver_, PrintPreviewFallback)
      .WillOnce(base::test::RunOnceCallback<0>(expected_text, false, ""));
  conversation_driver_->SubmitSummarizationRequest();
  EXPECT_EQ(conversation_driver_->GetArticleTextForTesting(), expected_text);
  WaitForOnEngineCompletionComplete();

  // Fallback failed will not retrigger another fallback.
  EXPECT_CALL(*conversation_driver_, GetPageContent)
      .WillOnce(base::test::RunOnceCallback<0>("", false, ""));
  EXPECT_CALL(*conversation_driver_, PrintPreviewFallback)
      .WillOnce(base::test::RunOnceCallback<0>("", false, ""));
  conversation_driver_->SubmitSummarizationRequest();
  EXPECT_EQ(conversation_driver_->GetArticleTextForTesting(), "");
  WaitForOnEngineCompletionComplete();

  // Fallback won't initiate for video content.
  EXPECT_CALL(*conversation_driver_, GetPageContent)
      .WillOnce(base::test::RunOnceCallback<0>("", true, ""));
  EXPECT_CALL(*conversation_driver_, PrintPreviewFallback).Times(0);
  conversation_driver_->SubmitSummarizationRequest();
  EXPECT_EQ(conversation_driver_->GetArticleTextForTesting(), "");
  WaitForOnEngineCompletionComplete();

  // Fallback won't initiate if we already have content
  EXPECT_CALL(*conversation_driver_, GetPageContent)
      .WillOnce(base::test::RunOnceCallback<0>(expected_text, false, ""));
  EXPECT_CALL(*conversation_driver_, PrintPreviewFallback).Times(0);
  conversation_driver_->SubmitSummarizationRequest();
  EXPECT_EQ(conversation_driver_->GetArticleTextForTesting(), expected_text);
  WaitForOnEngineCompletionComplete();

  // Don't fallback on failed print preview extraction.
  EXPECT_CALL(*conversation_driver_, GetPageURL)
      .WillRepeatedly(testing::Return(GURL("https://docs.google.com")));
  EXPECT_CALL(*conversation_driver_, GetPageContent)
      .WillOnce(base::test::RunOnceCallback<0>("", false, ""));
  EXPECT_CALL(*conversation_driver_, PrintPreviewFallback).Times(0);
  conversation_driver_->SubmitSummarizationRequest();
  EXPECT_EQ(conversation_driver_->GetArticleTextForTesting(), "");
}

TEST_F(ConversationDriverUnitTest, UpdateOrCreateLastAssistantEntry_Delta) {
  // Tests that history combines completion events when the engine provides
  // delta text responses.
  conversation_driver_->SetEngineForTesting(
      std::make_unique<MockEngineConsumer>());
  auto* mock_engine = static_cast<MockEngineConsumer*>(
      conversation_driver_->GetEngineForTesting());
  mock_engine->SetSupportsDeltaTextResponses(true);

  EXPECT_EQ(conversation_driver_->GetConversationHistory().size(), 0u);
  {
    auto event = mojom::ConversationEntryEvent::NewCompletionEvent(
        mojom::CompletionEvent::New("This"));
    conversation_driver_->UpdateOrCreateLastAssistantEntry(std::move(event));

    const std::vector<mojom::ConversationTurnPtr>& history =
        conversation_driver_->GetConversationHistory();
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
    conversation_driver_->UpdateOrCreateLastAssistantEntry(std::move(event));

    const std::vector<mojom::ConversationTurnPtr>& history =
        conversation_driver_->GetConversationHistory();
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
    conversation_driver_->UpdateOrCreateLastAssistantEntry(std::move(event));

    const std::vector<mojom::ConversationTurnPtr>& history =
        conversation_driver_->GetConversationHistory();
    EXPECT_EQ(history.size(), 1u);

    EXPECT_EQ(history.back()->text, "This is successful.");
    auto& events = history.back()->events;
    EXPECT_EQ(events->size(), 1u);

    EXPECT_TRUE(events->at(0)->is_completion_event());
    EXPECT_EQ(events->at(0)->get_completion_event()->completion,
              "This is successful.");
  }
}

TEST_F(ConversationDriverUnitTest,
       UpdateOrCreateLastAssistantEntry_DeltaWithSearch) {
  // Tests that history combines completion events when the engine provides
  // delta text responses.
  conversation_driver_->SetEngineForTesting(
      std::make_unique<MockEngineConsumer>());
  auto* mock_engine = static_cast<MockEngineConsumer*>(
      conversation_driver_->GetEngineForTesting());
  mock_engine->SetSupportsDeltaTextResponses(true);
  // In addition, add a non-completion event (e.g. search) and verify it's
  // not removed.
  {
    auto event = mojom::ConversationEntryEvent::NewSearchStatusEvent(
        mojom::SearchStatusEvent::New());
    conversation_driver_->UpdateOrCreateLastAssistantEntry(std::move(event));
    const std::vector<mojom::ConversationTurnPtr>& history =
        conversation_driver_->GetConversationHistory();
    EXPECT_EQ(history.size(), 1u);
    auto& events = history.back()->events;
    EXPECT_EQ(events->size(), 1u);
  }
  {
    // Leading space on the first message should be removed
    auto event = mojom::ConversationEntryEvent::NewCompletionEvent(
        mojom::CompletionEvent::New(" This is"));
    conversation_driver_->UpdateOrCreateLastAssistantEntry(std::move(event));

    const std::vector<mojom::ConversationTurnPtr>& history =
        conversation_driver_->GetConversationHistory();
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
    conversation_driver_->UpdateOrCreateLastAssistantEntry(std::move(event));

    const std::vector<mojom::ConversationTurnPtr>& history =
        conversation_driver_->GetConversationHistory();
    EXPECT_EQ(history.size(), 1u);

    EXPECT_EQ(history.back()->text, "This is successful.");
    auto& events = history.back()->events;
    EXPECT_EQ(events->size(), 2u);

    EXPECT_TRUE(events->at(1)->is_completion_event());
    EXPECT_EQ(events->at(1)->get_completion_event()->completion,
              "This is successful.");
  }
}

TEST_F(ConversationDriverUnitTest, UpdateOrCreateLastAssistantEntry_NotDelta) {
  // Tests that history combines completion events when the engine provides
  // delta text responses.
  conversation_driver_->SetEngineForTesting(
      std::make_unique<MockEngineConsumer>());
  auto* mock_engine = static_cast<MockEngineConsumer*>(
      conversation_driver_->GetEngineForTesting());
  mock_engine->SetSupportsDeltaTextResponses(false);

  EXPECT_EQ(conversation_driver_->GetConversationHistory().size(), 0u);
  {
    auto event = mojom::ConversationEntryEvent::NewCompletionEvent(
        mojom::CompletionEvent::New("This"));
    conversation_driver_->UpdateOrCreateLastAssistantEntry(std::move(event));

    const std::vector<mojom::ConversationTurnPtr>& history =
        conversation_driver_->GetConversationHistory();
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
    conversation_driver_->UpdateOrCreateLastAssistantEntry(std::move(event));

    const std::vector<mojom::ConversationTurnPtr>& history =
        conversation_driver_->GetConversationHistory();
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
    conversation_driver_->UpdateOrCreateLastAssistantEntry(std::move(event));

    const std::vector<mojom::ConversationTurnPtr>& history =
        conversation_driver_->GetConversationHistory();
    EXPECT_EQ(history.size(), 1u);

    EXPECT_EQ(history.back()->text, "This is successful.");
    auto& events = history.back()->events;
    EXPECT_EQ(events->size(), 1u);

    EXPECT_TRUE(events->at(0)->is_completion_event());
    EXPECT_EQ(events->at(0)->get_completion_event()->completion,
              "This is successful.");
  }
}

TEST_F(ConversationDriverUnitTest,
       UpdateOrCreateLastAssistantEntry_NotDeltaWithSearch) {
  // Tests that history combines completion events when the engine provides
  // delta text responses.
  conversation_driver_->SetEngineForTesting(
      std::make_unique<MockEngineConsumer>());
  auto* mock_engine = static_cast<MockEngineConsumer*>(
      conversation_driver_->GetEngineForTesting());
  mock_engine->SetSupportsDeltaTextResponses(false);
  // In addition, add a non-completion event (e.g. search) and verify it's
  // not removed.
  {
    auto event = mojom::ConversationEntryEvent::NewSearchStatusEvent(
        mojom::SearchStatusEvent::New());
    conversation_driver_->UpdateOrCreateLastAssistantEntry(std::move(event));
    const std::vector<mojom::ConversationTurnPtr>& history =
        conversation_driver_->GetConversationHistory();
    EXPECT_EQ(history.size(), 1u);
    auto& events = history.back()->events;
    EXPECT_EQ(events->size(), 1u);
  }
  {
    // Leading space should be removed for every partial message
    auto event = mojom::ConversationEntryEvent::NewCompletionEvent(
        mojom::CompletionEvent::New(" This is "));
    conversation_driver_->UpdateOrCreateLastAssistantEntry(std::move(event));

    const std::vector<mojom::ConversationTurnPtr>& history =
        conversation_driver_->GetConversationHistory();
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
    conversation_driver_->UpdateOrCreateLastAssistantEntry(std::move(event));

    const std::vector<mojom::ConversationTurnPtr>& history =
        conversation_driver_->GetConversationHistory();
    EXPECT_EQ(history.size(), 1u);

    EXPECT_EQ(history.back()->text, "This is successful.");
    auto& events = history.back()->events;
    EXPECT_EQ(events->size(), 2u);

    EXPECT_TRUE(events->at(1)->is_completion_event());
    EXPECT_EQ(events->at(1)->get_completion_event()->completion,
              "This is successful.");
  }
}

TEST_F(ConversationDriverUnitTest, ModifyConversation) {
  conversation_driver_->SetShouldSendPageContents(false);
  EmulateUserOptedIn();

  url_loader_factory_.SetInterceptor(
      base::BindLambdaForTesting([&](const network::ResourceRequest& request) {
        // Set header for enabling SSE.
        auto head = network::mojom::URLResponseHead::New();
        head->mime_type = "text/event-stream";
        url_loader_factory_.ClearResponses();
        url_loader_factory_.AddResponse(
            request.url, std::move(head),
            R"(data: {"completion": "new answer", "stop": null})",
            network::URLLoaderCompletionStatus());
      }));

  // Setup history for testing.
  auto created_time1 = base::Time::Now();
  std::vector<mojom::ConversationTurnPtr> history;
  history.push_back(mojom::ConversationTurn::New(
      mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
      mojom::ConversationTurnVisibility::VISIBLE, "prompt1", std::nullopt,
      std::nullopt, created_time1, std::nullopt));
  history.push_back(mojom::ConversationTurn::New(
      mojom::CharacterType::ASSISTANT, mojom::ActionType::RESPONSE,
      mojom::ConversationTurnVisibility::VISIBLE, "answer1", std::nullopt,
      std::nullopt, base::Time::Now(), std::nullopt));
  conversation_driver_->SetChatHistoryForTesting(std::move(history));

  // Modify an entry for the first time.
  conversation_driver_->ModifyConversation(0, "prompt2");
  const auto& conversation_history =
      conversation_driver_->GetConversationHistory();
  ASSERT_EQ(conversation_history.size(), 1u);
  EXPECT_EQ(conversation_history[0]->text, "prompt1");
  EXPECT_EQ(conversation_history[0]->created_time, created_time1);

  ASSERT_TRUE(conversation_history[0]->edits);
  ASSERT_EQ(conversation_history[0]->edits->size(), 1u);
  EXPECT_EQ(conversation_history[0]->edits->at(0)->text, "prompt2");
  EXPECT_NE(conversation_history[0]->edits->at(0)->created_time, created_time1);
  EXPECT_FALSE(conversation_history[0]->edits->at(0)->edits);

  WaitForOnEngineCompletionComplete();
  ASSERT_EQ(conversation_history.size(), 2u);
  EXPECT_EQ(conversation_history[1]->text, "new answer");

  auto created_time2 = conversation_history[0]->edits->at(0)->created_time;

  // Modify the same entry again.
  conversation_driver_->ModifyConversation(0, "prompt3");
  ASSERT_EQ(conversation_history.size(), 1u);
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

  WaitForOnEngineCompletionComplete();
  ASSERT_EQ(conversation_history.size(), 2u);
  EXPECT_EQ(conversation_history[1]->text, "new answer");

  // Modify server response should have text and completion event updated in
  // the entry of edits.
  conversation_driver_->ModifyConversation(1, " answer2 ");
  ASSERT_EQ(conversation_history.size(), 2u);
  EXPECT_EQ(conversation_history[1]->text, "new answer");

  ASSERT_TRUE(conversation_history[1]->edits);
  ASSERT_EQ(conversation_history[1]->edits->size(), 1u);
  EXPECT_EQ(conversation_history[1]->edits->at(0)->text, "answer2");
  EXPECT_NE(conversation_history[1]->edits->at(0)->created_time,
            conversation_history[1]->created_time);

  ASSERT_TRUE(conversation_history[1]->events);
  ASSERT_EQ(conversation_history[1]->events->size(), 1u);
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

class PageContentRefineTest : public ConversationDriverUnitTest,
                              public testing::WithParamInterface<bool> {
 public:
  PageContentRefineTest() {
    scoped_feature_list_.InitWithFeatureState(features::kPageContentRefine,
                                              GetParam());
  }
  void SetUp() override {
    ConversationDriverUnitTest::SetUp();
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
  conversation_driver_->SetEngineForTesting(
      std::make_unique<MockEngineConsumer>());
  auto* mock_engine = static_cast<MockEngineConsumer*>(
      conversation_driver_->GetEngineForTesting());

  conversation_driver_->SetTextEmbedderForTesting(
      std::unique_ptr<MockTextEmbedder, base::OnTaskRunnerDeleter>(
          new MockTextEmbedder(embedder_task_runner_),
          base::OnTaskRunnerDeleter(embedder_task_runner_)));
  auto* mock_text_embedder = static_cast<MockTextEmbedder*>(
      conversation_driver_->GetTextEmbedderForTesting());

  if (IsPageContentRefineEnabled()) {
    ON_CALL(*mock_text_embedder, IsInitialized)
        .WillByDefault(testing::Return(true));
  } else {
    EXPECT_CALL(*mock_text_embedder, IsInitialized).Times(0);
  }

  uint32_t max_page_content_length = conversation_driver_->GetCurrentModel()
                                         .options->get_leo_model_options()
                                         ->max_page_content_length;
  struct {
    std::string prompt;
    std::string page_content;
    bool should_refine_page_content;
  } test_cases[] = {
      {"prompt", std::string(max_page_content_length - 1, 'A'), false},
      {"prompt", std::string(max_page_content_length, 'A'), false},
      {"prompt", std::string(max_page_content_length + 1, 'A'), true},
      {l10n_util::GetStringUTF8(IDS_AI_CHAT_QUESTION_SUMMARIZE_PAGE),
       std::string(max_page_content_length + 1, 'A'), false},
  };

  for (const auto& test_case : test_cases) {
    SCOPED_TRACE(testing::Message()
                 << "Prompt: " << test_case.prompt
                 << ", Page content length: " << test_case.page_content.length()
                 << ", Should refine page content: "
                 << test_case.should_refine_page_content);
    if (test_case.should_refine_page_content && IsPageContentRefineEnabled()) {
      EXPECT_CALL(*mock_text_embedder,
                  GetTopSimilarityWithPromptTilContextLimit(_, _, _, _))
          .Times(1);
      EXPECT_CALL(*mock_engine, GenerateAssistantResponse(_, _, _, _, _, _))
          .Times(0);
    } else {
      EXPECT_CALL(*mock_text_embedder,
                  GetTopSimilarityWithPromptTilContextLimit(_, _, _, _))
          .Times(0);
      EXPECT_CALL(*mock_engine, GenerateAssistantResponse(_, _, _, _, _, _))
          .Times(1);
    }
    conversation_driver_->PerformAssistantGeneration(
        test_case.prompt, 0, test_case.page_content, false, "");
  }
}

TEST_P(PageContentRefineTest, TextEmbedderInitialized) {
  if (!IsPageContentRefineEnabled()) {
    return;
  }
  conversation_driver_->SetEngineForTesting(
      std::make_unique<MockEngineConsumer>());
  auto* mock_engine = static_cast<MockEngineConsumer*>(
      conversation_driver_->GetEngineForTesting());

  conversation_driver_->SetTextEmbedderForTesting(
      std::unique_ptr<MockTextEmbedder, base::OnTaskRunnerDeleter>(
          new MockTextEmbedder(embedder_task_runner_),
          base::OnTaskRunnerDeleter(embedder_task_runner_)));
  auto* mock_text_embedder = static_cast<MockTextEmbedder*>(
      conversation_driver_->GetTextEmbedderForTesting());
  struct {
    bool is_initialized;
    bool initialize_result;
  } test_cases[] = {
      {true, false},  // Already initialized so initialize_result is ignored.
      {false, false},
      {false, true},
  };

  uint32_t max_page_content_length = conversation_driver_->GetCurrentModel()
                                         .options->get_leo_model_options()
                                         ->max_page_content_length;
  for (const auto& test_case : test_cases) {
    SCOPED_TRACE(testing::Message()
                 << "IsInitialized: " << test_case.is_initialized
                 << ", Initialize result: " << test_case.initialize_result);

    ON_CALL(*mock_text_embedder, IsInitialized)
        .WillByDefault(testing::Return(test_case.is_initialized));
    if (test_case.is_initialized) {
      EXPECT_CALL(*mock_text_embedder, Initialize).Times(0);
      EXPECT_CALL(*mock_engine, GenerateAssistantResponse(_, _, _, _, _, _))
          .Times(0);
      EXPECT_CALL(*mock_text_embedder,
                  GetTopSimilarityWithPromptTilContextLimit(_, _, _, _))
          .Times(1);
    } else {
      EXPECT_CALL(*mock_text_embedder, Initialize)
          .WillOnce(
              base::test::RunOnceCallback<0>(test_case.initialize_result));
      if (!test_case.initialize_result) {
        EXPECT_CALL(*mock_engine, GenerateAssistantResponse(_, _, _, _, _, _))
            .Times(1);
        EXPECT_CALL(*mock_text_embedder,
                    GetTopSimilarityWithPromptTilContextLimit(_, _, _, _))
            .Times(0);
      } else {
        EXPECT_CALL(*mock_engine, GenerateAssistantResponse(_, _, _, _, _, _))
            .Times(0);
        EXPECT_CALL(*mock_text_embedder,
                    GetTopSimilarityWithPromptTilContextLimit(_, _, _, _))
            .Times(1);
      }
    }
    conversation_driver_->PerformAssistantGeneration(
        "prompt", 0, std::string(max_page_content_length + 1, 'A'), false, "");
  }
}
TEST_P(PageContentRefineTest, LeoLocalModelsUpdater) {
  if (IsPageContentRefineEnabled()) {
    EXPECT_CALL(*leo_local_models_updater_, Register());
  } else {
    EXPECT_CALL(*leo_local_models_updater_, Register()).Times(0);
  }
  auto credential_manager = std::make_unique<MockAIChatCredentialManager>(
      base::NullCallback(), &local_state_);
  auto conversation_driver = std::make_unique<MockConversationDriver>(
      &prefs_, &local_state_, service_.get(), nullptr,
      leo_local_models_updater_.get(), std::move(credential_manager),
      shared_url_loader_factory_, "");

  EXPECT_CALL(*leo_local_models_updater_, GetUniversalQAModel())
      .WillOnce(testing::ReturnRefOfCopy(base::FilePath()));
  conversation_driver->OnLeoLocalModelsReady();
  EXPECT_TRUE(conversation_driver->universal_qa_model_path_.empty());

  EXPECT_CALL(*leo_local_models_updater_, GetUniversalQAModel())
      .WillOnce(testing::ReturnRefOfCopy(
          base::FilePath::FromASCII("/path/to/model")));
  conversation_driver->OnLeoLocalModelsReady();
  EXPECT_EQ(conversation_driver->universal_qa_model_path_,
            base::FilePath::FromASCII("/path/to/model"));

  uint32_t max_page_content_length = conversation_driver->GetCurrentModel()
                                         .options->get_leo_model_options()
                                         ->max_page_content_length;

  // should_refine_page_content is false.
  conversation_driver->universal_qa_model_path_ = base::FilePath();
  EXPECT_CALL(*leo_local_models_updater_, GetUniversalQAModel()).Times(0);
  conversation_driver->PerformAssistantGeneration(
      "prompt", 0, std::string(max_page_content_length, 'A'), false, "");
  EXPECT_TRUE(conversation_driver->universal_qa_model_path_.empty());

  // Already has TextEmbedder.
  conversation_driver->SetTextEmbedderForTesting(
      std::unique_ptr<MockTextEmbedder, base::OnTaskRunnerDeleter>(
          new MockTextEmbedder(embedder_task_runner_),
          base::OnTaskRunnerDeleter(embedder_task_runner_)));
  conversation_driver->universal_qa_model_path_ = base::FilePath();
  EXPECT_CALL(*leo_local_models_updater_, GetUniversalQAModel()).Times(0);
  conversation_driver->PerformAssistantGeneration(
      "prompt", 0, std::string(max_page_content_length, 'A'), false, "");
  EXPECT_TRUE(conversation_driver->universal_qa_model_path_.empty());
}

}  // namespace ai_chat
