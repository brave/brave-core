/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/page_info/brave_page_info_bubble_view.h"

#include "base/test/scoped_feature_list.h"
#include "brave/browser/ui/page_info/features.h"
#include "brave/browser/ui/views/page_info/brave_page_info_view_ids.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "chrome/browser/ui/views/location_bar/location_icon_view.h"
#include "chrome/browser/ui/views/page_info/page_info_bubble_view.h"
#include "chrome/browser/ui/views/page_info/page_info_view_factory.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "content/public/test/browser_test.h"
#include "ui/events/test/test_event.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/test/button_test_api.h"
#include "ui/views/test/widget_test.h"
#include "ui/views/view_utils.h"

namespace {

// Opens the page info bubble.
void OpenPageInfoBubble(Browser* browser) {
  BrowserView* browser_view = BrowserView::GetBrowserViewForBrowser(browser);
  LocationIconView* location_icon_view =
      browser_view->toolbar()->location_bar()->location_icon_view();
  ASSERT_TRUE(location_icon_view);
  ui::test::TestEvent event;
  location_icon_view->ShowBubble(event);
  views::BubbleDialogDelegateView* page_info =
      PageInfoBubbleView::GetPageInfoBubbleForTesting();
  ASSERT_TRUE(page_info);
  page_info->set_close_on_deactivate(false);
}

BravePageInfoBubbleView* GetBubbleView() {
  return views::AsViewClass<BravePageInfoBubbleView>(
      PageInfoBubbleView::GetPageInfoBubbleForTesting());
}

void ClickButton(views::View* bubble_view, int button_id) {
  auto* button = views::Button::AsButton(bubble_view->GetViewByID(button_id));
  ASSERT_TRUE(button);
  views::test::ButtonTestApi(button).NotifyClick(
      ui::MouseEvent(ui::EventType::kMousePressed, gfx::Point(), gfx::Point(),
                     base::TimeTicks(), ui::EF_LEFT_MOUSE_BUTTON, 0));
}

}  // namespace

class BravePageInfoBubbleViewBrowserTestBase : public InProcessBrowserTest {
 public:
  BravePageInfoBubbleViewBrowserTestBase() = default;
  ~BravePageInfoBubbleViewBrowserTestBase() override = default;

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    ASSERT_TRUE(embedded_test_server()->Start());
  }

 protected:
  void NavigateAndOpenBubble() {
    // Navigate to a test page.
    GURL test_url = embedded_test_server()->GetURL("/test.html");
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), test_url));

    // Set a site permission, so that the page info permissions subpage has an
    // entry to display.
    HostContentSettingsMapFactory::GetForProfile(browser()->profile())
        ->SetContentSettingDefaultScope(test_url, test_url,
                                        ContentSettingsType::GEOLOCATION,
                                        CONTENT_SETTING_ALLOW);

    // Open the page info bubble.
    OpenPageInfoBubble(browser());
    auto* bubble_view = GetBubbleView();
    ASSERT_TRUE(bubble_view);
  }

  bool IsSiteSettingsViewDrawn(views::View* bubble_view) {
    auto* site_settings_view = bubble_view->GetViewByID(
        PageInfoViewFactory::VIEW_ID_PAGE_INFO_CURRENT_VIEW);
    return site_settings_view && site_settings_view->IsDrawn();
  }
};

class BravePageInfoBubbleViewBrowserTest
    : public BravePageInfoBubbleViewBrowserTestBase {
 public:
  BravePageInfoBubbleViewBrowserTest() {
    feature_list_.InitAndEnableFeature(
        page_info::features::kShowBraveShieldsInPageInfo);
  }

  ~BravePageInfoBubbleViewBrowserTest() override = default;

 private:
  base::test::ScopedFeatureList feature_list_;
};

// Test that the close button is hidden.
IN_PROC_BROWSER_TEST_F(BravePageInfoBubbleViewBrowserTest, CloseButtonHidden) {
  NavigateAndOpenBubble();
  auto* bubble_view = GetBubbleView();
  ASSERT_TRUE(bubble_view);

  // Verify that the close button exists but is not visible.
  views::View* close_button = bubble_view->GetViewByID(
      PageInfoViewFactory::VIEW_ID_PAGE_INFO_CLOSE_BUTTON);
  ASSERT_TRUE(close_button);
  EXPECT_FALSE(close_button->GetVisible());
}

// Test that the Shields info is visible by default.
IN_PROC_BROWSER_TEST_F(BravePageInfoBubbleViewBrowserTest,
                       ShieldsPageVisibleByDefault) {
  NavigateAndOpenBubble();
  auto* bubble_view = GetBubbleView();
  ASSERT_TRUE(bubble_view);

  EXPECT_FALSE(IsSiteSettingsViewDrawn(bubble_view));
}

// Test that the site settings tab is active and the site settings are displayed
// when a page info subpage is programmatically shown.
IN_PROC_BROWSER_TEST_F(BravePageInfoBubbleViewBrowserTest,
                       SiteSettingsVisibleWhenSubpageOpened) {
  NavigateAndOpenBubble();
  auto* bubble_view = GetBubbleView();
  ASSERT_TRUE(bubble_view);
  bubble_view->OpenPermissionPage(ContentSettingsType::GEOLOCATION);

  // The site settings UI should be visible.
  EXPECT_TRUE(IsSiteSettingsViewDrawn(bubble_view));
}

// Test the behavior of the tab switcher.
IN_PROC_BROWSER_TEST_F(BravePageInfoBubbleViewBrowserTest, TabSwitching) {
  NavigateAndOpenBubble();
  auto* bubble_view = GetBubbleView();
  ASSERT_TRUE(bubble_view);

  // Verify that both tab switcher buttons exist.
  EXPECT_TRUE(bubble_view->GetViewByID(
      static_cast<int>(BravePageInfoViewID::kTabSwitcherShieldsButton)));
  EXPECT_TRUE(bubble_view->GetViewByID(
      static_cast<int>(BravePageInfoViewID::kTabSwitcherSiteSettingsButton)));

  // After clicking the Site Settings button, the site settings view should be
  // visible.
  ClickButton(
      bubble_view,
      static_cast<int>(BravePageInfoViewID::kTabSwitcherSiteSettingsButton));
  EXPECT_TRUE(IsSiteSettingsViewDrawn(bubble_view));

  // After clicking the Shields button, the site settings view should be hidden
  // again.
  ClickButton(bubble_view,
              static_cast<int>(BravePageInfoViewID::kTabSwitcherShieldsButton));
  EXPECT_FALSE(IsSiteSettingsViewDrawn(bubble_view));
}

class BravePageInfoBubbleViewFlagDisabledBrowserTest
    : public BravePageInfoBubbleViewBrowserTestBase {
 public:
  BravePageInfoBubbleViewFlagDisabledBrowserTest() {
    feature_list_.InitAndDisableFeature(
        page_info::features::kShowBraveShieldsInPageInfo);
  }

  ~BravePageInfoBubbleViewFlagDisabledBrowserTest() override = default;

 private:
  base::test::ScopedFeatureList feature_list_;
};

// Test that the close button is visible when the feature flag is disabled.
IN_PROC_BROWSER_TEST_F(BravePageInfoBubbleViewFlagDisabledBrowserTest,
                       CloseButtonVisible) {
  NavigateAndOpenBubble();
  auto* bubble_view = GetBubbleView();
  ASSERT_TRUE(bubble_view);

  // Verify that the close button exists and is visible.
  views::View* close_button = bubble_view->GetViewByID(
      PageInfoViewFactory::VIEW_ID_PAGE_INFO_CLOSE_BUTTON);
  ASSERT_TRUE(close_button);
  EXPECT_TRUE(close_button->GetVisible());
}

// Test that the tab switcher is not present when the feature flag is disabled.
IN_PROC_BROWSER_TEST_F(BravePageInfoBubbleViewFlagDisabledBrowserTest,
                       TabSwitcherNotPresent) {
  NavigateAndOpenBubble();
  auto* bubble_view = GetBubbleView();
  ASSERT_TRUE(bubble_view);

  // Verify that both tab switcher buttons do not exist.
  EXPECT_FALSE(bubble_view->GetViewByID(
      static_cast<int>(BravePageInfoViewID::kTabSwitcherShieldsButton)));
  EXPECT_FALSE(bubble_view->GetViewByID(
      static_cast<int>(BravePageInfoViewID::kTabSwitcherSiteSettingsButton)));

  // Verify that the site settings pages are visible.
  EXPECT_TRUE(IsSiteSettingsViewDrawn(bubble_view));
}
