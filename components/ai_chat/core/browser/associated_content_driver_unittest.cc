// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/associated_content_driver.h"

#include <optional>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

#include "base/functional/bind.h"
#include "base/location.h"
#include "base/run_loop.h"
#include "base/task/sequenced_task_runner.h"
#include "base/test/bind.h"
#include "base/test/gmock_callback_support.h"
#include "base/test/mock_callback.h"
#include "base/test/task_environment.h"
#include "base/test/values_test_util.h"
#include "brave/components/ai_chat/core/browser/conversation_handler.h"
#include "brave/components/ai_chat/core/browser/types.h"
#include "brave/components/ai_chat/core/common/mojom/page_content_extractor.mojom.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

using testing::_;
using testing::ElementsAre;
using testing::Eq;
using testing::NiceMock;
using testing::Optional;

class MockAssociatedContentDriver : public AssociatedContentDriver {
 public:
  MockAssociatedContentDriver(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
      : AssociatedContentDriver(nullptr /*ai_chat_service*/,
                                url_loader_factory) {}
  ~MockAssociatedContentDriver() override = default;

  MOCK_METHOD(GURL, GetPageURL, (), (const, override));
  MOCK_METHOD(std::u16string, GetPageTitle, (), (const, override));
  MOCK_METHOD(void,
              GetPageContent,
              (ConversationHandler::GetPageContentCallback, std::string_view),
              (override));
  MOCK_METHOD(void,
              GetSearchSummarizerKey,
              (mojom::PageContentExtractor::GetSearchSummarizerKeyCallback),
              (override));
};

class AssociatedContentDriverUnitTest : public ::testing::Test {
 public:
  void SetUp() override {
    shared_url_loader_factory_ =
        base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
            &url_loader_factory_);
    associated_content_driver_ =
        std::make_unique<NiceMock<MockAssociatedContentDriver>>(
            shared_url_loader_factory_);
  }

  void SetSearchQuerySummaryInterceptor(bool empty = false) {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&, empty](const network::ResourceRequest& request) {
          std::string rsp =
              empty ? R"({"conversation": []})" : R"({"conversation": [
                    {"query": "query", "answer": [{"text": "summary"}]}]})";
          url_loader_factory_.ClearResponses();
          url_loader_factory_.AddResponse(request.url.spec(), rsp);
        }));
  }

 protected:
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  std::unique_ptr<MockAssociatedContentDriver> associated_content_driver_;
  base::test::TaskEnvironment task_environment_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
};

TEST_F(AssociatedContentDriverUnitTest, GetContent) {
  base::MockCallback<ConversationHandler::GetPageContentCallback> callback1;
  base::MockCallback<ConversationHandler::GetPageContentCallback> callback2;
  base::MockCallback<ConversationHandler::GetPageContentCallback> callback3;

  EXPECT_CALL(callback1, Run("content", false, "token")).Times(1);
  EXPECT_CALL(callback2, Run("content", false, "token")).Times(1);
  EXPECT_CALL(callback3, Run("content", false, "token")).Times(1);

  // Should only ask content once
  base::RunLoop run_loop;
  EXPECT_CALL(*associated_content_driver_, GetPageContent(_, _))
      .WillOnce([&](ConversationHandler::GetPageContentCallback callback,
                    std::string_view invalidation_token) {
        // Simulate async response so that multiple calls can queue
        base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
            FROM_HERE,
            base::BindOnce(
                [](ConversationHandler::GetPageContentCallback callback) {
                  std::move(callback).Run("content", false, "token");
                },
                std::move(callback)));
      });

  // Test
  associated_content_driver_->GetContent(callback1.Get());
  associated_content_driver_->GetContent(callback2.Get());
  associated_content_driver_->GetContent(callback3.Get());

  // Block until content is "fetched"
  run_loop.RunUntilIdle();

  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(associated_content_driver_.get());
}

TEST_F(AssociatedContentDriverUnitTest, GetStagedEntriesFromContent) {
  SetSearchQuerySummaryInterceptor();
  // Give the function a valid URL
  ON_CALL(*associated_content_driver_, GetPageURL)
      .WillByDefault(
          testing::Return(GURL("https://search.brave.com/search?q=test")));
  // Give the function a valid key
  EXPECT_CALL(*associated_content_driver_, GetSearchSummarizerKey)
      .WillOnce(base::test::RunOnceCallback<0>("key"));

  // Expect a result
  base::MockCallback<ConversationHandler::GetStagedEntriesCallback> callback;
  EXPECT_CALL(
      callback,
      Run(Optional(ElementsAre(Eq(SearchQuerySummary("query", "summary"))))))
      .Times(1);

  // Test
  associated_content_driver_->GetStagedEntriesFromContent(callback.Get());

  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(associated_content_driver_.get());
}

TEST_F(AssociatedContentDriverUnitTest,
       GetStagedEntriesFromContent_NotBraveSearchSERP) {
  SetSearchQuerySummaryInterceptor(true);
  // Fetch should not be called if page URL is not Brave Search SERP, staged
  // query and summary will be cleared.
  ON_CALL(*associated_content_driver_, GetPageURL)
      .WillByDefault(testing::Return(GURL("https://search.brave.com")));
  EXPECT_CALL(*associated_content_driver_, GetSearchSummarizerKey).Times(0);

  base::MockCallback<ConversationHandler::GetStagedEntriesCallback> callback;
  EXPECT_CALL(callback, Run(Eq(std::nullopt))).Times(1);

  associated_content_driver_->GetStagedEntriesFromContent(callback.Get());

  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(associated_content_driver_.get());
}

TEST_F(AssociatedContentDriverUnitTest, GetStagedEntriesFromContent_NoKey) {
  ON_CALL(*associated_content_driver_, GetPageURL)
      .WillByDefault(
          testing::Return(GURL("https://search.brave.com/search?q=test")));
  EXPECT_CALL(*associated_content_driver_, GetSearchSummarizerKey)
      .WillOnce(base::test::RunOnceCallback<0>(std::nullopt));

  base::MockCallback<ConversationHandler::GetStagedEntriesCallback> callback;
  EXPECT_CALL(callback, Run(Eq(std::nullopt))).Times(1);

  associated_content_driver_->GetStagedEntriesFromContent(callback.Get());

  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(associated_content_driver_.get());
}

TEST_F(AssociatedContentDriverUnitTest, GetStagedEntriesFromContent_NoResult) {
  SetSearchQuerySummaryInterceptor(true);
  ON_CALL(*associated_content_driver_, GetPageURL)
      .WillByDefault(
          testing::Return(GURL("https://search.brave.com/search?q=test")));
  EXPECT_CALL(*associated_content_driver_, GetSearchSummarizerKey)
      .WillOnce(base::test::RunOnceCallback<0>("key"));

  base::MockCallback<ConversationHandler::GetStagedEntriesCallback> callback;
  EXPECT_CALL(callback, Run(Eq(std::nullopt))).Times(1);

  associated_content_driver_->GetStagedEntriesFromContent(callback.Get());

  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(associated_content_driver_.get());
}

TEST_F(AssociatedContentDriverUnitTest, ParseSearchQuerySummaryResponse) {
  struct {
    std::string response;
    std::optional<std::vector<SearchQuerySummary>> expected_query_summary;
  } test_cases[] = {
      {"{}", std::nullopt},
      {R"({"conversation": []})", std::nullopt},  // empty conversation
      {R"({"conversation": [{"query": "q","answer": []}]})",
       std::vector<SearchQuerySummary>()},  // empty answer
      {R"({"conversation": [{"query": "q", "answer": [{"text": "t"}]}]})",
       std::vector<SearchQuerySummary>({SearchQuerySummary("q", "t")})},
      {R"({"conversation": [
          {"query": "q1", "answer": [{"text": "t1"}, {"text": "t2"}]},
          {"query": "q2", "answer": []},
          {"query": "q3", "answer": [{"text": "t3"}]}
        ]})",
       std::vector<SearchQuerySummary>(
           {SearchQuerySummary("q1", "t1"), SearchQuerySummary("q3", "t3")})},
  };

  for (const auto& test_case : test_cases) {
    SCOPED_TRACE(testing::Message() << test_case.response);
    auto query_summary =
        associated_content_driver_->ParseSearchQuerySummaryResponse(
            base::test::ParseJson(test_case.response));
    EXPECT_EQ(query_summary, test_case.expected_query_summary);
  }
}

}  // namespace ai_chat
