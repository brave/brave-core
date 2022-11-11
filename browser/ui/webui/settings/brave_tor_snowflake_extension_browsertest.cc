// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "base/path_service.h"
#include "brave/browser/ui/webui/brave_settings_ui.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "extensions/browser/disable_reason.h"
#include "extensions/browser/extension_system.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_builder.h"

namespace {

constexpr const char kSnowflakeExtensionId[] =
    "mafpmfcccpbjnhfhjnllmmalhifmlcie";

bool ClickSnowflakeToggle(content::WebContents* web_contents) {
  return EvalJs(
             web_contents,
             "window.testing.torSubpage.getElementById('torSnowflake').click()")
      .value.is_none();
}

bool IsSnowflakeToggled(content::WebContents* web_contents) {
  return EvalJs(
             web_contents,
             "window.testing.torSubpage.getElementById('torSnowflake').checked")
      .value.GetBool();
}

}  // namespace

class TorSnowflakeExtensionBrowserTest : public InProcessBrowserTest {
 public:
  TorSnowflakeExtensionBrowserTest() {
    // Disabling CSP on webui pages so EvalJS could be run in main world.
    BraveSettingsUI::ShouldDisableCSPForTesting() = true;
    BraveSettingsUI::ShouldExposeElementsForTesting() = true;
  }

  ~TorSnowflakeExtensionBrowserTest() override {
    BraveSettingsUI::ShouldDisableCSPForTesting() = false;
    BraveSettingsUI::ShouldExposeElementsForTesting() = false;
  }

  void SimulateSnowflakeInstall() {
    scoped_refptr<const extensions::Extension> extension(
        extensions::ExtensionBuilder("Snowflake")
            .SetID(kSnowflakeExtensionId)
            .Build());
    extensions::ExtensionSystem::Get(browser()->profile())
        ->extension_service()
        ->AddExtension(extension.get());
  }

  void EnableSnowflake(bool enable) {
    if (enable) {
      extensions::ExtensionSystem::Get(browser()->profile())
          ->extension_service()
          ->EnableExtension(kSnowflakeExtensionId);
    } else {
      extensions::ExtensionSystem::Get(browser()->profile())
          ->extension_service()
          ->DisableExtension(kSnowflakeExtensionId,
                             extensions::disable_reason::DISABLE_USER_ACTION);
    }
  }

  bool IsSnowflakeInstalled() {
    return extensions::ExtensionRegistry::Get(browser()->profile())
        ->GetInstalledExtension(kSnowflakeExtensionId);
  }
};

IN_PROC_BROWSER_TEST_F(TorSnowflakeExtensionBrowserTest, InstallFail) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(),
                                           GURL("brave://settings/privacy")));
  auto* web_contents = browser()->tab_strip_model()->GetActiveWebContents();
  content::WebContentsConsoleObserver console_observer(web_contents);
  console_observer.SetPattern("Could not fetch data from the Chrome Web Store");
  ClickSnowflakeToggle(web_contents);
  console_observer.Wait();
  EXPECT_FALSE(IsSnowflakeToggled(web_contents));
}

IN_PROC_BROWSER_TEST_F(TorSnowflakeExtensionBrowserTest,
                       InstallDisableEnableUninstall) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(),
                                           GURL("brave://settings/privacy")));
  auto* web_contents = browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_FALSE(IsSnowflakeToggled(web_contents));

  SimulateSnowflakeInstall();
  EXPECT_TRUE(IsSnowflakeToggled(web_contents));
  {
    // toggle extension disabled->enabled->disabled
    EnableSnowflake(false);
    EXPECT_FALSE(IsSnowflakeToggled(web_contents));
    EXPECT_TRUE(IsSnowflakeInstalled());

    EnableSnowflake(true);
    EXPECT_TRUE(IsSnowflakeToggled(web_contents));
    EXPECT_TRUE(IsSnowflakeInstalled());

    EnableSnowflake(false);
  }

  // enable from settings.
  ClickSnowflakeToggle(web_contents);
  EXPECT_TRUE(IsSnowflakeToggled(web_contents));
  EXPECT_TRUE(IsSnowflakeInstalled());

  // disabled from settings -> uninstalled.
  ClickSnowflakeToggle(web_contents);
  EXPECT_FALSE(IsSnowflakeToggled(web_contents));
  EXPECT_FALSE(IsSnowflakeInstalled());
}
