/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/webui/tab_search/tab_search_page_handler.h"

#include "base/test/bind.h"
#include "base/test/gmock_callback_support.h"
#include "base/test/mock_callback.h"
#include "brave/components/ai_chat/core/browser/engine/mock_engine_consumer.h"
#include "brave/components/ai_chat/core/browser/types.h"
#include "testing/gmock/include/gmock/gmock.h"

#include "src/chrome/browser/ui/webui/tab_search/tab_search_page_handler_unittest.cc"

namespace {

constexpr char kFooDotComUrl1[] = "https://foo.com/1";
constexpr char kFooDotComUrl2[] = "https://foo.com/2";
constexpr char kBarDotComUrl1[] = "https://bar.com/1";
constexpr char kBarDotComUrl2[] = "https://bar.com/2";
constexpr char kCatDotComUrl1[] = "https://cat.com/1";
constexpr char kCatDotComUrl2[] = "https://cat.com/2";
constexpr char kDogDotComUrl1[] = "https://dog.com/1";

constexpr char kFooDotComOrigin[] = "https://foo.com";
constexpr char kBarDotComOrigin[] = "https://bar.com";
constexpr char kCatDotComOrigin[] = "https://cat.com";

constexpr char kFooDotComTitle1[] = "foo.com 1";
constexpr char kFooDotComTitle2[] = "foo.com 2";
constexpr char kBarDotComTitle1[] = "bar.com 1";
constexpr char kBarDotComTitle2[] = "bar.com 2";
constexpr char kCatDotComTitle1[] = "cat.com 1";
constexpr char kCatDotComTitle2[] = "cat.com 2";
constexpr char kDogDotComTitle1[] = "dog.com 1";

}  // namespace

TEST_F(TabSearchPageHandlerTest, GetSuggestedTopics) {
  handler()->SetAIChatEngineForTesting(
      std::make_unique<testing::NiceMock<ai_chat::MockEngineConsumer>>());

  // Create multiple tabs in different windows and verify GetSuggestedTopics is
  // called with expected tabs info.
  // Browser with the same profile but not normal type.
  AddTabWithTitle(browser5(), GURL(kFooDotComUrl1), kFooDotComTitle1);
  // Browser with a different profile of the default browser.
  AddTabWithTitle(browser4(), GURL(kFooDotComUrl2), kFooDotComTitle2);
  // Browser with incognito profile.
  AddTabWithTitle(browser3(), GURL(kBarDotComUrl1), kBarDotComTitle1);
  // Browser with the same profile of the default browser.
  AddTabWithTitle(browser2(), GURL(kBarDotComUrl2), kBarDotComTitle2);
  // The default browser.
  AddTabWithTitle(browser1(), GURL(kCatDotComUrl2), kCatDotComTitle2);
  AddTabWithTitle(browser1(), GURL(kCatDotComUrl1), kCatDotComTitle1);

  // Tab with kCatDotComUrl1 and kCatDotComTitle1.
  const int tab_id1 =
      browser1()->tab_strip_model()->GetTabAtIndex(0)->GetHandle().raw_value();
  // Tab with kCatDotComUrl2 and kCatDotComTitle2.
  const int tab_id2 =
      browser1()->tab_strip_model()->GetTabAtIndex(1)->GetHandle().raw_value();
  // Tab with kBarDotComUrl2 and kBarDotComTitle2.
  const int tab_id3 =
      browser2()->tab_strip_model()->GetTabAtIndex(0)->GetHandle().raw_value();

  std::vector<ai_chat::Tab> expected_tabs = {
      {base::NumberToString(tab_id1), kCatDotComTitle1, kCatDotComOrigin},
      {base::NumberToString(tab_id2), kCatDotComTitle2, kCatDotComOrigin},
      {base::NumberToString(tab_id3), kBarDotComTitle2, kBarDotComOrigin},
  };

  std::vector<std::string> expected_topics = {"topic1", "topic2", "topic3",
                                              "topic4", "topic5"};
  auto* mock_engine = static_cast<ai_chat::MockEngineConsumer*>(
      handler()->GetAIChatEngineForTesting());
  EXPECT_CALL(*mock_engine, GetSuggestedTopics(expected_tabs, _))
      .WillOnce(base::test::RunOnceCallback<1>(expected_topics));

  // Uninteresting calls from upstream.
  EXPECT_CALL(page_, TabsChanged(_)).Times(testing::AnyNumber());
  EXPECT_CALL(page_, TabUpdated(_)).Times(testing::AnyNumber());
  EXPECT_CALL(page_, TabsRemoved(_)).Times(testing::AnyNumber());

  handler()->GetSuggestedTopics(
      base::BindLambdaForTesting([&](const std::vector<std::string>& topics) {
        EXPECT_EQ(topics, expected_topics);
      }));
}
