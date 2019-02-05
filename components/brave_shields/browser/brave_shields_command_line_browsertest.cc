/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "brave/components/brave_shields/common/brave_shield_switches.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "url/gurl.h"


class BraveShieldsCommandLineBrowserTest
    : public InProcessBrowserTest,
      public ::testing::WithParamInterface<bool> {
 public:
  void SetUpDefaultCommandLine(base::CommandLine* command_line) override {
    using namespace brave_shields::switches;
    InProcessBrowserTest::SetUpDefaultCommandLine(command_line);

    const char* state = GetParam() ? "allow" : "block";
    command_line->AppendSwitchASCII(kShieldsAdsSetDefault, state);
    command_line->AppendSwitchASCII(kShieldsHttpseSetDefault, state);
    command_line->AppendSwitchASCII(kShieldsNoScriptSetDefault, state);
    command_line->AppendSwitchASCII(kShieldsFingerprintingSetDefault, state);
    command_line->AppendSwitchASCII(kShieldsSetDefault, state);
    command_line->AppendSwitchASCII(kShieldsCookiePolicySetDefault, state);
  }
};

IN_PROC_BROWSER_TEST_P(BraveShieldsCommandLineBrowserTest, SmokeTest) {
  const bool ads = brave_shields::IsAllowContentSettingsForProfile(
      browser()->profile(), GURL("https://example.com"), GURL(),
      CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kBraveShields);
  const bool httpse = brave_shields::IsAllowContentSettingsForProfile(
      browser()->profile(), GURL("https://example.com"), GURL(),
      CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kBraveShields);
  const bool noscript = brave_shields::IsAllowContentSettingsForProfile(
      browser()->profile(), GURL("https://example.com"), GURL(),
      CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kBraveShields);
  const bool fingerprinting = brave_shields::IsAllowContentSettingsForProfile(
      browser()->profile(), GURL("https://example.com"), GURL(),
      CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kBraveShields);
  const bool shields = brave_shields::IsAllowContentSettingsForProfile(
      browser()->profile(), GURL("https://example.com"), GURL(),
      CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kBraveShields);
  const bool cookies = brave_shields::IsAllowContentSettingsForProfile(
      browser()->profile(), GURL("https://example.com"), GURL(),
      CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kBraveShields);

  EXPECT_EQ(GetParam(), ads);
  EXPECT_EQ(GetParam(), httpse);
  EXPECT_EQ(GetParam(), noscript);
  EXPECT_EQ(GetParam(), fingerprinting);
  EXPECT_EQ(GetParam(), shields);
  EXPECT_EQ(GetParam(), cookies);
}

INSTANTIATE_TEST_CASE_P(InstantiationName,
                        BraveShieldsCommandLineBrowserTest,
                        testing::Bool());
