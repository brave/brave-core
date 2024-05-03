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
#include "base/test/bind.h"
#include "base/test/gmock_callback_support.h"
#include "base/test/mock_callback.h"
#include "base/test/task_environment.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "brave/components/skus/common/skus_sdk.mojom.h"
#include "components/grit/brave_components_strings.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
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

namespace ai_chat {

namespace {

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
      AIChatMetrics* ai_chat_metrics,
      base::RepeatingCallback<mojo::PendingRemote<skus::mojom::SkusService>()>
          skus_service_getter,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      std::string_view channel_string)
      : ConversationDriver(profile_prefs,
                           local_state,
                           ai_chat_metrics,
                           std::move(skus_service_getter),
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

}  // namespace

class ConversationDriverUnitTest : public testing::Test {
 public:
  ConversationDriverUnitTest() = default;
  ~ConversationDriverUnitTest() override = default;

  void SetUp() override {
    prefs::RegisterProfilePrefs(prefs_.registry());
    prefs::RegisterLocalStatePrefs(local_state_.registry());
    shared_url_loader_factory_ =
        base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
            &url_loader_factory_);
    auto skus_service_getter = base::BindRepeating(
        []() { return mojo::PendingRemote<skus::mojom::SkusService>(); });
    conversation_driver_ = std::make_unique<MockConversationDriver>(
        &prefs_, &local_state_, nullptr, skus_service_getter,
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
  sync_preferences::TestingPrefServiceSyncable prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
  std::unique_ptr<MockConversationDriver> conversation_driver_;
};

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
  EXPECT_EQ(
      history,
      std::vector<mojom::ConversationTurn>(
          {mojom::ConversationTurn(
               mojom::CharacterType::HUMAN,
               mojom::ActionType::SUMMARIZE_SELECTED_TEXT,
               mojom::ConversationTurnVisibility::VISIBLE,
               l10n_util::GetStringUTF8(
                   IDS_AI_CHAT_QUESTION_SUMMARIZE_SELECTED_TEXT),
               "I have spoken."),
           mojom::ConversationTurn(mojom::CharacterType::ASSISTANT,
                                   mojom::ActionType::RESPONSE,
                                   mojom::ConversationTurnVisibility::VISIBLE,
                                   "This is the way.", std::nullopt)}));

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
  EXPECT_EQ(
      history2,
      std::vector<mojom::ConversationTurn>(
          {mojom::ConversationTurn(
               mojom::CharacterType::HUMAN,
               mojom::ActionType::SUMMARIZE_SELECTED_TEXT,
               mojom::ConversationTurnVisibility::VISIBLE,
               l10n_util::GetStringUTF8(
                   IDS_AI_CHAT_QUESTION_SUMMARIZE_SELECTED_TEXT),
               "I have spoken."),
           mojom::ConversationTurn(mojom::CharacterType::ASSISTANT,
                                   mojom::ActionType::RESPONSE,
                                   mojom::ConversationTurnVisibility::VISIBLE,
                                   "This is the way.", std::nullopt),
           mojom::ConversationTurn(
               mojom::CharacterType::HUMAN,
               mojom::ActionType::SUMMARIZE_SELECTED_TEXT,
               mojom::ConversationTurnVisibility::VISIBLE,
               l10n_util::GetStringUTF8(
                   IDS_AI_CHAT_QUESTION_SUMMARIZE_SELECTED_TEXT),
               "I have spoken again."),
           mojom::ConversationTurn(mojom::CharacterType::ASSISTANT,
                                   mojom::ActionType::RESPONSE,
                                   mojom::ConversationTurnVisibility::VISIBLE,
                                   "This is the way.", std::nullopt)}));
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

}  // namespace ai_chat
