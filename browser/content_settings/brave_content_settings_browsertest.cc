/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/strings/stringprintf.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/content_settings/core/browser/content_settings_registry.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "content/public/test/browser_test.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "services/network/public/cpp/network_switches.h"
#include "url/gurl.h"

class BraveContentSettingsBrowserTest : public InProcessBrowserTest {
 public:
  BraveContentSettingsBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
    https_server_.SetSSLConfig(net::EmbeddedTestServer::CERT_TEST_NAMES);
    https_server_.ServeFilesFromSourceDirectory(GetChromeTestDataDir());
    EXPECT_TRUE(https_server_.Start());
  }

  ~BraveContentSettingsBrowserTest() override = default;

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
    command_line->AppendSwitchASCII(
        network::switches::kHostResolverRules,
        base::StringPrintf("MAP *:443 127.0.0.1:%d", https_server_.port()));
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
  }

  static ContentSetting GetIncognitoAwareDefaultSetting(
      ContentSettingsType content_type,
      ContentSetting incognito_default_setting) {
    // NOTIFICATIONS is auto-blocked in incognito after a random timeout, it
    // requires special handling.
    if (content_type == ContentSettingsType::NOTIFICATIONS) {
      return CONTENT_SETTING_BLOCK;
    }
    return incognito_default_setting;
  }

 protected:
  net::test_server::EmbeddedTestServer https_server_;
};

IN_PROC_BROWSER_TEST_F(BraveContentSettingsBrowserTest,
                       ContentSettingsInheritanceInIncognito) {
  const GURL url("https://a.test/");

  Browser* incognito_browser = CreateIncognitoBrowser();
  auto* normal_host_content_settings =
      HostContentSettingsMapFactory::GetForProfile(browser()->profile());
  auto* incognito_host_content_settings =
      HostContentSettingsMapFactory::GetForProfile(
          incognito_browser->profile());
  ASSERT_NE(normal_host_content_settings, incognito_host_content_settings);

  for (const content_settings::ContentSettingsInfo* info :
       *content_settings::ContentSettingsRegistry::GetInstance()) {
    const ContentSettingsType type = info->website_settings_info()->type();
    SCOPED_TRACE(testing::Message()
                 << "ContentSettingsType=" << static_cast<int>(type));
    // Ignore unusual settings and settings that use CONTENT_SETTING_DEFAULT as
    // a default value (DCHECKs as invalid default value).
    if (!info->IsSettingValid(CONTENT_SETTING_BLOCK) ||
        info->website_settings_info()->initial_default_value().GetInt() == 0) {
      continue;
    }

    if (info->GetInitialDefaultSetting() == CONTENT_SETTING_BLOCK) {
      // Not interested in already blocked permissions.
      continue;
    }

    // Make sure defaults are equal in normal and incognito profiles.
    const ContentSetting normal_default_setting =
        normal_host_content_settings->GetDefaultContentSetting(type, nullptr);
    const ContentSetting incognito_default_setting =
        incognito_host_content_settings->GetDefaultContentSetting(type,
                                                                  nullptr);
    EXPECT_EQ(normal_default_setting, incognito_default_setting);
    // Make sure the current value in normal profile is default.
    EXPECT_EQ(normal_host_content_settings->GetContentSetting(url, url, type),
              normal_default_setting);
    // Make sure the current value in inconito profile is default.
    EXPECT_EQ(
        incognito_host_content_settings->GetContentSetting(url, url, type),
        incognito_default_setting);

    // Set non-default value in normal profile.
    normal_host_content_settings->SetContentSettingDefaultScope(
        url, url, type, CONTENT_SETTING_BLOCK);
    // Make sure the value is properly applied in normal profile.
    EXPECT_EQ(normal_host_content_settings->GetContentSetting(url, url, type),
              CONTENT_SETTING_BLOCK);

    // Check incognito value inheritance.
    const bool should_ignore_inheritance_for_privacy =
        info->incognito_behavior() ==
        content_settings::ContentSettingsInfo::INHERIT_IF_LESS_PERMISSIVE;
    EXPECT_EQ(
        incognito_host_content_settings->GetContentSetting(url, url, type),
        should_ignore_inheritance_for_privacy
            ? GetIncognitoAwareDefaultSetting(type, incognito_default_setting)
            : CONTENT_SETTING_BLOCK);

    incognito_host_content_settings->SetContentSettingDefaultScope(
        url, url, type, CONTENT_SETTING_BLOCK);
    // Make sure the value is properly applied in incognito profile.
    EXPECT_EQ(
        incognito_host_content_settings->GetContentSetting(url, url, type),
        CONTENT_SETTING_BLOCK);
  }

  const GURL url_to_navigate(url.Resolve("/empty.html"));
  EXPECT_TRUE(ui_test_utils::NavigateToURL(browser(), url_to_navigate));
  EXPECT_TRUE(ui_test_utils::NavigateToURL(incognito_browser, url_to_navigate));
}
