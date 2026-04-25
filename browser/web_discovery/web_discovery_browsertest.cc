/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/infobars/web_discovery_infobar_view.h"
#include "brave/browser/web_discovery/web_discovery_infobar_delegate.h"
#include "brave/browser/web_discovery/web_discovery_tab_helper.h"
#include "brave/components/constants/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/infobars/content/content_infobar_manager.h"
#include "components/infobars/core/infobar_manager.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "extensions/browser/background_script_executor.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/common/constants.h"
#include "extensions/test/extension_background_page_waiter.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace web_discovery {

using WebDiscoveryTest = InProcessBrowserTest;
using ::testing::_;

namespace {

class TestObserver : public infobars::InfoBarManager::Observer {
 public:
  TestObserver() = default;
  ~TestObserver() override = default;
  MOCK_METHOD(void, OnInfoBarAdded, (infobars::InfoBar * infobar), (override));
};

}  // namespace

IN_PROC_BROWSER_TEST_F(WebDiscoveryTest, InfobarAddedTest) {
  auto* model = browser()->tab_strip_model();
  auto* contents = model->GetActiveWebContents();
  auto* tab_helper = WebDiscoveryTabHelper::FromWebContents(contents);
  auto* infobar_manager =
      infobars::ContentInfoBarManager::FromWebContents(contents);

  TestObserver observer;
  EXPECT_CALL(observer, OnInfoBarAdded(_)).Times(1);
  infobar_manager->AddObserver(&observer);
  tab_helper->ShowInfoBar(browser()->profile()->GetPrefs());
  infobar_manager->RemoveObserver(&observer);

  // Verify WebDiscoveryInfoBarView.
  // WebDiscoveryInfoBarView::content_view_ should be direct child as
  // it occupies whole parent rect.
  auto infobar = std::make_unique<WebDiscoveryInfoBarView>(
      std::make_unique<WebDiscoveryInfoBarDelegate>(
          browser()->profile()->GetPrefs()));
  EXPECT_EQ(infobar.get(), infobar->content_view_->parent());
}

// Verifies that the WDP JS module loads without errors in the Brave Extension
// background page. This catches webpack bundling regressions.
IN_PROC_BROWSER_TEST_F(WebDiscoveryTest, ExtensionModuleLoads) {
  browser()->profile()->GetPrefs()->SetBoolean(kWebDiscoveryEnabled, true);

  // Wait for the extension to reload with its background page after the pref
  // change triggers BraveComponentLoader::UpdateBraveExtension().
  auto* registry = extensions::ExtensionRegistry::Get(browser()->profile());
  const auto* extension =
      registry->enabled_extensions().GetByID(brave_extension_id);
  ASSERT_TRUE(extension);
  extensions::ExtensionBackgroundPageWaiter(browser()->profile(), *extension)
      .WaitForBackgroundOpen();

  // setWDPStartedCallbackForTesting fires immediately if WDP is already
  // running, or once start() completes.
  constexpr char kScript[] = R"((async () => {
    await new Promise(resolve => {
      window.setWDPStartedCallbackForTesting(() => {
        chrome.test.sendScriptResult(true);
        resolve();
      });
    });
  })();)";

  base::Value result = extensions::BackgroundScriptExecutor::ExecuteScript(
      browser()->profile(), brave_extension_id, kScript,
      extensions::BackgroundScriptExecutor::ResultCapture::kSendScriptResult);
  EXPECT_EQ(true, result);
}

}  // namespace web_discovery
