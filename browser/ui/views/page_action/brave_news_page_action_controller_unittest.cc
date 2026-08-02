// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/page_action/brave_news_page_action_controller.h"

#include <memory>
#include <vector>

#include "base/scoped_observation.h"
#include "brave/browser/brave_news/brave_news_tab_helper.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "chrome/browser/ui/actions/chrome_action_id.h"
#include "chrome/browser/ui/browser_window/test/mock_browser_window_interface.h"
#include "chrome/browser/ui/page_action/page_action_controller.h"
#include "chrome/browser/ui/page_action/page_action_icon_type.h"
#include "chrome/browser/ui/page_action/page_action_model.h"
#include "chrome/browser/ui/page_action/page_action_model_observer.h"
#include "chrome/browser/ui/page_action/test_support/fake_tab_interface.h"
#include "chrome/browser/ui/page_action/test_support/test_page_action_properties_provider.h"
#include "chrome/browser/ui/toolbar/pinned_toolbar/pinned_toolbar_actions_model.h"
#include "chrome/test/base/testing_profile.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/actions/actions.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/gurl.h"

using testing::Return;

namespace page_actions {

namespace {

// Records the most recent state pushed into the action's PageActionModel.
class TestObserver : public PageActionModelObserver {
 public:
  TestObserver() = default;
  ~TestObserver() override = default;

  // PageActionModelObserver:
  void OnPageActionModelChanged(
      const PageActionModelInterface& model) override {
    visible_ = model.GetVisible();
    tooltip_text_ = model.GetTooltipText();
    ++model_change_count_;
  }

  bool visible() const { return visible_; }
  const std::u16string& tooltip_text() const { return tooltip_text_; }
  int model_change_count() const { return model_change_count_; }

 private:
  bool visible_ = false;
  std::u16string tooltip_text_;
  int model_change_count_ = 0;
};

brave_news::mojom::PublisherPtr MakePublisher(const GURL& feed_source,
                                              bool subscribed) {
  auto publisher = brave_news::mojom::Publisher::New();
  publisher->publisher_id = "test-publisher";
  publisher->type = brave_news::mojom::PublisherType::COMBINED_SOURCE;
  publisher->publisher_name = "Test Publisher";
  publisher->feed_source = feed_source;
  publisher->site_url = feed_source;
  publisher->user_enabled_status =
      subscribed ? brave_news::mojom::UserEnabled::ENABLED
                 : brave_news::mojom::UserEnabled::NOT_MODIFIED;
  return publisher;
}

}  // namespace

class BraveNewsPageActionControllerTest : public testing::Test {
 public:
  BraveNewsPageActionControllerTest()
      : properties_provider_(PageActionPropertiesMap{{
            kActionShowBraveNews,
            PageActionProperties{
                .histogram_name = "BraveNews",
                .type = brave::kBraveNewsPageActionIconType,
            },
        }}) {}

  void SetUp() override {
    profile_.GetPrefs()->SetBoolean(brave_news::prefs::kBraveNewsOptedIn, true);
    profile_.GetPrefs()->SetBoolean(brave_news::prefs::kNewTabPageShowToday,
                                    true);

    ON_CALL(browser_window_, GetProfile()).WillByDefault(Return(&profile_));

    pinned_actions_model_ =
        std::make_unique<PinnedToolbarActionsModel>(&profile_);
    tab_interface_ = std::make_unique<FakeTabInterface>(&profile_);
    ON_CALL(*tab_interface_, GetBrowserWindowInterface())
        .WillByDefault(Return(&browser_window_));
    tab_interface_->Activate();

    page_action_controller_ = std::make_unique<PageActionControllerImpl>(
        *tab_interface_, std::vector<actions::ActionId>{kActionShowBraveNews},
        properties_provider_, pinned_actions_model_.get());

    action_item_ = actions::ActionItem::Builder()
                       .SetActionId(kActionShowBraveNews)
                       .SetVisible(true)
                       .SetEnabled(true)
                       .Build();
    action_item_subscription_ =
        page_action_controller_->CreateActionItemSubscription(
            action_item_.get());

    page_action_controller_->AddObserver(kActionShowBraveNews, observation_);

    BraveNewsTabHelper::CreateForWebContents(tab_interface_->GetContents());

    controller_ = std::make_unique<BraveNewsPageActionController>(
        *tab_interface_, *page_action_controller_);
    controller_->Init();
  }

  void TearDown() override {
    observation_.Reset();
    controller_.reset();
    action_item_subscription_ = base::CallbackListSubscription();
    action_item_.reset();
    page_action_controller_.reset();
    tab_interface_.reset();
    pinned_actions_model_.reset();
  }

  BraveNewsTabHelper* tab_helper() {
    return BraveNewsTabHelper::FromWebContents(tab_interface_->GetContents());
  }

  BraveNewsPageActionController* controller() { return controller_.get(); }
  const TestObserver& observer() const { return observer_; }
  PrefService* prefs() { return profile_.GetPrefs(); }

 private:
  content::BrowserTaskEnvironment task_environment_;
  TestPageActionPropertiesProvider properties_provider_;
  TestingProfile profile_;
  testing::NiceMock<MockBrowserWindowInterface> browser_window_;

  std::unique_ptr<PinnedToolbarActionsModel> pinned_actions_model_;
  std::unique_ptr<FakeTabInterface> tab_interface_;
  std::unique_ptr<PageActionControllerImpl> page_action_controller_;
  std::unique_ptr<actions::ActionItem> action_item_;
  base::CallbackListSubscription action_item_subscription_;

  TestObserver observer_;
  base::ScopedObservation<PageActionModelInterface, PageActionModelObserver>
      observation_{&observer_};

  std::unique_ptr<BraveNewsPageActionController> controller_;
};

TEST_F(BraveNewsPageActionControllerTest, HiddenByDefault) {
  EXPECT_FALSE(observer().visible());
}

TEST_F(BraveNewsPageActionControllerTest, VisibleWithFeed) {
  tab_helper()->SetDefaultFeedForTesting(
      MakePublisher(GURL("https://example.com/feed"), /*subscribed=*/false));

  EXPECT_TRUE(observer().visible());
  EXPECT_EQ(observer().tooltip_text(),
            l10n_util::GetStringUTF16(IDS_BRAVE_NEWS_ACTION_VIEW_TOOLTIP));
}

TEST_F(BraveNewsPageActionControllerTest, HiddenWhenNotOptedIn) {
  prefs()->SetBoolean(brave_news::prefs::kBraveNewsOptedIn, false);
  tab_helper()->SetDefaultFeedForTesting(
      MakePublisher(GURL("https://example.com/feed"), /*subscribed=*/true));

  EXPECT_FALSE(observer().visible());
}

TEST_F(BraveNewsPageActionControllerTest, HiddenAgainWhenFeedRemoved) {
  tab_helper()->SetDefaultFeedForTesting(
      MakePublisher(GURL("https://example.com/feed"), /*subscribed=*/false));
  ASSERT_TRUE(observer().visible());

  tab_helper()->SetDefaultFeedForTesting(nullptr);
  EXPECT_FALSE(observer().visible());
}

TEST_F(BraveNewsPageActionControllerTest,
       ExecuteActionNoopWithoutToolbarButtonProvider) {
  controller()->ExecuteAction(/*toolbar_button_provider=*/nullptr,
                              /*item=*/nullptr);
}

}  // namespace page_actions
