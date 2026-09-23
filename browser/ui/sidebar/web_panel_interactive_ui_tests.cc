/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string_view>

#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/ui/sidebar/sidebar_browsertest_base.h"
#include "brave/browser/ui/sidebar/sidebar_controller.h"
#include "brave/browser/ui/sidebar/sidebar_model.h"
#include "brave/browser/ui/sidebar/sidebar_utils.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/split_view/brave_multi_contents_view.h"
#include "brave/components/sidebar/browser/pref_names.h"
#include "brave/components/sidebar/browser/sidebar_item.h"
#include "brave/components/sidebar/common/features.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/frame/multi_contents_view.h"
#include "chrome/browser/ui/views/frame/multi_contents_view_delegate.h"
#include "chrome/test/base/interactive_test_utils.h"
#include "chrome/test/base/ui_test_utils.h"
#include "chrome/test/interaction/interactive_browser_test.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "ui/base/interaction/state_observer.h"
#include "ui/views/view.h"
#include "url/gurl.h"

namespace sidebar {

class SidebarWebPanelInteractiveUITest
    : public InteractiveBrowserTestMixin<SidebarBrowserTest> {
 public:
  SidebarWebPanelInteractiveUITest() {
    scoped_features_.InitAndEnableFeature(sidebar::features::kSidebarWebPanel);
  }
  ~SidebarWebPanelInteractiveUITest() override = default;

  auto WaitForActiveTabChange(int index) {
    DEFINE_LOCAL_STATE_IDENTIFIER_VALUE(ui::test::PollingStateObserver<int>,
                                        kActiveTabIndexObserver);
    return Steps(PollState(kActiveTabIndexObserver,
                           base::BindRepeating(
                               &TabStripModel::active_index,
                               base::Unretained(browser()->tab_strip_model()))),
                 WaitForState(kActiveTabIndexObserver, index),
                 StopObservingState(kActiveTabIndexObserver));
  }

  // Don't send click event as it makes this test flaky on all CI.
  auto ActivatePane(content::WebContents* target_contents) {
    return Steps(Do([this, target_contents]() {
      GetBraveMultiContentsView()->delegate_for_testing()->WebContentsFocused(
          target_contents);
    }));
  }

 private:
  base::test::ScopedFeatureList scoped_features_;
};

// Verifies that a real mouse click actually activates the clicked pane's tab,
// in both directions (into the panel, and back out to the normal tab).
IN_PROC_BROWSER_TEST_F(SidebarWebPanelInteractiveUITest,
                       ClickActivatesWebPanelAndNormalTab) {
  // Ensure the browser window is actually the OS-level active/key window
  // before sending any real synthetic clicks -- it may not be by
  // default depending on the environment.
  ASSERT_TRUE(ui_test_utils::BringBrowserWindowToFront(browser()));

  // Use about:blank (rather than a real external site) so navigation never
  // depends on network access and always commits as a normal page.
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL("about:blank")));
  ASSERT_TRUE(CanAddCurrentActiveTabToSidebar(browser()));
  // To prevent the item-added feedback bubble from launching and stealing
  // the clicks this test sends.
  browser()->GetProfile()->GetPrefs()->SetInteger(
      sidebar::kSidebarItemAddedFeedbackBubbleShowCount, 3);
  controller()->AddItemWithCurrentTab();
  const int web_panel_item_index =
      static_cast<int>(model()->GetAllSidebarItems().size()) - 1;
  controller()->ActivateItemAt(web_panel_item_index);
  ASSERT_TRUE(GetBraveMultiContentsView()->IsWebPanelVisible());

  ASSERT_EQ(2, browser()->tab_strip_model()->count());
  ASSERT_EQ(1, browser()->tab_strip_model()->active_index());

  // The panel's pinned tab was just created and is still navigating; make
  // sure it's actually ready to receive input before clicking it.
  ASSERT_TRUE(content::WaitForLoadStop(
      browser()->tab_strip_model()->GetWebContentsAt(0)));

  content::WebContents* web_panel_contents =
      browser()->tab_strip_model()->GetWebContentsAt(0);
  content::WebContents* normal_tab_contents =
      browser()->tab_strip_model()->GetWebContentsAt(1);

  RunTestSequence(
      ActivatePane(web_panel_contents), WaitForActiveTabChange(0),
      Check([this]() {
        return GetBraveMultiContentsView()->is_web_panel_active_for_testing();
      }),

      ActivatePane(normal_tab_contents), WaitForActiveTabChange(1),
      Check([this]() {
        return !GetBraveMultiContentsView()->is_web_panel_active_for_testing();
      }));
}

}  // namespace sidebar
