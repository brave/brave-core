/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_command_controller.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/test/test_browser_dialog.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/tabs/tab.h"
#include "chrome/browser/ui/views/tabs/tab_close_button.h"
#include "chrome/browser/ui/views/tabs/tab_hover_card_bubble_view.h"
#include "chrome/browser/ui/views/tabs/tab_hover_card_controller.h"
#include "chrome/browser/ui/views/tabs/tab_hover_card_metrics.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "ui/gfx/animation/animation_test_api.h"
#include "ui/views/controls/label.h"
#include "ui/views/test/widget_test.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_observer.h"

using views::Widget;

class TabHoverCardBubbleViewBrowserTest : public DialogBrowserTest {
 public:
  TabHoverCardBubbleViewBrowserTest()
      : animation_mode_reset_(gfx::AnimationTestApi::SetRichAnimationRenderMode(
            gfx::Animation::RichAnimationRenderMode::FORCE_DISABLED)) {
    TabHoverCardController::disable_animations_for_testing_ = true;
  }
  TabHoverCardBubbleViewBrowserTest(const TabHoverCardBubbleViewBrowserTest&) =
      delete;
  TabHoverCardBubbleViewBrowserTest& operator=(
      const TabHoverCardBubbleViewBrowserTest&) = delete;
  ~TabHoverCardBubbleViewBrowserTest() override = default;

  void SetUpOnMainThread() override {
    DialogBrowserTest::SetUpOnMainThread();
    tab_strip_ = BrowserView::GetBrowserViewForBrowser(browser())->tabstrip();
  }

  TabStrip* tab_strip() { return tab_strip_; }

  TabHoverCardBubbleView* hover_card() {
    return tab_strip()->hover_card_controller_->hover_card_;
  }

  std::u16string GetHoverCardTitle() {
    return hover_card()->GetTitleTextForTesting();
  }

  std::u16string GetHoverCardDomain() {
    return hover_card()->GetDomainTextForTesting();
  }

  int GetHoverCardsSeenCount() {
    return tab_strip()
        ->hover_card_controller_->metrics_for_testing()
        ->cards_seen_count();
  }

  void MouseExitTabStrip() {
    ui::MouseEvent stop_hover_event(ui::ET_MOUSE_EXITED, gfx::Point(),
                                    gfx::Point(), base::TimeTicks(),
                                    ui::EF_NONE, 0);
    tab_strip()->OnMouseExited(stop_hover_event);
  }

  void ClickMouseOnTab(int index) {
    Tab* const tab = tab_strip()->tab_at(index);
    ui::MouseEvent click_event(ui::ET_MOUSE_PRESSED, gfx::Point(), gfx::Point(),
                               base::TimeTicks(), ui::EF_NONE, 0);
    tab->OnMousePressed(click_event);
  }

  void HoverMouseOverTabAt(int index) {
    // We don't use Tab::OnMouseEntered here to invoke the hover card because
    // that path is disabled in browser tests. If we enabled it, the real mouse
    // might interfere with the test.
    tab_strip()->UpdateHoverCard(
        tab_strip()->tab_at(index),
        TabSlotController::HoverCardUpdateType::kHover);
  }

  // DialogBrowserTest:
  void ShowUi(const std::string& name) override {
    HoverMouseOverTabAt(0);
    views::test::WidgetVisibleWaiter(hover_card()->GetWidget()).Wait();
  }

 private:
  std::unique_ptr<base::AutoReset<gfx::Animation::RichAnimationRenderMode>>
      animation_mode_reset_;

  TabStrip* tab_strip_ = nullptr;
};

// This test appears to be flaky on Windows. Upstream tests were also found to
// be flaky, so for now we'll follow the upstream lead and disable this one on
// Windows. See crbug.com/1050765.
#if BUILDFLAG(IS_WIN)
#define MAYBE_ChromeSchemeUrl DISABLED_ChromeSchemeUrl
#else
#define MAYBE_ChromeSchemeUrl ChromeSchemeUrl
#endif
IN_PROC_BROWSER_TEST_F(TabHoverCardBubbleViewBrowserTest,
                       MAYBE_ChromeSchemeUrl) {
  TabRendererData new_tab_data = TabRendererData();
  new_tab_data.title = u"Settings - Addresses and more";
  new_tab_data.last_committed_url = GURL("chrome://settings/addresses");
  tab_strip()->AddTabAt(1, new_tab_data);

  ShowUi("default");
  Widget* widget = hover_card()->GetWidget();
  EXPECT_TRUE(widget != nullptr);
  EXPECT_TRUE(widget->IsVisible());
  HoverMouseOverTabAt(1);
  EXPECT_EQ(GetHoverCardTitle(), u"Settings - Addresses and more");
  EXPECT_EQ(GetHoverCardDomain(), u"brave://settings");
}
