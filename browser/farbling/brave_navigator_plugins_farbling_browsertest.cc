/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/path_service.h"
#include "base/strings/stringprintf.h"
#include "base/test/thread_test_helper.h"
#include "brave/browser/extensions/brave_base_local_data_files_browsertest.h"
#include "brave/components/brave_component_updater/browser/local_data_files_service.h"
#include "brave/components/brave_shields/content/browser/brave_shields_util.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/webcompat/core/common/features.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"

using brave_shields::ControlType;

namespace {

constexpr char kPluginsLengthScript[] = "navigator.plugins.length;";
constexpr char kNavigatorPdfViewerEnabledCrashTest[] =
    "navigator.pdfViewerEnabled == navigator.pdfViewerEnabled";

}  // namespace

class BraveNavigatorPluginsFarblingBrowserTest : public InProcessBrowserTest {
 public:
  BraveNavigatorPluginsFarblingBrowserTest() {
    scoped_feature_list_.InitAndEnableFeature(
        webcompat::features::kBraveWebcompatExceptionsService);
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    host_resolver()->AddRule("*", "127.0.0.1");
    content::SetupCrossSiteRedirector(embedded_test_server());

    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    embedded_test_server()->ServeFilesFromDirectory(test_data_dir);

    ASSERT_TRUE(embedded_test_server()->Start());

    top_level_page_url_ = embedded_test_server()->GetURL("a.com", "/");
    farbling_url_ = embedded_test_server()->GetURL("a.com", "/simple.html");
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

  content::WebContents* contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

 private:
  GURL top_level_page_url_;
  GURL farbling_url_;
  base::test::ScopedFeatureList scoped_feature_list_;
};

// Tests that access to navigator.pdfViewerEnabled attribute does not crash.
IN_PROC_BROWSER_TEST_F(BraveNavigatorPluginsFarblingBrowserTest,
                       NavigatorPdfViewerEnabledNoCrash) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), farbling_url()));
  EXPECT_EQ(true, EvalJs(contents(), kNavigatorPdfViewerEnabledCrashTest));
}

// Tests results of farbling known values
// https://github.com/brave/brave-browser/issues/9435
IN_PROC_BROWSER_TEST_F(BraveNavigatorPluginsFarblingBrowserTest,
                       FarbleNavigatorPlugins) {
  // Farbling level: off
  // get real length of navigator.plugins
  AllowFingerprinting();
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), farbling_url()));
  int off_length =
      content::EvalJs(contents(), kPluginsLengthScript).ExtractInt();

  // Farbling level: balanced (default)
  // navigator.plugins should contain all real plugins + 2 fake ones
  SetFingerprintingDefault();
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), farbling_url()));
  int balanced_length =
      content::EvalJs(contents(), kPluginsLengthScript).ExtractInt();
  EXPECT_EQ(balanced_length, off_length + 2);

  // Farbling level: maximum
  // navigator.plugins should contain no real plugins, only 2 fake ones
  BlockFingerprinting();
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), farbling_url()));
  int maximum_length =
      content::EvalJs(contents(), kPluginsLengthScript).ExtractInt();
  EXPECT_EQ(maximum_length, 2);
  EXPECT_EQ(content::EvalJs(contents(), "navigator.plugins[0].name;"),
            "8mTJjRv2");
  EXPECT_EQ(content::EvalJs(contents(), "navigator.plugins[0].filename;"),
            "0iZUpzhYrVxgvf2b");
  EXPECT_EQ(content::EvalJs(contents(), "navigator.plugins[0].description;"),
            "z8eu2Eh36GLs9mTRIMtWyZrdOuf2bNl5");
  EXPECT_EQ(content::EvalJs(contents(), "navigator.plugins[0].length;"), 1);
  EXPECT_EQ(content::EvalJs(contents(), "navigator.plugins[0][0].type;"), "");
  EXPECT_EQ(content::EvalJs(contents(), "navigator.plugins[0][0].description;"),
            "6pc1iZMOHDBny4cOuf2j4FCgYrVpzhYz");
  EXPECT_EQ(content::EvalJs(contents(), "navigator.plugins[1].name;"),
            "JjZUxgv");
  EXPECT_EQ(content::EvalJs(contents(), "navigator.plugins[1].filename;"),
            "2nyCJECgYrVp7GD");
  EXPECT_EQ(content::EvalJs(contents(), "navigator.plugins[1].description;"),
            "nb0Do7GLs9mb0DgYzCJMteXq8HiwYUx");
  EXPECT_EQ(content::EvalJs(contents(), "navigator.plugins[1].length;"), 1);
  EXPECT_EQ(content::EvalJs(contents(), "navigator.plugins[1][0].type;"), "");
  EXPECT_EQ(content::EvalJs(contents(), "navigator.plugins[1][0].description;"),
            "pzhQIECgYzCBny4cOuXLFh3Epc1aseXq");

  // Farbling level: default, but webcompat exception enabled
  // get real length of navigator.plugins
  SetFingerprintingDefault();
  brave_shields::SetWebcompatEnabled(
      content_settings(), ContentSettingsType::BRAVE_WEBCOMPAT_PLUGINS, true,
      farbling_url(), nullptr);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), farbling_url()));
  int off_length2 =
      content::EvalJs(contents(), kPluginsLengthScript).ExtractInt();
  EXPECT_EQ(off_length, off_length2);
}

// Tests that names of built-in plugins get farbled by default
// https://github.com/brave/brave-browser/issues/10597
IN_PROC_BROWSER_TEST_F(BraveNavigatorPluginsFarblingBrowserTest,
                       FarbleNavigatorPluginsBuiltin) {
  // Farbling level: off
  AllowFingerprinting();
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), farbling_url()));
  int off_length =
      content::EvalJs(contents(), kPluginsLengthScript).ExtractInt();
  EXPECT_EQ(off_length, 2);
  EXPECT_EQ(content::EvalJs(contents(), "navigator.plugins[0].name;"),
            "Chrome PDF Plugin");
  EXPECT_EQ(content::EvalJs(contents(), "navigator.plugins[1].name;"),
            "Chrome PDF Viewer");

  // Farbling level: balanced (default)
  SetFingerprintingDefault();
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), farbling_url()));
  EXPECT_EQ(content::EvalJs(contents(), "navigator.plugins[0].name;"),
            "OpenSource doc Renderer");
  EXPECT_EQ(content::EvalJs(contents(), "navigator.plugins[3].name;"),
            "Chrome doc Viewer");
}

// Tests that names of built-in plugins that get farbled will reset to their
// original names when fingerprinting is turned off
// https://github.com/brave/brave-browser/issues/11278
IN_PROC_BROWSER_TEST_F(BraveNavigatorPluginsFarblingBrowserTest,
                       FarbleNavigatorPluginsReset) {
  // Farbling level: balanced (default)
  SetFingerprintingDefault();
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), farbling_url()));
  EXPECT_EQ(content::EvalJs(contents(), "navigator.plugins[0].name;"),
            "OpenSource doc Renderer");
  EXPECT_EQ(content::EvalJs(contents(), "navigator.plugins[3].name;"),
            "Chrome doc Viewer");

  // Farbling level: off
  AllowFingerprinting();
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), farbling_url()));
  int off_length =
      content::EvalJs(contents(), kPluginsLengthScript).ExtractInt();
  EXPECT_EQ(off_length, 2);
  EXPECT_EQ(content::EvalJs(contents(), "navigator.plugins[0].name;"),
            "Chrome PDF Plugin");
  EXPECT_EQ(content::EvalJs(contents(), "navigator.plugins[1].name;"),
            "Chrome PDF Viewer");
}
