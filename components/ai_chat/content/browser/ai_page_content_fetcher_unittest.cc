// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/content/browser/ai_page_content_fetcher.h"

#include <string>
#include <utility>

#include "base/functional/callback_helpers.h"
#include "base/test/test_future.h"
#include "brave/components/ai_chat/content/browser/annotated_page_content_test_util.h"
#include "components/optimization_guide/content/browser/page_content_proto_provider.h"
#include "content/public/test/test_renderer_host.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace ai_chat {

namespace {

class FakeAIPageContentFetcher : public AIPageContentFetcher {
 public:
  explicit FakeAIPageContentFetcher(content::WebContents* web_contents)
      : AIPageContentFetcher(web_contents) {}

  void RespondWith(optimization_guide::AIPageContentResultOrError result) {
    ASSERT_TRUE(pending_callback_);
    std::move(pending_callback_).Run(std::move(result));
  }

  bool was_called() const { return was_called_; }

 protected:
  void CallGetAIPageContent(
      optimization_guide::OnAIPageContentDone callback) override {
    was_called_ = true;
    pending_callback_ = std::move(callback);
  }

 private:
  bool was_called_ = false;
  optimization_guide::OnAIPageContentDone pending_callback_;
};

}  // namespace

class AIPageContentFetcherTest : public content::RenderViewHostTestHarness {
 protected:
  void SetUp() override {
    content::RenderViewHostTestHarness::SetUp();
    fetcher_ = std::make_unique<FakeAIPageContentFetcher>(web_contents());
  }

  void TearDown() override {
    // Reset before parent TearDown destroys web_contents() to avoid a dangling
    // raw_ptr on AIPageContentFetcher::web_contents_.
    fetcher_.reset();
    content::RenderViewHostTestHarness::TearDown();
  }

  std::unique_ptr<FakeAIPageContentFetcher> fetcher_;
};

// For URLs with custom extraction logic, FetchPageContent should delegate to
// PageContentFetcher rather than calling GetAIPageContent.
TEST_F(AIPageContentFetcherTest, CustomURL_SkipsGetAIPageContent) {
  NavigateAndCommit(GURL("https://www.youtube.com/watch?v=abc123"));
  fetcher_->FetchPageContent("", base::DoNothing());
  EXPECT_FALSE(fetcher_->was_called());
}

// When GetAIPageContent returns an error, the callback should be called with
// empty content.
TEST_F(AIPageContentFetcherTest, Error_RunsCallbackWithEmpty) {
  NavigateAndCommit(GURL("https://example.com/page"));
  base::test::TestFuture<std::string, bool, std::string> future;
  fetcher_->FetchPageContent("", future.GetCallback());
  ASSERT_TRUE(fetcher_->was_called());
  fetcher_->RespondWith(base::unexpected(std::string("test error")));
  const auto& [content, is_video, invalidation_token] = future.Get();
  EXPECT_EQ(content, "");
  EXPECT_FALSE(is_video);
  EXPECT_EQ(invalidation_token, "");
}

// When GetAIPageContent returns a result that produces no content blocks,
// the callback should be called with empty content.
TEST_F(AIPageContentFetcherTest, EmptyContentBlocks_RunsCallbackWithEmpty) {
  NavigateAndCommit(GURL("https://example.com/page"));
  base::test::TestFuture<std::string, bool, std::string> future;
  fetcher_->FetchPageContent("", future.GetCallback());
  ASSERT_TRUE(fetcher_->was_called());
  optimization_guide::AIPageContentResult result;
  result.proto = annotated_page_content_test_util::CreateEmptyPage();
  fetcher_->RespondWith(base::ok(std::move(result)));
  const auto& [content, is_video, invalidation_token] = future.Get();
  EXPECT_EQ(content, "");
  EXPECT_FALSE(is_video);
  EXPECT_EQ(invalidation_token, "");
}

// When GetAIPageContent returns a valid result, the callback should be called
// with the extracted text content.
TEST_F(AIPageContentFetcherTest, Success_RunsCallbackWithText) {
  NavigateAndCommit(GURL("https://example.com/page"));
  base::test::TestFuture<std::string, bool, std::string> future;
  fetcher_->FetchPageContent("", future.GetCallback());
  ASSERT_TRUE(fetcher_->was_called());
  optimization_guide::AIPageContentResult result;
  result.proto = annotated_page_content_test_util::CreateMinimalPage(
      "My Page", "https://example.com/page");
  fetcher_->RespondWith(base::ok(std::move(result)));
  const auto& [content, is_video, invalidation_token] = future.Get();
  EXPECT_FALSE(content.empty());
  EXPECT_FALSE(is_video);
  EXPECT_EQ(invalidation_token, "");
}

}  // namespace ai_chat
