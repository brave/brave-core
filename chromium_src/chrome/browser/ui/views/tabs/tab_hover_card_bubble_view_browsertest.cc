/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/strings/string_util.h"
#include "base/test/scoped_feature_list.h"
#include "chrome/browser/ui/test/test_browser_dialog.h"
#include "chrome/browser/ui/ui_features.h"
#include "chrome/browser/ui/views/tabs/tab_hover_card_bubble_view.h"
#include "chrome/browser/ui/views/tabs/tab_hover_card_controller.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "ui/gfx/animation/animation_test_api.h"
#include "ui/views/controls/label.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_observer.h"

using views::Widget;

// Helper to wait until the hover card widget is visible.
class HoverCardVisibleWaiter : public views::WidgetObserver {
 public:
  explicit HoverCardVisibleWaiter(Widget* hover_card)
      : hover_card_(hover_card) {
    hover_card_->AddObserver(this);
  }
  ~HoverCardVisibleWaiter() override { hover_card_->RemoveObserver(this); }

  void Wait() {
    if (hover_card_->IsVisible())
      return;
    run_loop_.Run();
  }

  // WidgetObserver overrides:
  void OnWidgetVisibilityChanged(Widget* widget, bool visible) override {
    if (visible)
      run_loop_.Quit();
  }

 private:
  Widget* const hover_card_;
  base::RunLoop run_loop_;
};

class TabHoverCardBubbleViewBrowserTest : public DialogBrowserTest {
 public:
  TabHoverCardBubbleViewBrowserTest()
      : animation_mode_reset_(gfx::AnimationTestApi::SetRichAnimationRenderMode(
            gfx::Animation::RichAnimationRenderMode::FORCE_DISABLED)) {
    TabHoverCardController::disable_animations_for_testing_ = true;
  }
  ~TabHoverCardBubbleViewBrowserTest() override = default;

  void SetUp() override {
    scoped_feature_list_.InitAndEnableFeature(features::kTabHoverCards);
    DialogBrowserTest::SetUp();
  }

  TabHoverCardBubbleView* GetHoverCard(const TabStrip* tab_strip) {
    return tab_strip->hover_card_controller_->hover_card_;
  }

  static Widget* GetHoverCardWidget(TabHoverCardBubbleView* hover_card) {
    return hover_card->GetWidget();
  }

  const base::string16& GetHoverCardTitle(
      const TabHoverCardBubbleView* hover_card) {
    return hover_card->title_label_->GetText();
  }

  const base::string16& GetHoverCardDomain(
      const TabHoverCardBubbleView* hover_card) {
    return hover_card->domain_label_->GetText();
  }

  void HoverMouseOverTabAt(TabStrip* tab_strip, int index) {
    // We don't use Tab::OnMouseEntered here to invoke the hover card because
    // that path is disabled in browser tests. If we enabled it, the real mouse
    // might interfere with the test.
    tab_strip->UpdateHoverCard(tab_strip->tab_at(index),
                               TabController::HoverCardUpdateType::kHover);
  }

  // DialogBrowserTest:
  void ShowUi(const std::string& name) override {
    TabStrip* tab_strip =
        BrowserView::GetBrowserViewForBrowser(browser())->tabstrip();
    HoverMouseOverTabAt(tab_strip, 0);
    TabHoverCardBubbleView* hover_card = GetHoverCard(tab_strip);
    Widget* widget = GetHoverCardWidget(hover_card);
    HoverCardVisibleWaiter waiter(widget);
    waiter.Wait();
  }

 private:
  std::unique_ptr<base::AutoReset<gfx::Animation::RichAnimationRenderMode>>
      animation_mode_reset_;
  base::test::ScopedFeatureList scoped_feature_list_;

  DISALLOW_COPY_AND_ASSIGN(TabHoverCardBubbleViewBrowserTest);
};

// This test appears to be flaky on Windows. Upstream tests were also found to
// be flaky, so for now we'll follow the upstream lead and disable this one on
// Windows. See crbug.com/1050765.
#if defined(OS_WIN)
#define MAYBE_ChromeSchemeUrl DISABLED_ChromeSchemeUrl
#else
#define MAYBE_ChromeSchemeUrl ChromeSchemeUrl
#endif
IN_PROC_BROWSER_TEST_F(TabHoverCardBubbleViewBrowserTest,
                       MAYBE_ChromeSchemeUrl) {
  TabStrip* tab_strip =
      BrowserView::GetBrowserViewForBrowser(browser())->tabstrip();
  TabRendererData new_tab_data = TabRendererData();
  new_tab_data.title = base::UTF8ToUTF16("Settings - Addresses and more");
  new_tab_data.last_committed_url =
      GURL("chrome://settings/addresses");
  tab_strip->AddTabAt(1, new_tab_data, false);

  ShowUi("default");
  TabHoverCardBubbleView* hover_card = GetHoverCard(tab_strip);
  Widget* widget = GetHoverCardWidget(hover_card);
  EXPECT_TRUE(widget != nullptr);
  EXPECT_TRUE(widget->IsVisible());
  HoverMouseOverTabAt(tab_strip, 1);
  EXPECT_EQ(GetHoverCardTitle(hover_card),
            base::UTF8ToUTF16("Settings - Addresses and more"));
  EXPECT_EQ(GetHoverCardDomain(hover_card),
            base::UTF8ToUTF16("brave://settings"));
}
