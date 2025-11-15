/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/webui/tab_search/tab_search_page_handler.h"

#include "base/strings/string_number_conversions.h"
#include "base/test/bind.h"
#include "base/test/gmock_callback_support.h"
#include "base/test/mock_callback.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "components/grit/brave_components_strings.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/gurl.h"
#include "url/origin.h"

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "brave/browser/ai_chat/ai_chat_service_factory.h"
#include "brave/components/ai_chat/core/browser/ai_chat_credential_manager.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/browser/constants.h"
#include "brave/components/ai_chat/core/browser/engine/mock_engine_consumer.h"
#include "brave/components/ai_chat/core/browser/types.h"
#endif

#include <chrome/browser/ui/webui/tab_search/tab_search_page_handler_unittest.cc>

namespace {

#if BUILDFLAG(ENABLE_AI_CHAT)
constexpr char kFooDotComUrl1[] = "https://foo.com/1";
constexpr char kFooDotComUrl2[] = "https://foo.com/2";
constexpr char kBarDotComUrl1[] = "https://bar.com/1";
constexpr char kBarDotComUrl2[] = "https://bar.com/2";
constexpr char kCatDotComUrl1[] = "https://cat.com/1";
constexpr char kCatDotComUrl2[] = "https://cat.com/2";

constexpr char kFooDotComTitle1[] = "foo.com 1";
constexpr char kFooDotComTitle2[] = "foo.com 2";
constexpr char kBarDotComTitle1[] = "bar.com 1";
constexpr char kBarDotComTitle2[] = "bar.com 2";
constexpr char kCatDotComTitle1[] = "cat.com 1";
constexpr char kCatDotComTitle2[] = "cat.com 2";

class MockAIChatCredentialManager : public ai_chat::AIChatCredentialManager {
 public:
  using ai_chat::AIChatCredentialManager::AIChatCredentialManager;
  MOCK_METHOD(void,
              GetPremiumStatus,
              (ai_chat::mojom::Service::GetPremiumStatusCallback callback),
              (override));
};
#endif

}  // namespace

#if BUILDFLAG(ENABLE_AI_CHAT)
TEST_F(TabSearchPageHandlerTest, GetSuggestedTopics) {
  auto* ai_chat_service =
      ai_chat::AIChatServiceFactory::GetForBrowserContext(profile());
  ai_chat_service->SetTabOrganizationEngineForTesting(
      std::make_unique<testing::NiceMock<ai_chat::MockEngineConsumer>>());
  // Disable caching suggested topics, caching using tab tracker service is
  // covered by TabSearchPageHandlerBrowserTest.
  ai_chat_service->SetTabTrackerServiceForTesting(nullptr);

  ai_chat_service->SetCredentialManagerForTesting(
      std::make_unique<testing::NiceMock<MockAIChatCredentialManager>>(
          base::NullCallback(), nullptr));

  auto* mock_credential_manager = static_cast<MockAIChatCredentialManager*>(
      ai_chat_service->GetCredentialManagerForTesting());
  ON_CALL(*mock_credential_manager, GetPremiumStatus(_))
      .WillByDefault(
          [&](ai_chat::mojom::Service::GetPremiumStatusCallback callback) {
            ai_chat::mojom::PremiumInfoPtr premium_info =
                ai_chat::mojom::PremiumInfo::New();
            std::move(callback).Run(ai_chat::mojom::PremiumStatus::Inactive,
                                    std::move(premium_info));
          });

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
      {base::NumberToString(tab_id1), kCatDotComTitle1,
       url::Origin::Create(GURL(kCatDotComUrl1))},
      {base::NumberToString(tab_id2), kCatDotComTitle2,
       url::Origin::Create(GURL(kCatDotComUrl2))},
      {base::NumberToString(tab_id3), kBarDotComTitle2,
       url::Origin::Create(GURL(kBarDotComUrl2))},
  };

  std::vector<std::string> expected_topics = {"topic1", "topic2", "topic3",
                                              "topic4", "topic5"};
  auto* mock_engine = static_cast<ai_chat::MockEngineConsumer*>(
      ai_chat_service->GetTabOrganizationEngineForTesting());
  std::string model_name = ai_chat::kClaudeHaikuModelName;
  EXPECT_CALL(*mock_engine, GetModelName())
      .WillRepeatedly(testing::ReturnRef(model_name));
  EXPECT_CALL(*mock_engine, GetSuggestedTopics(expected_tabs, _))
      .WillOnce(base::test::RunOnceCallback<1>(expected_topics));

  // Uninteresting calls from upstream.
  EXPECT_CALL(page_, TabsChanged(_)).Times(testing::AnyNumber());
  EXPECT_CALL(page_, TabUpdated(_)).Times(testing::AnyNumber());
  EXPECT_CALL(page_, TabsRemoved(_)).Times(testing::AnyNumber());

  handler()->GetSuggestedTopics(
      base::BindLambdaForTesting([&](const std::vector<std::string>& topics,
                                     tab_search::mojom::ErrorPtr error) {
        EXPECT_EQ(topics, expected_topics);
        EXPECT_FALSE(error);
      }));

  testing::Mock::VerifyAndClearExpectations(mock_engine);

  EXPECT_CALL(*mock_engine, GetModelName())
      .WillOnce(testing::ReturnRef(model_name));
  EXPECT_CALL(*mock_engine, GetSuggestedTopics(expected_tabs, _))
      .WillOnce(base::test::RunOnceCallback<1>(
          base::unexpected(ai_chat::mojom::APIError::RateLimitReached)));

  handler()->GetSuggestedTopics(
      base::BindLambdaForTesting([&](const std::vector<std::string>& topics,
                                     tab_search::mojom::ErrorPtr error) {
        EXPECT_TRUE(topics.empty());
        auto rate_limited_info =
            tab_search::mojom::RateLimitedInfo::New(false /* is_premium */);
        EXPECT_EQ(
            error,
            tab_search::mojom::Error::New(
                l10n_util::GetStringUTF8(IDS_CHAT_UI_RATE_LIMIT_REACHED_DESC),
                std::move(rate_limited_info)));
      }));

  // Uninteresting calls from upstream.
  EXPECT_CALL(page_, TabsChanged(_)).Times(testing::AnyNumber());
  EXPECT_CALL(page_, TabUpdated(_)).Times(testing::AnyNumber());
  EXPECT_CALL(page_, TabsRemoved(_)).Times(testing::AnyNumber());

  testing::Mock::VerifyAndClearExpectations(mock_engine);

  EXPECT_CALL(*mock_engine, GetSuggestedTopics(expected_tabs, _))
      .WillOnce(base::test::RunOnceCallback<1>(
          base::unexpected(ai_chat::mojom::APIError::ConnectionIssue)));
  EXPECT_CALL(*mock_engine, GetModelName())
      .WillOnce(testing::ReturnRef(model_name));

  handler()->GetSuggestedTopics(
      base::BindLambdaForTesting([&](const std::vector<std::string>& topics,
                                     tab_search::mojom::ErrorPtr error) {
        EXPECT_TRUE(topics.empty());
        EXPECT_EQ(
            error,
            tab_search::mojom::Error::New(
                l10n_util::GetStringUTF8(IDS_CHAT_UI_ERROR_NETWORK), nullptr));
      }));

  // Uninteresting calls from upstream.
  EXPECT_CALL(page_, TabsChanged(_)).Times(testing::AnyNumber());
  EXPECT_CALL(page_, TabUpdated(_)).Times(testing::AnyNumber());
  EXPECT_CALL(page_, TabsRemoved(_)).Times(testing::AnyNumber());

  testing::Mock::VerifyAndClearExpectations(mock_engine);
}

TEST_F(TabSearchPageHandlerTest, GetFocusTabs) {
  auto* ai_chat_service =
      ai_chat::AIChatServiceFactory::GetForBrowserContext(profile());
  ai_chat_service->SetTabOrganizationEngineForTesting(
      std::make_unique<testing::NiceMock<ai_chat::MockEngineConsumer>>());

  ai_chat_service->SetCredentialManagerForTesting(
      std::make_unique<testing::NiceMock<MockAIChatCredentialManager>>(
          base::NullCallback(), nullptr));
  auto* mock_credential_manager = static_cast<MockAIChatCredentialManager*>(
      ai_chat_service->GetCredentialManagerForTesting());
  ON_CALL(*mock_credential_manager, GetPremiumStatus(_))
      .WillByDefault(
          [&](ai_chat::mojom::Service::GetPremiumStatusCallback callback) {
            ai_chat::mojom::PremiumInfoPtr premium_info =
                ai_chat::mojom::PremiumInfo::New();
            std::move(callback).Run(ai_chat::mojom::PremiumStatus::Inactive,
                                    std::move(premium_info));
          });

  // Create multiple tabs in different windows and verify GetFocusTabs is called
  // with expected tabs info.
  // Browser with the same profile but not normal type.
  AddTabWithTitle(browser5(), GURL(kFooDotComUrl1), kFooDotComTitle1);
  // Browser with a different profile of the default browser.
  AddTabWithTitle(browser4(), GURL(kFooDotComUrl2), kFooDotComTitle2);
  // Browser with incognito profile.
  AddTabWithTitle(browser3(), GURL(kBarDotComUrl1), kBarDotComTitle1);
  // Browser with the same profile of the default browser.
  AddTabWithTitle(browser2(), GURL(kBarDotComUrl2), kBarDotComTitle2);
  // The default browser.
  AddTabWithTitle(browser1(), GURL(kCatDotComUrl1), kCatDotComTitle1);

  const int tab_id1 =
      browser1()->tab_strip_model()->GetTabAtIndex(0)->GetHandle().raw_value();
  const int tab_id2 =
      browser2()->tab_strip_model()->GetTabAtIndex(0)->GetHandle().raw_value();
  const int tab_id3 =
      browser3()->tab_strip_model()->GetTabAtIndex(0)->GetHandle().raw_value();
  const int tab_id4 =
      browser4()->tab_strip_model()->GetTabAtIndex(0)->GetHandle().raw_value();
  const int tab_id5 =
      browser5()->tab_strip_model()->GetTabAtIndex(0)->GetHandle().raw_value();

  // Only tabs from the same profile and normal window are expected.
  std::vector<ai_chat::Tab> expected_tabs = {
      {base::NumberToString(tab_id1), kCatDotComTitle1,
       url::Origin::Create(GURL(kCatDotComUrl1))},
      {base::NumberToString(tab_id2), kBarDotComTitle2,
       url::Origin::Create(GURL(kBarDotComUrl2))},
  };

  // Only covers the case where the returned tab ID is invalid.
  // 1) Tab ID is not found.
  // 2) Tab in the incognito window.
  // 3) Tab in the non-normal window.
  // 4) Tab in another profile.
  // This tests the error handling in OnGetFocusTabs. The valid case is covered
  // in TabSearchPageHandlerBrowserTest so a real browser window can be created.
  std::vector<std::string> mock_ret_tabs = {
      "100", "invalid", base::NumberToString(tab_id3),
      base::NumberToString(tab_id4), base::NumberToString(tab_id5)};
  auto* mock_engine = static_cast<ai_chat::MockEngineConsumer*>(
      ai_chat_service->GetTabOrganizationEngineForTesting());
  std::string model_name = ai_chat::kClaudeHaikuModelName;
  EXPECT_CALL(*mock_engine, GetModelName())
      .WillOnce(testing::ReturnRef(model_name));
  EXPECT_CALL(*mock_engine, GetFocusTabs(expected_tabs, "topic", _))
      .WillOnce(base::test::RunOnceCallback<2>(mock_ret_tabs));

  handler()->GetFocusTabs("topic", base::BindLambdaForTesting(
                                       [&](bool new_window_created,
                                           tab_search::mojom::ErrorPtr error) {
                                         EXPECT_FALSE(new_window_created);
                                         // We just do nothing in this case.
                                         EXPECT_FALSE(error);
                                       }));

  // Uninteresting calls from upstream.
  EXPECT_CALL(page_, TabsChanged(_)).Times(testing::AnyNumber());
  EXPECT_CALL(page_, TabUpdated(_)).Times(testing::AnyNumber());
  EXPECT_CALL(page_, TabsRemoved(_)).Times(testing::AnyNumber());

  testing::Mock::VerifyAndClearExpectations(mock_engine);

  // Test error.
  EXPECT_CALL(*mock_engine, GetModelName())
      .WillOnce(testing::ReturnRef(model_name));
  EXPECT_CALL(*mock_engine, GetFocusTabs(expected_tabs, "topic", _))
      .WillOnce(base::test::RunOnceCallback<2>(
          base::unexpected(ai_chat::mojom::APIError::RateLimitReached)));

  handler()->GetFocusTabs(
      "topic",
      base::BindLambdaForTesting(
          [&](bool new_window_created, tab_search::mojom::ErrorPtr error) {
            EXPECT_FALSE(new_window_created);
            auto rate_limited_info =
                tab_search::mojom::RateLimitedInfo::New(false /* is_premium */);
            EXPECT_EQ(error, tab_search::mojom::Error::New(
                                 l10n_util::GetStringUTF8(
                                     IDS_CHAT_UI_RATE_LIMIT_REACHED_DESC),
                                 std::move(rate_limited_info)));
          }));

  // Uninteresting calls from upstream.
  EXPECT_CALL(page_, TabsChanged(_)).Times(testing::AnyNumber());
  EXPECT_CALL(page_, TabUpdated(_)).Times(testing::AnyNumber());
  EXPECT_CALL(page_, TabsRemoved(_)).Times(testing::AnyNumber());

  testing::Mock::VerifyAndClearExpectations(mock_engine);
}

TEST_F(TabSearchPageHandlerTest, UndoFocusTabs) {
  // Add tabs in windows with the default profile.
  AddTabWithTitle(browser1(), GURL(kFooDotComUrl2), kFooDotComTitle2);
  AddTabWithTitle(browser1(), GURL(kFooDotComUrl1), kFooDotComTitle1);
  AddTabWithTitle(browser2(), GURL(kCatDotComUrl2), kCatDotComTitle2);
  AddTabWithTitle(browser2(), GURL(kCatDotComUrl1), kCatDotComTitle1);
  AddTabWithTitle(browser2(), GURL(kBarDotComUrl2), kBarDotComTitle2);
  AddTabWithTitle(browser2(), GURL(kBarDotComUrl1), kBarDotComTitle1);

  ASSERT_EQ(browser1()->tab_strip_model()->count(), 2);
  ASSERT_EQ(browser2()->tab_strip_model()->count(), 4);

  const int tab_id1 =
      browser1()->tab_strip_model()->GetTabAtIndex(0)->GetHandle().raw_value();
  const int tab_id2 =
      browser1()->tab_strip_model()->GetTabAtIndex(1)->GetHandle().raw_value();
  const int tab_id3 =
      browser2()->tab_strip_model()->GetTabAtIndex(0)->GetHandle().raw_value();
  const int tab_id4 =
      browser2()->tab_strip_model()->GetTabAtIndex(1)->GetHandle().raw_value();
  const int tab_id5 =
      browser2()->tab_strip_model()->GetTabAtIndex(2)->GetHandle().raw_value();
  const int tab_id6 =
      browser2()->tab_strip_model()->GetTabAtIndex(3)->GetHandle().raw_value();

  // Close tab_id6 to have a non-existing tab ID inside, for example, tab is
  // closed in the new window.
  browser2()->tab_strip_model()->CloseWebContentsAt(3,
                                                    TabCloseTypes::CLOSE_NONE);

  base::flat_map<SessionID, std::vector<TabSearchPageHandler::TabInfo>>
      original_tabs_info;
  // Use browser1's session ID to mock that these tabs were moved from browser1.
  original_tabs_info[browser1()->session_id()] = {
      {tab_id3, 2},
      {tab_id4, 1},
      // Index 5 is bigger than the last index after restored, this can happen
      // when a tab in the original window is closed before undo.
      {tab_id5, 5},
      {tab_id6, 6},
      {100, 5},
  };
  handler()->SetOriginalTabsInfoByWindowForTesting(original_tabs_info);
  handler()->UndoFocusTabs(base::BindLambdaForTesting([&]() {
    Browser* browser1 = browser();
    EXPECT_EQ(browser1->tab_strip_model()->count(), 5)
        << "The tabs should be moved back to the window stored.";
    EXPECT_EQ(
        browser1->tab_strip_model()->GetTabAtIndex(0)->GetHandle().raw_value(),
        tab_id1);
    EXPECT_EQ(
        browser1->tab_strip_model()->GetTabAtIndex(1)->GetHandle().raw_value(),
        tab_id4);
    EXPECT_EQ(
        browser1->tab_strip_model()->GetTabAtIndex(2)->GetHandle().raw_value(),
        tab_id3);
    EXPECT_EQ(
        browser1->tab_strip_model()->GetTabAtIndex(3)->GetHandle().raw_value(),
        tab_id2);
    EXPECT_EQ(
        browser1->tab_strip_model()->GetTabAtIndex(4)->GetHandle().raw_value(),
        tab_id5);

    // We do not wait for the window to be closed and only verify the tabs are
    // moved out here, it is covered in TabSearchPageHandlerBrowserTest.
    EXPECT_EQ(this->browser2()->GetTabStripModel()->count(), 0)
        << "The tabs should be moved back to the window stored.";
  }));

  // Uninteresting calls from upstream.
  EXPECT_CALL(page_, TabsChanged(_)).Times(testing::AnyNumber());
  EXPECT_CALL(page_, TabUpdated(_)).Times(testing::AnyNumber());
  EXPECT_CALL(page_, TabsRemoved(_)).Times(testing::AnyNumber());
}
#endif  // BUILDFLAG(ENABLE_AI_CHAT)
