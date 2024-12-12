// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/path_service.h"
#include "base/test/scoped_feature_list.h"
#include "base/time/time.h"
#include "brave/browser/ui/webui/brave_settings_ui.h"
#include "brave/browser/ui/webui/settings/brave_extensions_manifest_v2_handler.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "extensions/browser/disable_reason.h"
#include "extensions/browser/extension_system.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_builder.h"

namespace {

constexpr char kExtensionId[] = "doojmbjmlfjjnbmnoijecmcbfeoakpjm";

bool ClickExtensionToggle(content::WebContents* web_contents) {
  return EvalJs(web_contents,
                "window.testing.extensionsV2Subpage.getElementById('"
                "doojmbjmlfjjnbmnoijecmcbfeoakpjm').click()")
      .value.is_none();
}

bool ClickExtensionRemove(content::WebContents* web_contents) {
  return EvalJs(web_contents,
                "window.testing.extensionsV2Subpage.getElementById('"
                "doojmbjmlfjjnbmnoijecmcbfeoakpjm').querySelector('#"
                "doojmbjmlfjjnbmnoijecmcbfeoakpjm').click()")
      .value.is_none();
}

bool IsExtensionToggled(content::WebContents* web_contents) {
  return EvalJs(web_contents,
                "window.testing.extensionsV2Subpage.getElementById('"
                "doojmbjmlfjjnbmnoijecmcbfeoakpjm').checked")
      .value.GetBool();
}

bool IsExtensionToggleEnabled(content::WebContents* web_contents) {
  return EvalJs(web_contents,
                "!window.testing.extensionsV2Subpage.getElementById('"
                "doojmbjmlfjjnbmnoijecmcbfeoakpjm')."
                "disabled")
      .value.GetBool();
}

void NonBlockingDelay(base::TimeDelta delay) {
  base::RunLoop run_loop(base::RunLoop::Type::kNestableTasksAllowed);
  base::SingleThreadTaskRunner::GetCurrentDefault()->PostDelayedTask(
      FROM_HERE, run_loop.QuitWhenIdleClosure(), delay);
  run_loop.Run();
}

}  // namespace

class BraveExtensionsManifestV2BrowserTest : public InProcessBrowserTest {
 public:
  BraveExtensionsManifestV2BrowserTest() {
    feature_list_.InitAndEnableFeature(kExtensionsManifestV2);
    BraveSettingsUI::ShouldExposeElementsForTesting() = true;
  }

  ~BraveExtensionsManifestV2BrowserTest() override {
    BraveSettingsUI::ShouldExposeElementsForTesting() = false;
  }

  void SimulateInstall() {
    scoped_refptr<const extensions::Extension> extension(
        extensions::ExtensionBuilder("extension").SetID(kExtensionId).Build());
    extensions::ExtensionSystem::Get(browser()->profile())
        ->extension_service()
        ->AddExtension(extension.get());
  }

  void EnableExtension(bool enable) {
    if (enable) {
      extensions::ExtensionSystem::Get(browser()->profile())
          ->extension_service()
          ->EnableExtension(kExtensionId);
    } else {
      extensions::ExtensionSystem::Get(browser()->profile())
          ->extension_service()
          ->DisableExtension(kExtensionId,
                             extensions::disable_reason::DISABLE_USER_ACTION);
    }
  }

  bool IsExtensionEnabled() {
    return extensions::ExtensionRegistry::Get(browser()->profile())
        ->enabled_extensions()
        .Contains(kExtensionId);
  }

  bool IsExtensionInstalled() {
    return extensions::ExtensionRegistry::Get(browser()->profile())
        ->GetInstalledExtension(kExtensionId);
  }

  void WaitExtensionToggled(bool toggled) {
    while (IsExtensionToggled(
               browser()->tab_strip_model()->GetActiveWebContents()) !=
           toggled) {
      NonBlockingDelay(base::Milliseconds(10));
    }
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(BraveExtensionsManifestV2BrowserTest, InstallFail) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), GURL("brave://settings/extensions/v2")));
  auto* web_contents = browser()->tab_strip_model()->GetActiveWebContents();
  content::WebContentsConsoleObserver console_observer(web_contents);
  console_observer.SetPattern("Could not fetch data from the Chrome Web Store");
  ClickExtensionToggle(web_contents);
  ASSERT_TRUE(console_observer.Wait());

  while (IsExtensionToggled(web_contents)) {
    NonBlockingDelay(base::Milliseconds(10));
  }
  EXPECT_TRUE(IsExtensionToggleEnabled(web_contents));
}

IN_PROC_BROWSER_TEST_F(BraveExtensionsManifestV2BrowserTest,
                       InstallDisableEnableUninstall) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), GURL("brave://settings/extensions/v2")));
  auto* web_contents = browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_FALSE(IsExtensionToggled(web_contents));
  EXPECT_TRUE(IsExtensionToggleEnabled(web_contents));

  SimulateInstall();
  WaitExtensionToggled(true);
  {
    // toggle extension disabled->enabled->disabled
    EnableExtension(false);
    WaitExtensionToggled(false);

    EXPECT_TRUE(IsExtensionInstalled());

    EnableExtension(true);
    WaitExtensionToggled(true);
    EXPECT_TRUE(IsExtensionInstalled());

    EnableExtension(false);
    WaitExtensionToggled(false);
  }

  // enable from settings.
  ClickExtensionToggle(web_contents);
  WaitExtensionToggled(true);
  EXPECT_TRUE(IsExtensionInstalled());
  EXPECT_TRUE(IsExtensionEnabled());

  // disabled from settings.
  ClickExtensionToggle(web_contents);
  WaitExtensionToggled(false);
  EXPECT_TRUE(IsExtensionInstalled());
  EXPECT_FALSE(IsExtensionEnabled());

  // remove from settings.
  ClickExtensionRemove(web_contents);
  EXPECT_FALSE(IsExtensionInstalled());
  EXPECT_FALSE(IsExtensionEnabled());
}
