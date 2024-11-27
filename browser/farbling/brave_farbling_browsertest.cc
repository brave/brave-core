// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/threading/thread_restrictions.h"
#include "brave/browser/brave_shields/brave_farbling_service_factory.h"
#include "brave/components/brave_shields/content/browser/brave_farbling_service.h"
#include "brave/components/brave_shields/content/browser/brave_shields_test_utils.h"
#include "brave/components/brave_shields/content/browser/brave_shields_util.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/webcompat/core/common/features.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"

namespace {

inline constexpr char kGetPluginsAsStringScript[] =
    "Array.from(navigator.plugins).map(p => p.name).join(', ');";
inline constexpr char kNavigatorPluginsFilename[] = "navigator_plugins.txt";

}  // namespace

class BraveFarblingBrowserTest : public InProcessBrowserTest {
 public:
  BraveFarblingBrowserTest() {
    scoped_feature_list_.InitAndEnableFeature(
        webcompat::features::kBraveWebcompatExceptionsService);
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    host_resolver()->AddRule("*", "127.0.0.1");
    content::SetupCrossSiteRedirector(embedded_test_server());

    base::FilePath test_data_dir =
        base::PathService::CheckedGet(brave::DIR_TEST_DATA);
    embedded_test_server()->ServeFilesFromDirectory(test_data_dir);

    ASSERT_TRUE(embedded_test_server()->Start());

    top_level_page_url_ = embedded_test_server()->GetURL("a.com", "/");
    farbling_url_ = embedded_test_server()->GetURL("a.com", "/simple.html");
  }

  const GURL& farbling_url() { return farbling_url_; }

  HostContentSettingsMap* content_settings() {
    return HostContentSettingsMapFactory::GetForProfile(browser()->profile());
  }

  content::WebContents* contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

 private:
  // By default farbling tokens are stable in tests. Passing 0 makes farbling
  // tokens be random even in tests.
  brave_shields::ScopedStableFarblingTokensForTesting
      scoped_random_farbling_tokens_{0};
  base::test::ScopedFeatureList scoped_feature_list_;
  GURL top_level_page_url_;
  GURL farbling_url_;
};

IN_PROC_BROWSER_TEST_F(BraveFarblingBrowserTest, NavigatorPluginsAreFarbled) {
  brave_shields::ScopedStableFarblingTokensForTesting
      scoped_stable_farbling_tokens{1};
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), farbling_url()));
  auto plugins_str = content::EvalJs(contents(), kGetPluginsAsStringScript);
  EXPECT_EQ(plugins_str,
            "Online PDF Viewer, HqVxgvf, 4cOuf2jw, Browser com.adobe.pdf ");
}

IN_PROC_BROWSER_TEST_F(BraveFarblingBrowserTest,
                       PRE_FarblingTokenIsKeptAfterRestart) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), farbling_url()));
  auto plugins_str = content::EvalJs(contents(), kGetPluginsAsStringScript);
  EXPECT_NE(plugins_str, "");
  // Write the current plugins list to a file in the profile directory.
  base::ScopedAllowBlockingForTesting allow_blocking;
  base::FilePath temp_dir = browser()->profile()->GetPath();
  base::FilePath output_file = temp_dir.AppendASCII(kNavigatorPluginsFilename);
  std::string result = plugins_str.ExtractString();
  base::WriteFile(output_file, result);
}

IN_PROC_BROWSER_TEST_F(BraveFarblingBrowserTest,
                       FarblingTokenIsKeptAfterRestart) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), farbling_url()));
  auto plugins_str = content::EvalJs(contents(), kGetPluginsAsStringScript);
  EXPECT_NE(plugins_str, "");
  // Read the plugins list from a file in the profile directory.
  base::ScopedAllowBlockingForTesting allow_blocking;
  base::FilePath temp_dir = browser()->profile()->GetPath();
  base::FilePath input_file = temp_dir.AppendASCII(kNavigatorPluginsFilename);
  std::string expected;
  EXPECT_TRUE(base::ReadFileToString(input_file, &expected));
  // Compare the plugins list from the previous launch.
  EXPECT_EQ(plugins_str, expected);
}

IN_PROC_BROWSER_TEST_F(BraveFarblingBrowserTest,
                       FarblingTokenIsClearedAfterWebsiteClear) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), farbling_url()));
  const std::string plugins_before_cleanup =
      content::EvalJs(contents(), kGetPluginsAsStringScript).ExtractString();

  // Ensure that the farbling token is stable while the website data is not
  // cleared.
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), farbling_url()));
  EXPECT_EQ(content::EvalJs(contents(), kGetPluginsAsStringScript),
            plugins_before_cleanup);

  // Clear the website data.
  content_settings()->ClearSettingsForOneType(
      ContentSettingsType::BRAVE_SHIELDS_METADATA);

  // A new token should be generated.
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), farbling_url()));
  EXPECT_NE(content::EvalJs(contents(), kGetPluginsAsStringScript),
            plugins_before_cleanup);
}

IN_PROC_BROWSER_TEST_F(BraveFarblingBrowserTest,
                       CheckBetweenNormalAndIncognitoProfile) {
  auto* profile1 = browser()->profile();
  auto* incognito_profile = CreateIncognitoBrowser(profile1)->profile();

  auto* brave_farbling_service =
      brave::BraveFarblingServiceFactory::GetForProfile(profile1);
  ASSERT_TRUE(brave_farbling_service);

  auto* brave_farbling_service_incognito =
      brave::BraveFarblingServiceFactory::GetForProfile(incognito_profile);
  ASSERT_TRUE(brave_farbling_service_incognito);

  // Compare the state of the PRNGs.
  brave::FarblingPRNG prng;
  brave::FarblingPRNG prng_incognito;
  EXPECT_TRUE(brave_farbling_service->MakePseudoRandomGeneratorForURL(
      farbling_url(), &prng));
  EXPECT_TRUE(brave_farbling_service_incognito->MakePseudoRandomGeneratorForURL(
      farbling_url(), &prng_incognito));
  EXPECT_NE(prng, prng_incognito);

  // Compare the farbling tokens.
  const auto farbling_token = brave_shields::GetFarblingToken(
      HostContentSettingsMapFactory::GetForProfile(profile1), farbling_url());
  const auto farbling_token_incognito = brave_shields::GetFarblingToken(
      HostContentSettingsMapFactory::GetForProfile(incognito_profile),
      farbling_url());
  EXPECT_FALSE(farbling_token.is_zero());
  EXPECT_FALSE(farbling_token_incognito.is_zero());
  EXPECT_NE(farbling_token, farbling_token_incognito);
}

IN_PROC_BROWSER_TEST_F(BraveFarblingBrowserTest, CheckBetweenTwoProfiles) {
  auto* profile_1 = browser()->profile();
  ASSERT_TRUE(profile_1);

  // Create another profile.
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  ASSERT_TRUE(profile_manager);
  base::FilePath dest_path = profile_manager->user_data_dir();
  dest_path = dest_path.Append(FILE_PATH_LITERAL("Profile2"));
  Profile* profile_2 = nullptr;
  {
    base::ScopedAllowBlockingForTesting allow_blocking;
    profile_2 = profile_manager->GetProfile(dest_path);
  }
  ASSERT_TRUE(profile_2);
  auto* browser_2 = CreateBrowser(profile_2);
  ASSERT_TRUE(browser_2);

  auto* brave_farbling_service_profile_1 =
      brave::BraveFarblingServiceFactory::GetForProfile(profile_1);
  ASSERT_TRUE(brave_farbling_service_profile_1);

  auto* brave_farbling_service_profile_2 =
      brave::BraveFarblingServiceFactory::GetForProfile(profile_2);
  ASSERT_TRUE(brave_farbling_service_profile_2);

  // Compare the state of the PRNGs.
  brave::FarblingPRNG prng_1;
  brave::FarblingPRNG prng_2;
  EXPECT_TRUE(brave_farbling_service_profile_1->MakePseudoRandomGeneratorForURL(
      farbling_url(), &prng_1));
  EXPECT_TRUE(brave_farbling_service_profile_2->MakePseudoRandomGeneratorForURL(
      farbling_url(), &prng_2));
  EXPECT_NE(prng_1, prng_2);

  // Compare the farbling tokens.
  const auto farbling_token_1 = brave_shields::GetFarblingToken(
      HostContentSettingsMapFactory::GetForProfile(profile_1), farbling_url());
  const auto farbling_token_2 = brave_shields::GetFarblingToken(
      HostContentSettingsMapFactory::GetForProfile(profile_2), farbling_url());
  EXPECT_FALSE(farbling_token_1.is_zero());
  EXPECT_FALSE(farbling_token_2.is_zero());
  EXPECT_NE(farbling_token_1, farbling_token_2);
}
