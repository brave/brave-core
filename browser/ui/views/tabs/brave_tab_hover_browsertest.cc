/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stddef.h>
#include <memory>
#include <utility>

#include "base/feature_list.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/views/tabs/brave_tab_hover_card_controller.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_renderer_data.h"
#include "chrome/browser/ui/thumbnails/thumbnail_tab_helper.h"
#include "chrome/browser/ui/ui_features.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/tabs/tab.h"
#include "chrome/browser/ui/views/tabs/tab_container.h"
#include "chrome/browser/ui/views/tabs/tab_hover_card_bubble_view.h"
#include "chrome/browser/ui/views/tabs/tab_hover_card_controller.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_user_data.h"
#include "content/public/test/browser_test.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/animation/animation_test_api.h"
#include "ui/gfx/geometry/point.h"
#include "ui/views/test/widget_test.h"
#include "ui/views/widget/widget.h"

using views::Widget;

class BraveTabHoverTest : public InProcessBrowserTest {
 public:
  BraveTabHoverTest()
      : animation_mode_reset_(gfx::AnimationTestApi::SetRichAnimationRenderMode(
            gfx::Animation::RichAnimationRenderMode::FORCE_DISABLED)) {
    TabHoverCardController::disable_animations_for_testing_ = true;
  }

  BraveTabHoverTest(const BraveTabHoverTest&) = delete;
  BraveTabHoverTest& operator=(const BraveTabHoverTest&) = delete;

  content::WebContents* contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  TabHoverCardBubbleView* hover_card() {
    return tabstrip()->hover_card_controller_->hover_card_;
  }

  TabStrip* tabstrip() {
    auto* browser_view = static_cast<BrowserView*>(browser()->window());
    return browser_view->tabstrip();
  }

  Tab* active_tab() {
    TabStripModel* tab_strip_model = browser()->tab_strip_model();
    return tabstrip()->tab_at(tab_strip_model->active_index());
  }

  void HoverOverTab(Tab* tab) {
    // Note: As stated in |tab_hover_card_bubble_view_browsertest.cc| we don't
    // do this with OnMouseEnter because the path is disabled in tests, and we
    // could accidentally mess with the test using the real mouse.
    // Additionally, triggering the test with
    // GetFocusManager()->SetFocusedView(tab) is flakey, and fails semi
    // regularly. Thus, we have to trigger the hover directly.
    tabstrip()->UpdateHoverCard(tab,
                                TabSlotController::HoverCardUpdateType::kHover);
  }

 private:
  std::unique_ptr<base::AutoReset<gfx::Animation::RichAnimationRenderMode>>
      animation_mode_reset_;
};

// There should be no tooltip unless the mode is |TOOLTIP|, as otherwise we'll
// get a tooltip AND a card showing up.
IN_PROC_BROWSER_TEST_F(BraveTabHoverTest,
                       GetTooltipOnlyHasTextWhenHoverModeIsTooltip) {
  TabRendererData data;
  data.visible_url = GURL("https://example.com");
  data.title = u"Hello World";
  tabstrip()->SetTabData(browser()->tab_strip_model()->active_index(), data);
  EXPECT_EQ(u"Hello World", active_tab()->data().title);

  browser()->profile()->GetPrefs()->SetInteger(brave_tabs::kTabHoverMode,
                                               brave_tabs::TabHoverMode::CARD);
  EXPECT_EQ(u"", active_tab()->GetCachedTooltipText());

  browser()->profile()->GetPrefs()->SetInteger(
      brave_tabs::kTabHoverMode, brave_tabs::TabHoverMode::CARD_WITH_PREVIEW);
  EXPECT_EQ(u"", active_tab()->GetCachedTooltipText());

  browser()->profile()->GetPrefs()->SetInteger(
      brave_tabs::kTabHoverMode, brave_tabs::TabHoverMode::TOOLTIP);
  EXPECT_EQ(u"Hello World", active_tab()->GetCachedTooltipText());
}

// The ThumbnailTabHelper needs to be attached in all |TabHoverModes| so that
// we can change between modes safely without restarting.
IN_PROC_BROWSER_TEST_F(BraveTabHoverTest, ThumbnailHelperIsAlwaysAttached) {
  browser()->profile()->GetPrefs()->SetInteger(brave_tabs::kTabHoverMode,
                                               brave_tabs::TabHoverMode::CARD);
  TabRendererData data;
  data.visible_url = GURL("https://card.com");
  data.title = u"Hello World";
  tabstrip()->AddTabAt(0, data);
  EXPECT_EQ(0, browser()->tab_strip_model()->active_index());
  EXPECT_EQ(data.visible_url, active_tab()->data().visible_url);
  EXPECT_NE(nullptr,
            content::WebContentsUserData<ThumbnailTabHelper>::FromWebContents(
                contents()));

  browser()->profile()->GetPrefs()->SetInteger(
      brave_tabs::kTabHoverMode, brave_tabs::TabHoverMode::CARD_WITH_PREVIEW);
  data.visible_url = GURL("https://card-with-preview.com");
  data.title = u"Foo Bar";
  tabstrip()->AddTabAt(0, data);
  EXPECT_EQ(0, browser()->tab_strip_model()->active_index());
  EXPECT_EQ(data.visible_url, active_tab()->data().visible_url);
  EXPECT_NE(nullptr,
            content::WebContentsUserData<ThumbnailTabHelper>::FromWebContents(
                contents()));

  browser()->profile()->GetPrefs()->SetInteger(
      brave_tabs::kTabHoverMode, brave_tabs::TabHoverMode::TOOLTIP);

  data.visible_url = GURL("https://tooltip.com");
  data.title = u"Baf Baz";
  tabstrip()->AddTabAt(0, data);
  EXPECT_EQ(0, tabstrip()->GetActiveIndex());
  EXPECT_EQ(data.visible_url, active_tab()->data().visible_url);
  EXPECT_NE(nullptr,
            content::WebContentsUserData<ThumbnailTabHelper>::FromWebContents(
                contents()));
}

// This is based on |TabHoverCardBubbleViewBrowserTest|. Unfortunately, all the
// tests that do similar things are flakey, and this one is too.
// See crbug.com/1050765.
#if BUILDFLAG(IS_WIN)
#define MAYBE_HoverModesAreCorrectlyConfigured \
  DISABLED_HoverModesAreCorrectlyConfigured
#else
#define MAYBE_HoverModesAreCorrectlyConfigured HoverModesAreCorrectlyConfigured
#endif
IN_PROC_BROWSER_TEST_F(BraveTabHoverTest,
                       MAYBE_HoverModesAreCorrectlyConfigured) {
  // In Card mode, the widget should become visible but the thumbnail should not
  // be created.
  browser()->profile()->GetPrefs()->SetInteger(brave_tabs::kTabHoverMode,
                                               brave_tabs::TabHoverMode::CARD);
  HoverOverTab(active_tab());

  Widget* widget = hover_card()->GetWidget();
  ASSERT_NE(nullptr, widget);
  views::test::WidgetVisibleWaiter(widget).Wait();
  EXPECT_FALSE(hover_card()->has_thumbnail_view());
  EXPECT_TRUE(widget->IsVisible());

  // Clear focus, to reset the bubble.
  HoverOverTab(nullptr);
  EXPECT_FALSE(widget->IsVisible());

  // In Preview mode, the widget should become visible and the card should have
  // a thumbnail view.
  browser()->profile()->GetPrefs()->SetInteger(
      brave_tabs::kTabHoverMode, brave_tabs::TabHoverMode::CARD_WITH_PREVIEW);
  HoverOverTab(active_tab());
  widget = hover_card()->GetWidget();
  ASSERT_NE(nullptr, widget);
  views::test::WidgetVisibleWaiter(widget).Wait();
  EXPECT_TRUE(hover_card()->has_thumbnail_view());
  EXPECT_TRUE(widget->IsVisible());

  // Clear focus, to hide the bubble.
  HoverOverTab(nullptr);
  EXPECT_FALSE(widget->IsVisible());

  // In Tooltip mode, the widget should not be made visible.
  browser()->profile()->GetPrefs()->SetInteger(
      brave_tabs::kTabHoverMode, brave_tabs::TabHoverMode::TOOLTIP);
  HoverOverTab(active_tab());
  widget = hover_card()->GetWidget();
  ASSERT_NE(nullptr, widget);
  EXPECT_FALSE(widget->IsVisible());
}

IN_PROC_BROWSER_TEST_F(BraveTabHoverTest, ChromeFeatureDisabledByDefault) {
  EXPECT_FALSE(base::FeatureList::IsEnabled(features::kTabHoverCardImages));
}

class BraveTabHoverTestWithChromeFlag : public BraveTabHoverTest {
 public:
  BraveTabHoverTestWithChromeFlag() {
    features_.InitAndEnableFeature(features::kTabHoverCardImages);
  }

 private:
  base::test::ScopedFeatureList features_;
};

// See crbug.com/1050765.
#if BUILDFLAG(IS_WIN)
#define MAYBE_ChromeFeatureForcesPreviews DISABLED_ChromeFeatureForcesPreviews
#else
#define MAYBE_ChromeFeatureForcesPreviews ChromeFeatureForcesPreviews
#endif
IN_PROC_BROWSER_TEST_F(BraveTabHoverTestWithChromeFlag,
                       MAYBE_ChromeFeatureForcesPreviews) {
  EXPECT_TRUE(base::FeatureList::IsEnabled(features::kTabHoverCardImages));

  // In Card mode, the widget should become visible and because the
  // |kTabHoverCardImages| feature is enabled, the preview should be created.
  browser()->profile()->GetPrefs()->SetInteger(brave_tabs::kTabHoverMode,
                                               brave_tabs::TabHoverMode::CARD);
  HoverOverTab(active_tab());

  Widget* widget = hover_card()->GetWidget();
  ASSERT_NE(nullptr, widget);
  views::test::WidgetVisibleWaiter(widget).Wait();
  EXPECT_TRUE(hover_card()->has_thumbnail_view());
  EXPECT_TRUE(widget->IsVisible());

  // Clear focus, to hide the bubble.
  HoverOverTab(nullptr);
  EXPECT_FALSE(widget->IsVisible());

  // In Preview mode, both flags are set to enable the preview.
  browser()->profile()->GetPrefs()->SetInteger(
      brave_tabs::kTabHoverMode, brave_tabs::TabHoverMode::CARD_WITH_PREVIEW);
  HoverOverTab(active_tab());

  widget = hover_card()->GetWidget();
  ASSERT_NE(nullptr, widget);
  views::test::WidgetVisibleWaiter(widget).Wait();
  EXPECT_TRUE(hover_card()->has_thumbnail_view());
  EXPECT_TRUE(widget->IsVisible());

  // Clear focus, to hide the bubble.
  HoverOverTab(nullptr);
  EXPECT_FALSE(widget->IsVisible());

  // In Tooltip mode, the widget should not be made visible.
  browser()->profile()->GetPrefs()->SetInteger(
      brave_tabs::kTabHoverMode, brave_tabs::TabHoverMode::TOOLTIP);
  HoverOverTab(active_tab());
  widget = hover_card()->GetWidget();
  ASSERT_NE(nullptr, widget);
  EXPECT_FALSE(widget->IsVisible());
}
