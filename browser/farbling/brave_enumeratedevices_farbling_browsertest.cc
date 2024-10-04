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
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "components/permissions/permission_request.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "url/gurl.h"

using brave_shields::ControlType;

constexpr char kEnumerateDevicesScript[] =
    "navigator.mediaDevices.enumerateDevices()"
    ".then(function(devices) {"
    "  var devicekinds = '';"
    "  devices.forEach(function(device) {"
    "    devicekinds = devicekinds + device.kind + '|';"
    "  });"
    "  return devicekinds;"
    "})";

class BraveEnumerateDevicesFarblingBrowserTest : public InProcessBrowserTest {
 public:
  BraveEnumerateDevicesFarblingBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {}

  BraveEnumerateDevicesFarblingBrowserTest(
      const BraveEnumerateDevicesFarblingBrowserTest&) = delete;
  BraveEnumerateDevicesFarblingBrowserTest& operator=(
      const BraveEnumerateDevicesFarblingBrowserTest&) = delete;

  ~BraveEnumerateDevicesFarblingBrowserTest() override = default;

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    https_server_.SetSSLConfig(net::EmbeddedTestServer::CERT_TEST_NAMES);
    https_server_.ServeFilesFromDirectory(test_data_dir);
    EXPECT_TRUE(https_server_.Start());
    top_level_page_url_ = https_server_.GetURL("b.test", "/");
    farbling_url_ = https_server_.GetURL("b.test", "/simple.html");
    host_resolver()->AddRule("*", "127.0.0.1");
  }

 protected:
  base::test::ScopedFeatureList feature_list_;
  net::EmbeddedTestServer https_server_;

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

  void EnableWebcompatException() {
    brave_shields::SetWebcompatEnabled(
        content_settings(),
        ContentSettingsType::BRAVE_WEBCOMPAT_HARDWARE_CONCURRENCY, true,
        top_level_page_url_, nullptr);
  }

 private:
  GURL top_level_page_url_;
  GURL farbling_url_;
};

// Tests results of farbling known values
IN_PROC_BROWSER_TEST_F(BraveEnumerateDevicesFarblingBrowserTest,
                       FarbleEnumerateDevices) {
  // Farbling level: off
  // get real navigator.mediaDevices.enumerateDevices array
  AllowFingerprinting();
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), farbling_url()));
  std::string real_value =
      content::EvalJs(contents(), kEnumerateDevicesScript).ExtractString();
  ASSERT_NE(real_value, "");

  // Farbling level: balanced (default)
  // navigator.mediaDevices.enumerateDevices array is shuffled
  // pseudo-randomly based on domain+session key
  SetFingerprintingDefault();
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), farbling_url()));
  std::string balanced_value =
      content::EvalJs(contents(), kEnumerateDevicesScript).ExtractString();
  EXPECT_NE(balanced_value, real_value);

  // Farbling level: maximum
  // same as farbling level: balanced
  BlockFingerprinting();
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), farbling_url()));
  std::string maximum_value =
      content::EvalJs(contents(), kEnumerateDevicesScript).ExtractString();
  EXPECT_EQ(balanced_value, maximum_value);

  // Farbling level: default, but with webcompat exception enabled
  // get real navigator.mediaDevices.enumerateDevices array
  SetFingerprintingDefault();
  EnableWebcompatException();
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), farbling_url()));
  std::string real_value2 =
      content::EvalJs(contents(), kEnumerateDevicesScript).ExtractString();
  ASSERT_NE(real_value2, "");
}
