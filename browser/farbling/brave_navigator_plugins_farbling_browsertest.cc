/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "base/strings/stringprintf.h"
#include "base/task/post_task.h"
#include "base/test/thread_test_helper.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/brave_content_browser_client.h"
#include "brave/browser/extensions/brave_base_local_data_files_browsertest.h"
#include "brave/common/brave_paths.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_component_updater/browser/local_data_files_service.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/chrome_content_client.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"

using brave_shields::ControlType;

const char kPluginsLengthScript[] =
    "domAutomationController.send(navigator.plugins.length);";

class BraveNavigatorPluginsFarblingBrowserTest : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    content_client_.reset(new ChromeContentClient);
    content::SetContentClient(content_client_.get());
    browser_content_client_.reset(new BraveContentBrowserClient());
    content::SetBrowserClientForTesting(browser_content_client_.get());

    host_resolver()->AddRule("*", "127.0.0.1");
    content::SetupCrossSiteRedirector(embedded_test_server());

    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    embedded_test_server()->ServeFilesFromDirectory(test_data_dir);

    ASSERT_TRUE(embedded_test_server()->Start());

    top_level_page_url_ = embedded_test_server()->GetURL("a.com", "/");
    farbling_url_ = embedded_test_server()->GetURL("a.com", "/simple.html");
  }

  void TearDown() override {
    browser_content_client_.reset();
    content_client_.reset();
  }

  const GURL& farbling_url() { return farbling_url_; }

  HostContentSettingsMap* content_settings() {
    return HostContentSettingsMapFactory::GetForProfile(browser()->profile());
  }

  void AllowFingerprinting() {
    brave_shields::SetFingerprintingControlType(
        content_settings(), ControlType::ALLOW, top_level_page_url_);
  }

  void BlockFingerprinting() {
    brave_shields::SetFingerprintingControlType(
        content_settings(), ControlType::BLOCK, top_level_page_url_);
  }

  void SetFingerprintingDefault() {
    brave_shields::SetFingerprintingControlType(
        content_settings(), ControlType::DEFAULT, top_level_page_url_);
  }

  template <typename T>
  int ExecScriptGetInt(const std::string& script, T* frame) {
    int value;
    EXPECT_TRUE(ExecuteScriptAndExtractInt(frame, script, &value));
    return value;
  }

  template <typename T>
  std::string ExecScriptGetStr(const std::string& script, T* frame) {
    std::string value;
    EXPECT_TRUE(ExecuteScriptAndExtractString(frame, script, &value));
    return value;
  }

  content::WebContents* contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  bool NavigateToURLUntilLoadStop(const GURL& url) {
    ui_test_utils::NavigateToURL(browser(), url);
    return WaitForLoadStop(contents());
  }

 private:
  GURL top_level_page_url_;
  GURL farbling_url_;
  std::unique_ptr<ChromeContentClient> content_client_;
  std::unique_ptr<BraveContentBrowserClient> browser_content_client_;
};

// Tests results of farbling known values
// https://github.com/brave/brave-browser/issues/9435
IN_PROC_BROWSER_TEST_F(BraveNavigatorPluginsFarblingBrowserTest,
                       FarbleNavigatorPlugins) {
  // Farbling level: off
  // get real length of navigator.plugins, which should return an empty array
  // from Chromium 90 on (see https://crrev.com/c/2629990 for reference).
  AllowFingerprinting();
  NavigateToURLUntilLoadStop(farbling_url());
  int real_plugins_length = ExecScriptGetInt(kPluginsLengthScript, contents());
  EXPECT_EQ(real_plugins_length, 0);

  // Farbling level: balanced (default) navigator.plugins should contain 2 fake
  // ones (we used to report 2 real plugins and 2 fake ones before Chromium 90,
  // but there are no real ones now, see https://crrev.com/c/2629990).
  SetFingerprintingDefault();
  NavigateToURLUntilLoadStop(farbling_url());
  int balanced_length = ExecScriptGetInt(kPluginsLengthScript, contents());
  EXPECT_EQ(balanced_length, 2);

  // Farbling level: maximum
  // navigator.plugins should contain no real plugins either, only 2 fake ones.
  BlockFingerprinting();
  NavigateToURLUntilLoadStop(farbling_url());
  int maximum_length = ExecScriptGetInt(kPluginsLengthScript, contents());
  EXPECT_EQ(maximum_length, 2);
  EXPECT_EQ(ExecScriptGetStr(
                "domAutomationController.send(navigator.plugins[0].name);",
                contents()),
            "Xr1at27");
  EXPECT_EQ(ExecScriptGetStr(
                "domAutomationController.send(navigator.plugins[0].filename);",
                contents()),
            "SJEChw48ev3bNGD");
  EXPECT_EQ(
      ExecScriptGetStr(
          "domAutomationController.send(navigator.plugins[0].description);",
          contents()),
      "rVqVqVqVqVKlSpUqVqVKlSJEChQIECh");
  EXPECT_EQ(ExecScriptGetInt(
                "domAutomationController.send(navigator.plugins[0].length);",
                contents()),
            1);
  EXPECT_EQ(ExecScriptGetStr(
                "domAutomationController.send(navigator.plugins[0][0].type);",
                contents()),
            "");
  EXPECT_EQ(
      ExecScriptGetStr(
          "domAutomationController.send(navigator.plugins[0][0].description);",
          contents()),
      "HDBAgQo0aNGDBgw48.fvXrVKFiRIkyZM");
  EXPECT_EQ(ExecScriptGetStr(
                "domAutomationController.send(navigator.plugins[1].name);",
                contents()),
            "8.fPHDhw");
  EXPECT_EQ(ExecScriptGetStr(
                "domAutomationController.send(navigator.plugins[1].filename);",
                contents()),
            "06du37du3bt2bNmT");
  EXPECT_EQ(
      ExecScriptGetStr(
          "domAutomationController.send(navigator.plugins[1].description);",
          contents()),
      "BgwYMmTpUq1aNmTJky5cOnTp069ePnTp");
  EXPECT_EQ(ExecScriptGetInt(
                "domAutomationController.send(navigator.plugins[1].length);",
                contents()),
            1);
  EXPECT_EQ(ExecScriptGetStr(
                "domAutomationController.send(navigator.plugins[1][0].type);",
                contents()),
            "");
  EXPECT_EQ(
      ExecScriptGetStr(
          "domAutomationController.send(navigator.plugins[1][0].description);",
          contents()),
      "qVKly58ePHDBgQoUqVKFix48.fvXLlSJ");
}

// Tests that no built-in plugins returned even when fingerprinting is allowed,
// and that only fake ones are exposed when "balanced" behavior is selected.
IN_PROC_BROWSER_TEST_F(BraveNavigatorPluginsFarblingBrowserTest,
                       FarbleNavigatorPluginsNoBuiltinAvailable) {
  // Farbling level: off
  AllowFingerprinting();
  NavigateToURLUntilLoadStop(farbling_url());
  int real_plugins_length = ExecScriptGetInt(kPluginsLengthScript, contents());
  EXPECT_EQ(real_plugins_length, 0);

  // Farbling level: balanced (default)
  SetFingerprintingDefault();
  NavigateToURLUntilLoadStop(farbling_url());
  EXPECT_EQ(ExecScriptGetStr(
                "domAutomationController.send(navigator.plugins[0].name);",
                contents()),
            "Xr1at27");
  EXPECT_EQ(ExecScriptGetStr(
                "domAutomationController.send(navigator.plugins[1].name);",
                contents()),
            "8.fPHDhw");
}

// Tests that the list of plugins returned by navigator.plugins will reset to
// being and empty one when fingerprinting is turned off.
IN_PROC_BROWSER_TEST_F(BraveNavigatorPluginsFarblingBrowserTest,
                       FarbleNavigatorPluginsReset) {
  // Farbling level: balanced (default)
  SetFingerprintingDefault();
  NavigateToURLUntilLoadStop(farbling_url());
  EXPECT_EQ(ExecScriptGetStr(
                "domAutomationController.send(navigator.plugins[0].name);",
                contents()),
            "Xr1at27");
  EXPECT_EQ(ExecScriptGetStr(
                "domAutomationController.send(navigator.plugins[1].name);",
                contents()),
            "8.fPHDhw");

  // Farbling level: off
  AllowFingerprinting();
  NavigateToURLUntilLoadStop(farbling_url());
  int real_plugins_length = ExecScriptGetInt(kPluginsLengthScript, contents());
  EXPECT_EQ(real_plugins_length, 0);
}
