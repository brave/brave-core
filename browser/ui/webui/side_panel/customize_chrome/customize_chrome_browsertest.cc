// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/task/current_thread.h"
#include "base/test/bind.h"
#include "base/values.h"
#include "chrome/browser/ui/actions/chrome_action_id.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_actions.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/customize_chrome/side_panel_controller.h"
#include "chrome/browser/ui/tabs/public/tab_features.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/side_panel/side_panel.h"
#include "chrome/browser/ui/views/side_panel/side_panel_web_ui_view.h"
#include "chrome/browser/ui/webui/side_panel/customize_chrome/customize_toolbar/customize_toolbar.mojom.h"
#include "chrome/common/webui_url_constants.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "ui/actions/actions.h"
#include "ui/views/controls/webview/webview.h"

class CustomizeChromeSidePanelBrowserTest : public InProcessBrowserTest {
 protected:
  // Returns the CustomizeChromeTabHelper associated with the tab
  customize_chrome::SidePanelController* GetSidePanelController(
      Browser* browser) {
    return browser->GetActiveTabInterface()
        ->GetTabFeatures()
        ->customize_chrome_side_panel_controller();
  }

  // Returns the WebContents of the side panel that is currently shown in the
  // BrowserView
  content::WebContents* GetCustomizeChromeWebContents() {
    auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser());
    if (!browser_view || !browser_view->side_panel()) {
      return nullptr;
    }
    auto* view = browser_view->side_panel()->GetViewByID(
        SidePanelWebUIView::kSidePanelWebViewId);
    if (!view) {
      return nullptr;
    }
    return static_cast<views::WebView*>(view)->web_contents();
  }

  // Opens the customize chrome side panel and returns its WebContents.
  content::WebContents* OpenCustomizeChromeSidePanel() {
    CHECK(ui_test_utils::NavigateToURL(browser(),
                                       GURL(chrome::kChromeUINewTabURL)));

    GetSidePanelController(browser())->OpenSidePanel(
        SidePanelOpenTrigger::kAppMenu, CustomizeChromeSection::kAppearance);

    content::WebContents* const web_contents = GetCustomizeChromeWebContents();
    CHECK(web_contents);
    content::WaitForLoadStop(web_contents);
    return web_contents;
  }

  // Calls the real CustomizeToolbarHandler already created by the page's own
  // JS (a second C++-side mojo::Remote can't be created: the handler only
  // allows one pipe per WebUI, see CustomizeChromeUI::
  // CreateCustomizeToolbarHandler()) and returns the ids/categories of the
  // actions it lists.
  base::ListValue ListActions(content::WebContents* web_contents) {
    constexpr char kScript[] = R"(
        (async () => {
          const {CustomizeToolbarApiProxy} = await import(
              'chrome://customize-chrome-side-panel.top-chrome/' +
              'customize_toolbar/customize_toolbar_api_proxy.js');
          const {actions} =
              await CustomizeToolbarApiProxy.getInstance().handler
                  .listActions();
          return actions.map(a => ({id: a.id, category: a.category}));
        })()
    )";
    const content::EvalJsResult result = content::EvalJs(web_contents, kScript);
    EXPECT_TRUE(result.is_ok()) << result.ExtractError();
    return result.ExtractList().Clone();
  }

  bool ContainsAction(
      const base::ListValue& actions,
      side_panel::customize_chrome::mojom::ActionId id,
      std::optional<side_panel::customize_chrome::mojom::CategoryId> category =
          std::nullopt) {
    return std::ranges::any_of(actions, [&](const base::Value& action) {
      const auto& dict = action.GetDict();
      if (dict.FindInt("id") != static_cast<int>(id)) {
        return false;
      }
      return !category.has_value() ||
             dict.FindInt("category") == static_cast<int>(*category);
    });
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

  // Clicking the close button should close the side panel. The panel closes
  // asynchronously (the close is animated), so the render frame is still alive
  // when the click script returns and ExecJs succeeds. The RunUntil() below is
  // the authoritative check that the panel actually closed.
  EXPECT_TRUE(content::ExecJs(web_contents,
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

// This covers disabled upstream's unit test:
// CustomizeToolbarHandlerTest.ListActions.
// Note that we use this browser test as the upstream unit test depend on banned
// BrowserWithTestWindowTest, which is practically a browser test.
IN_PROC_BROWSER_TEST_F(CustomizeChromeSidePanelBrowserTest,
                       ListActionsFiltersAndAddsBraveActions) {
  content::WebContents* const web_contents = OpenCustomizeChromeSidePanel();
  const base::ListValue actions = ListActions(web_contents);

  using ActionId = side_panel::customize_chrome::mojom::ActionId;
  using CategoryId = side_panel::customize_chrome::mojom::CategoryId;

  // Sanity check: unmodified Chromium actions are still listed.
  EXPECT_TRUE(ContainsAction(actions, ActionId::kHome));
  EXPECT_TRUE(ContainsAction(actions, ActionId::kForward));
  EXPECT_TRUE(ContainsAction(actions, ActionId::kDevTools));

  // Unsupported Chromium actions are filtered out.
  EXPECT_FALSE(ContainsAction(actions, ActionId::kShowPaymentMethods));
  EXPECT_FALSE(ContainsAction(actions, ActionId::kShowTranslate));
  EXPECT_FALSE(ContainsAction(actions, ActionId::kShowReadAnything));
  EXPECT_FALSE(ContainsAction(actions, ActionId::kShowAddresses));

  // LensOverlay is disabled in Brave, so its action is never created upstream
  // in the first place (this is the reason
  // CustomizeToolbarHandlerTest.ListActions/ActionsUpdatedOnVisibilityChange
  // are disabled in unit_tests.filter).
  EXPECT_FALSE(ContainsAction(actions, ActionId::kShowLensOverlay));

  // Brave-specific actions are added.
  EXPECT_TRUE(ContainsAction(actions, ActionId::kShowAddBookmarkButton,
                             CategoryId::kNavigation));
  EXPECT_TRUE(ContainsAction(actions, ActionId::kShowSidePanel,
                             CategoryId::kNavigation));
}

// This covers disabled upstream's unit test:
// CustomizeToolbarHandlerTest.ActionsUpdatedOnVisibilityChange.
// Note that we use this browser test as the upstream unit test depend on banned
// BrowserWithTestWindowTest, which is practically a browser test.
IN_PROC_BROWSER_TEST_F(CustomizeChromeSidePanelBrowserTest,
                       ActionsUpdatedOnVisibilityChange) {
  content::WebContents* const web_contents = OpenCustomizeChromeSidePanel();

  using ActionId = side_panel::customize_chrome::mojom::ActionId;

  // Devtools is initially present in the actions list.
  ASSERT_TRUE(ContainsAction(ListActions(web_contents), ActionId::kDevTools));

  // Listen for the WebUI client being notified that actions changed.
  ASSERT_TRUE(content::ExecJs(web_contents, R"(
      (async () => {
        const {CustomizeToolbarApiProxy} = await import(
            'chrome://customize-chrome-side-panel.top-chrome/' +
            'customize_toolbar/customize_toolbar_api_proxy.js');
        window.notified = false;
        CustomizeToolbarApiProxy.getInstance()
            .callbackRouter.notifyActionsUpdated.addListener(() => {
              window.notified = true;
            });
      })()
  )"));

  // Set visibility of devtools to false, and...
  actions::ActionItem* const scope_action =
      browser()->browser_actions()->root_action_item();
  actions::ActionItem* const devtools_action_item =
      actions::ActionManager::Get().FindAction(kActionDevTools, scope_action);
  ASSERT_TRUE(devtools_action_item);

  // The WebUI client is notified, and...
  devtools_action_item->SetVisible(false);
  ASSERT_TRUE(base::test::RunUntil([&]() {
    return content::EvalJs(web_contents, "window.notified === true")
        .ExtractBool();
  }));

  // Devtools is now absent from the actions list.
  EXPECT_FALSE(ContainsAction(ListActions(web_contents), ActionId::kDevTools));
}
