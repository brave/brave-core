/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string_view>

#include "base/feature_list.h"
#include "base/path_service.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/components/brave_shields/content/browser/brave_shields_util.h"
#include "brave/components/brave_shields/core/common/brave_shield_constants.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/webcompat/core/common/features.h"
#include "brave/components/webcompat/content/browser/webcompat_exceptions_service.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/google/core/common/google_switches.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/default_handlers.h"
#include "url/origin.h"

#if BUILDFLAG(IS_ANDROID)
#include "chrome/test/base/android/android_browser_test.h"
#else
#include "chrome/browser/ui/browser_navigator_params.h"
#include "chrome/test/base/in_process_browser_test.h"
#endif

using brave_shields::ControlType;
using content_settings::mojom::ContentSettingsType;
using enum ContentSettingsType;

namespace {

struct TestCase {
  const char* name;
  const ContentSettingsType type;
};

constexpr TestCase kTestCases[] = {
    {"all-fingerprinting", BRAVE_FINGERPRINTING_V2},
    {"audio", BRAVE_WEBCOMPAT_AUDIO},
    {"canvas", BRAVE_WEBCOMPAT_CANVAS},
    {"device-memory", BRAVE_WEBCOMPAT_DEVICE_MEMORY},
    {"eventsource-pool", BRAVE_WEBCOMPAT_EVENT_SOURCE_POOL},
    {"font", BRAVE_WEBCOMPAT_FONT},
    {"hardware-concurrency", BRAVE_WEBCOMPAT_HARDWARE_CONCURRENCY},
    {"keyboard", BRAVE_WEBCOMPAT_KEYBOARD},
    {"language", BRAVE_WEBCOMPAT_LANGUAGE},
    {"media-devices", BRAVE_WEBCOMPAT_MEDIA_DEVICES},
    {"plugins", BRAVE_WEBCOMPAT_PLUGINS},
    {"screen", BRAVE_WEBCOMPAT_SCREEN},
    {"speech-synthesis", BRAVE_WEBCOMPAT_SPEECH_SYNTHESIS},
    {"usb-device-serial-number", BRAVE_WEBCOMPAT_USB_DEVICE_SERIAL_NUMBER},
    {"user-agent", BRAVE_WEBCOMPAT_USER_AGENT},
    {"webgl", BRAVE_WEBCOMPAT_WEBGL},
    {"webgl2", BRAVE_WEBCOMPAT_WEBGL2},
    {"websockets-pool", BRAVE_WEBCOMPAT_WEB_SOCKETS_POOL},
};

}  // namespace

class WebcompatExceptionsBrowserTest : public PlatformBrowserTest {
 public:
  WebcompatExceptionsBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
    feature_list_.InitAndEnableFeature(
        webcompat::features::kBraveWebcompatExceptionsService);
  }

  void SetUpOnMainThread() override {
    PlatformBrowserTest::SetUpOnMainThread();

    host_resolver()->AddRule("*", "127.0.0.1");

    base::FilePath test_data_dir =
        base::PathService::CheckedGet(brave::DIR_TEST_DATA);
    https_server_.SetSSLConfig(net::EmbeddedTestServer::CERT_TEST_NAMES);
    https_server_.ServeFilesFromDirectory(test_data_dir);
    https_server_.AddDefaultHandlers(GetTestDataDir());

    ASSERT_TRUE(https_server_.Start());

    url_ = https_server_.GetURL("a.test", "/simple.html");
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    PlatformBrowserTest::SetUpCommandLine(command_line);
    // Since the HTTPS server only serves a valid cert for localhost,
    // this is needed to load pages from "www.google.*" without an interstitial.
    command_line->AppendSwitch(switches::kIgnoreCertificateErrors);

    // The production code only allows known ports (80 for http and 443 for
    // https), but the test server runs on a random port.
    command_line->AppendSwitch(switches::kIgnoreGooglePortNumbers);
  }

  base::FilePath GetTestDataDir() {
    return base::FilePath(FILE_PATH_LITERAL("net/data/url_request_unittest"));
  }

  const net::EmbeddedTestServer& https_server() { return https_server_; }
  const GURL& url() { return url_; }

  content::WebContents* GetActiveWebContents() {
    return chrome_test_utils::GetActiveWebContents(this);
  }

  HostContentSettingsMap* content_settings() {
    return HostContentSettingsMapFactory::GetForProfile(
        chrome_test_utils::GetProfile(this));
  }

  void NavigateToURL(const std::string& origin, const std::string& path) {
    ASSERT_TRUE(content::NavigateToURL(GetActiveWebContents(),
                                       https_server().GetURL(origin, path)));
  }

 protected:
  base::test::ScopedFeatureList feature_list_;

 private:
  GURL url_;
  net::test_server::EmbeddedTestServer https_server_;
};

IN_PROC_BROWSER_TEST_F(WebcompatExceptionsBrowserTest, RemoteSettingsTest) {
  NavigateToURL("a.test", "/simple.html");
  const auto pattern = ContentSettingsPattern::FromString("*://a.test/*");
  auto* webcompat_exceptions_service =
      webcompat::WebcompatExceptionsService::CreateInstance(
          g_brave_browser_process->local_data_files_service());
  auto* map = content_settings();
  for (const auto& test_case : kTestCases) {
    // Check the default setting
    const auto observed_setting_default =
        map->GetContentSetting(GURL("https://a.test"), GURL(), test_case.type);
    EXPECT_NE(observed_setting_default, CONTENT_SETTING_ALLOW);

    // Create a rule and then reload the page.
    webcompat::PatternsByWebcompatTypeMap rule_map;
    rule_map[test_case.type] = std::vector<ContentSettingsPattern>({pattern});
    webcompat_exceptions_service->SetRulesForTesting(rule_map);
    NavigateToURL("a.test", "/simple.html");

    // Check the remote setting gets used
    const auto observed_setting_remote =
        map->GetContentSetting(GURL("https://a.test"), GURL(), test_case.type);
    EXPECT_EQ(observed_setting_remote, CONTENT_SETTING_ALLOW);

    // Check that the remote setting doesn't leak to another domain
    const auto observed_setting_cross_site =
        map->GetContentSetting(GURL("https://b.test"), GURL(), test_case.type);
    EXPECT_NE(observed_setting_cross_site, CONTENT_SETTING_ALLOW);

    // Check that manual setting can override the remote setting
    brave_shields::SetWebcompatEnabled(map, test_case.type, false,
                                       GURL("https://a.test"), nullptr);
    const auto observed_setting_override1 =
        map->GetContentSetting(GURL("https://a.test"), GURL(), test_case.type);
    EXPECT_EQ(observed_setting_override1, CONTENT_SETTING_BLOCK);

    // Check that manual setting can override the remote setting
    brave_shields::SetWebcompatEnabled(map, test_case.type, true,
                                       GURL("https://b.test"), nullptr);
    const auto observed_setting_override2 =
        map->GetContentSetting(GURL("https://b.test"), GURL(), test_case.type);
    EXPECT_EQ(observed_setting_override2, CONTENT_SETTING_ALLOW);

    // Check that webcompat returns false for non-http URLs.
    bool result = brave_shields::IsWebcompatEnabled(map, test_case.type,
                                                    GURL("file://tmp"));
    EXPECT_FALSE(result);

    // Check that the webcompat setting has been enabled as expected.
    bool result2 = brave_shields::IsWebcompatEnabled(map, test_case.type,
                                                     GURL("https://b.test"));
    EXPECT_TRUE(result2);
  }
}
