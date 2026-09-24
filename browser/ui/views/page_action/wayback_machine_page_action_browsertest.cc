// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/test/scoped_feature_list.h"
#include "brave/browser/ui/tabs/public/brave_tab_features.h"
#include "brave/browser/ui/views/page_action/wayback_machine_bubble_view.h"
#include "brave/browser/ui/views/page_action/wayback_machine_page_action_controller.h"
#include "brave/components/brave_wayback_machine/brave_wayback_machine_tab_helper.h"
#include "brave/components/brave_wayback_machine/features.h"
#include "brave/components/brave_wayback_machine/pref_names.h"
#include "brave/components/brave_wayback_machine/wayback_state.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/actions/chrome_action_id.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/toolbar_button_provider.h"
#include "chrome/browser/ui/views/location_bar/icon_label_bubble_view.h"
#include "chrome/browser/ui/views/page_action/test_support/page_action_test_support.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/prefs/pref_service.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "ui/base/page_transition_types.h"
#include "ui/events/base_event_utils.h"
#include "ui/events/event.h"
#include "ui/events/event_constants.h"
#include "ui/gfx/geometry/point.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/test/button_test_api.h"
#include "ui/views/test/widget_test.h"
#include "ui/views/view_utils.h"
#include "ui/views/widget/widget.h"
#include "url/gurl.h"

namespace page_actions {

class WaybackMachinePageActionBrowserTest : public InProcessBrowserTest {
 protected:
  BraveWaybackMachineTabHelper* GetTabHelper() {
    return BraveWaybackMachineTabHelper::FromWebContents(
        browser()->tab_strip_model()->GetActiveWebContents());
  }

  WaybackMachinePageActionController* GetController() {
    auto* tab = browser()->GetActiveTabInterface();
    if (!tab) {
      return nullptr;
    }
    auto* tab_features =
        tabs::BraveTabFeatures::FromTabFeatures(tab->GetTabFeatures());
    return tab_features ? tab_features->wayback_machine_page_action_controller()
                        : nullptr;
  }

  WaybackMachineBubbleView* GetBubbleView() {
    auto* controller = GetController();
    return controller ? controller->GetBubbleViewForTesting() : nullptr;
  }

  IconLabelBubbleView* GetIcon() {
    auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser());
    auto* button_provider = browser_view->toolbar_button_provider();
    return GetIconLabelBubbleViewForTesting(
        button_provider->GetPageActionViewInterface(kActionShowWaybackMachine),
        kActionShowWaybackMachine);
  }

  void ClickButton(views::Button* button) {
    views::test::ButtonTestApi(button).NotifyClick(
        ui::MouseEvent(ui::EventType::kMousePressed, gfx::Point(), gfx::Point(),
                       ui::EventTimeForNow(), ui::EF_LEFT_MOUSE_BUTTON,
                       ui::EF_LEFT_MOUSE_BUTTON));
  }

  void SetWaybackState(WaybackState state) {
    GetTabHelper()->SetWaybackStateForTesting(state);
  }
};

IN_PROC_BROWSER_TEST_F(WaybackMachinePageActionBrowserTest, BubbleLaunchTest) {
  auto* icon = GetIcon();
  EXPECT_FALSE(icon->GetVisible());

  SetWaybackState(WaybackState::kNeedToCheck);
  EXPECT_TRUE(icon->GetVisible());
  EXPECT_EQ(GetBubbleView(), nullptr);

  // Check bubble is launched.
  ClickButton(icon);
  EXPECT_NE(GetBubbleView(), nullptr);
}

IN_PROC_BROWSER_TEST_F(WaybackMachinePageActionBrowserTest,
                       DontAskAgainButtonTest) {
  auto* prefs = browser()->GetProfile()->GetPrefs();
  EXPECT_TRUE(prefs->GetBoolean(kBraveWaybackMachineEnabled));

  auto* icon = GetIcon();
  EXPECT_FALSE(icon->GetVisible());

  SetWaybackState(WaybackState::kNeedToCheck);
  EXPECT_TRUE(icon->GetVisible());

  ClickButton(icon);

  WaybackMachineBubbleView* bubble = GetBubbleView();
  ASSERT_NE(bubble, nullptr);

  auto* dont_ask_again =
      views::AsViewClass<views::MdTextButton>(bubble->GetExtraView());
  ASSERT_NE(dont_ask_again, nullptr);

  views::Widget* widget = bubble->GetWidget();
  ASSERT_NE(widget, nullptr);
  views::test::WidgetDestroyedWaiter waiter(widget);

  ClickButton(dont_ask_again);
  waiter.Wait();

  EXPECT_EQ(GetBubbleView(), nullptr);
  EXPECT_FALSE(prefs->GetBoolean(kBraveWaybackMachineEnabled));
}

class WaybackMachineAutoShowBubbleBrowserTest
    : public WaybackMachinePageActionBrowserTest {
 public:
  WaybackMachineAutoShowBubbleBrowserTest() {
    feature_list_.InitAndEnableFeature(
        brave_wayback_machine::features::kWaybackMachineAutoShowBubble);
  }

 protected:
  void CloseBubble() {
    WaybackMachineBubbleView* bubble = GetBubbleView();
    ASSERT_NE(bubble, nullptr);
    views::Widget* widget = bubble->GetWidget();
    ASSERT_NE(widget, nullptr);
    views::test::WidgetDestroyedWaiter waiter(widget);
    widget->CloseWithReason(views::Widget::ClosedReason::kCancelButtonClicked);
    waiter.Wait();
    EXPECT_EQ(GetBubbleView(), nullptr);
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(WaybackMachineAutoShowBubbleBrowserTest,
                       AutoShowsOnNeedToCheck) {
  EXPECT_EQ(GetBubbleView(), nullptr);

  SetWaybackState(WaybackState::kNeedToCheck);

  WaybackMachineBubbleView* bubble = GetBubbleView();
  ASSERT_NE(bubble, nullptr);
  ASSERT_NE(bubble->GetWidget(), nullptr);
  EXPECT_TRUE(bubble->GetWidget()->IsVisible());
}

IN_PROC_BROWSER_TEST_F(WaybackMachineAutoShowBubbleBrowserTest,
                       DoesNotReshowAfterDismiss) {
  SetWaybackState(WaybackState::kNeedToCheck);
  ASSERT_NE(GetBubbleView(), nullptr);

  CloseBubble();

  SetWaybackState(WaybackState::kNeedToCheck);
  EXPECT_EQ(GetBubbleView(), nullptr);

  SetWaybackState(WaybackState::kInitial);
  SetWaybackState(WaybackState::kNeedToCheck);
  EXPECT_NE(GetBubbleView(), nullptr);
}

IN_PROC_BROWSER_TEST_F(WaybackMachineAutoShowBubbleBrowserTest,
                       DoesNotAutoShowForInactiveTab) {
  content::WebContents* first_tab =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(AddTabAtIndex(1, GURL("about:blank"), ui::PAGE_TRANSITION_TYPED));
  ASSERT_EQ(1, browser()->tab_strip_model()->active_index());

  BraveWaybackMachineTabHelper::FromWebContents(first_tab)
      ->SetWaybackStateForTesting(WaybackState::kNeedToCheck);
  EXPECT_EQ(GetBubbleView(), nullptr);

  browser()->tab_strip_model()->ActivateTabAt(0);
  EXPECT_EQ(GetBubbleView(), nullptr);
}

}  // namespace page_actions
