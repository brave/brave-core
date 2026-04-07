// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/settings/brave_clear_browsing_data_handler.h"

#include <memory>

#include "base/test/bind.h"
#include "base/values.h"
#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/components/brave_ads/core/browser/service/test/ads_service_mock.h"
#include "brave/components/brave_rewards/core/pref_names.h"
#include "chrome/test/base/testing_profile.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_web_ui.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace settings {

class TestingBraveClearBrowsingDataHandler
    : public BraveClearBrowsingDataHandler {
 public:
  explicit TestingBraveClearBrowsingDataHandler(Profile* profile)
      : BraveClearBrowsingDataHandler(&test_web_ui_, profile) {
    web_contents_ = content::WebContents::Create(
        content::WebContents::CreateParams(profile));
    test_web_ui_.set_web_contents(web_contents_.get());
    set_web_ui(&test_web_ui_);
  }

  ~TestingBraveClearBrowsingDataHandler() override { set_web_ui(nullptr); }

  content::TestWebUI* test_web_ui() { return &test_web_ui_; }

  size_t javascript_allowed_count() const { return javascript_allowed_count_; }

  void HandleGetBraveRewardsEnabled(const base::ListValue& args) {
    BraveClearBrowsingDataHandler::HandleGetBraveRewardsEnabled(args);
  }

  void HandleClearBraveAdsData(const base::ListValue& args) {
    BraveClearBrowsingDataHandler::HandleClearBraveAdsData(args);
  }

 private:
  // The base `ClearBrowsingDataHandler::RegisterMessages` registers handlers
  // via `WebUI`, which requires services not available in unit tests.
  void RegisterMessages() override {}

  // The base `ClearBrowsingDataHandler::OnJavascriptAllowed` observes
  // `TemplateURLService` and adds browsing data counters, and
  // `OnJavascriptDisallowed` tears them down, both of which require services
  // not available in unit tests.
  void OnJavascriptAllowed() override { ++javascript_allowed_count_; }
  void OnJavascriptDisallowed() override { --javascript_allowed_count_; }

  std::unique_ptr<content::WebContents> web_contents_;
  content::TestWebUI test_web_ui_;

  // Tracks allow/disallow calls. Expected to be 0 (disallowed) or 1 (allowed);
  // a value greater than 1 indicates a developer called `AllowJavascript`
  // more than once without a corresponding `DisallowJavascript`.
  size_t javascript_allowed_count_ = 0;
};

class BraveClearBrowsingDataHandlerTest : public testing::Test {
 protected:
  content::BrowserTaskEnvironment browser_task_environment_;

  std::unique_ptr<TestingProfile> profile_ = TestingProfile::Builder().Build();
};

TEST_F(BraveClearBrowsingDataHandlerTest,
       HandleGetBraveRewardsEnabledResolvesWithFalseWhenDisabled) {
  // Arrange
  profile_->GetPrefs()->SetBoolean(brave_rewards::prefs::kEnabled, false);

  TestingBraveClearBrowsingDataHandler handler(profile_.get());

  // Act
  handler.HandleGetBraveRewardsEnabled(
      /*args=*/base::ListValue().Append(/*callback_id*/ "foo"));

  // Assert
  EXPECT_EQ(1U, handler.javascript_allowed_count());
  const content::TestWebUI::CallData& call_data =
      *handler.test_web_ui()->call_data().back();
  EXPECT_EQ("cr.webUIResponse", call_data.function_name());
  EXPECT_EQ(/*callback_id*/ "foo", call_data.arg1()->GetString());
  EXPECT_TRUE(/*resolved*/ call_data.arg2()->GetBool());
  EXPECT_FALSE(/*rewards_enabled*/ call_data.arg3()->GetBool());
}

TEST_F(BraveClearBrowsingDataHandlerTest,
       HandleGetBraveRewardsEnabledResolvesWithTrueWhenEnabled) {
  // Arrange
  profile_->GetPrefs()->SetBoolean(brave_rewards::prefs::kEnabled, true);

  TestingBraveClearBrowsingDataHandler handler(profile_.get());

  // Act
  handler.HandleGetBraveRewardsEnabled(
      /*args=*/base::ListValue().Append(/*callback_id*/ "foo"));

  // Assert
  EXPECT_EQ(1U, handler.javascript_allowed_count());
  const content::TestWebUI::CallData& call_data =
      *handler.test_web_ui()->call_data().back();
  EXPECT_EQ("cr.webUIResponse", call_data.function_name());
  EXPECT_EQ(/*callback_id*/ "foo", call_data.arg1()->GetString());
  EXPECT_TRUE(/*resolved*/ call_data.arg2()->GetBool());
  EXPECT_TRUE(/*rewards_enabled*/ call_data.arg3()->GetBool());
}

TEST_F(BraveClearBrowsingDataHandlerTest,
       HandleClearBraveAdsDataWithNullServiceDoesNotCrash) {
  // Arrange
  TestingBraveClearBrowsingDataHandler handler(profile_.get());

  // Act & Assert
  handler.HandleClearBraveAdsData(/*args=*/{});
}

TEST_F(BraveClearBrowsingDataHandlerTest,
       HandleClearBraveAdsDataCallsClearDataOnAdsService) {
  // Arrange
  brave_ads::AdsServiceFactory::GetInstance()->SetTestingFactory(
      profile_.get(),
      base::BindLambdaForTesting([](content::BrowserContext* /*context*/)
                                     -> std::unique_ptr<KeyedService> {
        return std::make_unique<brave_ads::AdsServiceMock>();
      }));

  auto* const ads_service_mock = static_cast<brave_ads::AdsServiceMock*>(
      brave_ads::AdsServiceFactory::GetForProfile(profile_.get()));

  TestingBraveClearBrowsingDataHandler handler(profile_.get());

  EXPECT_CALL(*ads_service_mock, ClearData);

  // Act
  handler.HandleClearBraveAdsData(/*args=*/{});
}

TEST_F(BraveClearBrowsingDataHandlerTest,
       RewardsEnabledPrefChangeFiresListenerWhenJavascriptAllowed) {
  // Arrange
  profile_->GetPrefs()->SetBoolean(brave_rewards::prefs::kEnabled, false);

  TestingBraveClearBrowsingDataHandler handler(profile_.get());
  handler.AllowJavascriptForTesting();

  // Act
  profile_->GetPrefs()->SetBoolean(brave_rewards::prefs::kEnabled, true);

  // Assert
  EXPECT_EQ(1U, handler.javascript_allowed_count());
  const content::TestWebUI::CallData& call_data =
      *handler.test_web_ui()->call_data().back();
  EXPECT_EQ("cr.webUIListenerCallback", call_data.function_name());
  EXPECT_EQ("brave-rewards-enabled-changed",
            /*event_name*/ call_data.arg1()->GetString());
  EXPECT_TRUE(/*rewards_enabled*/ call_data.arg2()->GetBool());
}

TEST_F(BraveClearBrowsingDataHandlerTest,
       RewardsEnabledPrefChangeDoesNotFireListenerAfterJavascriptDisallowed) {
  // Arrange
  profile_->GetPrefs()->SetBoolean(brave_rewards::prefs::kEnabled, false);

  TestingBraveClearBrowsingDataHandler handler(profile_.get());
  handler.AllowJavascriptForTesting();
  handler.DisallowJavascript();

  // Act
  profile_->GetPrefs()->SetBoolean(brave_rewards::prefs::kEnabled, true);

  // Assert
  EXPECT_EQ(0U, handler.javascript_allowed_count());
  EXPECT_TRUE(handler.test_web_ui()->call_data().empty());
}

TEST_F(BraveClearBrowsingDataHandlerTest,
       RewardsEnabledPrefChangeDoesNotFireListenerWhenJavascriptNotAllowed) {
  // Arrange
  profile_->GetPrefs()->SetBoolean(brave_rewards::prefs::kEnabled, false);

  TestingBraveClearBrowsingDataHandler handler(profile_.get());

  // Act
  profile_->GetPrefs()->SetBoolean(brave_rewards::prefs::kEnabled, true);

  // Assert
  EXPECT_EQ(0U, handler.javascript_allowed_count());
  EXPECT_TRUE(handler.test_web_ui()->call_data().empty());
}

}  // namespace settings
