// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/task/current_thread.h"
#include "base/test/bind.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/customize_chrome/side_panel_controller.h"
#include "chrome/browser/ui/tabs/public/tab_features.h"
#include "chrome/browser/ui/views/side_panel/side_panel_ui.h"
#include "chrome/common/webui_url_constants.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"

class CustomizeChromeSidePanelBrowserTest : public InProcessBrowserTest {
 protected:
  // Returns the CustomizeChromeTabHelper associated with the tab
  customize_chrome::SidePanelController* GetSidePanelController(
      Browser* browser) {
    return browser->GetActiveTabInterface()
        ->GetTabFeatures()
        ->customize_chrome_side_panel_controller();
  }

  SidePanelUI* GetSidePanelUI(Browser* browser) {
    return browser->browser_window_features()->side_panel_ui();
  }

  content::WebContents* GetCustomizeChromeWebContents() {
    return GetSidePanelUI(browser())->GetWebContentsForTest(
        SidePanelEntryId::kCustomizeChrome);
  }
};

IN_PROC_BROWSER_TEST_F(CustomizeChromeSidePanelBrowserTest, CloseButton) {
  // Given that customize chrome side panel is available,
  auto* customize_chrome_side_panel_controller =
      GetSidePanelController(browser());
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(),
                                           GURL(chrome::kChromeUINewTabURL)));
  ASSERT_TRUE(customize_chrome_side_panel_controller
                  ->IsCustomizeChromeEntryAvailable());

  // When the side panel is opened,
  customize_chrome_side_panel_controller->OpenSidePanel(
      SidePanelOpenTrigger::kAppMenu, CustomizeChromeSection::kAppearance);
  ASSERT_TRUE(
      customize_chrome_side_panel_controller->IsCustomizeChromeEntryShowing());
  content::WebContents* web_contents = GetCustomizeChromeWebContents();
  ASSERT_TRUE(web_contents);
  content::WaitForLoadStop(web_contents);

  // clicking the close button should close the side panel.
  // And the render frame will be deleted, so the returned result will be false.
  EXPECT_FALSE(content::ExecJs(web_contents,
                               R"-js-(
      document.querySelector("body > customize-chrome-app")
          .shadowRoot.querySelector("#closeButton")
          .shadowRoot.querySelector("#closeButton").click();)-js-"));

  // Double check that the side panel is closed.
  ASSERT_TRUE(base::test::RunUntil([&]() {
    return !customize_chrome_side_panel_controller
                ->IsCustomizeChromeEntryShowing();
  }));
}
