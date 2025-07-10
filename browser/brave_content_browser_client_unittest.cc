/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_content_browser_client.h"

#include "base/memory/raw_ptr.h"
#include "base/values.h"
#include "brave/components/brave_search/common/brave_search_utils.h"
#include "brave/components/skus/common/skus_utils.h"
#include "build/build_config.h"
#include "chrome/browser/chrome_content_browser_client.h"
#include "chrome/test/base/scoped_testing_local_state.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/test/browser_task_environment.h"
#include "extensions/buildflags/buildflags.h"
#include "testing/gtest/include/gtest/gtest.h"

#if BUILDFLAG(ENABLE_EXTENSIONS) || BUILDFLAG(IS_ANDROID)
#include "extensions/common/constants.h"
#endif

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "extensions/browser/extension_registry.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_builder.h"
#endif

#if BUILDFLAG(IS_WIN)
#include "base/test/scoped_os_info_override_win.h"
#include "brave/components/windows_recall/windows_recall.h"
#endif

class BraveContentBrowserClientTest : public testing::Test {
 protected:
  content::BrowserTaskEnvironment task_environment_;
  TestingProfile profile_;
};

TEST_F(BraveContentBrowserClientTest, ResolvesSync) {
  GURL url("chrome://sync/");
  ASSERT_TRUE(
      BraveContentBrowserClient::HandleURLOverrideRewrite(&url, nullptr));
  ASSERT_STREQ(url.spec().c_str(), "chrome://settings/braveSync");

  GURL url2("chrome://sync/");
  ASSERT_TRUE(
      BraveContentBrowserClient::HandleURLOverrideRewrite(&url2, nullptr));
}

TEST_F(BraveContentBrowserClientTest, ResolvesWelcomePage) {
  GURL url("chrome://welcome/");
  ASSERT_TRUE(
      BraveContentBrowserClient::HandleURLOverrideRewrite(&url, nullptr));
}

TEST_F(BraveContentBrowserClientTest, IsolatedWebAppsAreDisabled) {
  BraveContentBrowserClient client;
  EXPECT_FALSE(client.AreIsolatedWebAppsEnabled(&profile_));
}

TEST_F(BraveContentBrowserClientTest, GetOriginsRequiringDedicatedProcess) {
  ChromeContentBrowserClient chrome_client;
  BraveContentBrowserClient client;
  auto chrome_origins = chrome_client.GetOriginsRequiringDedicatedProcess();
  auto brave_origins = client.GetOriginsRequiringDedicatedProcess();

  ASSERT_TRUE(std::all_of(
      brave_origins.begin(), brave_origins.end(),
      [chrome_origins](auto& origin) {
        return std::any_of(chrome_origins.begin(), chrome_origins.end(),
                           [origin](auto& other) {
                             return other.IsSameOriginWith(origin);
                           }) ||
               skus::IsSafeOrigin(origin.GetURL()) ||
               brave_search::IsAllowedHost(origin.GetURL());
      }));
}

TEST_F(BraveContentBrowserClientTest, IsWindowsRecallDisabled) {
  BraveContentBrowserClient client;
  ScopedTestingLocalState testing_local_state(
      TestingBrowserProcess::GetGlobal());
#if BUILDFLAG(IS_WIN)
  base::test::ScopedOSInfoOverride win_version(
      base::test::ScopedOSInfoOverride::Type::kWin11Home);
  // Pref is registered.
  EXPECT_TRUE(testing_local_state.Get()->FindPreference(
      windows_recall::prefs::kWindowsRecallDisabled));
  // Disabled by default on Win11 or newer.
  EXPECT_TRUE(client.IsWindowsRecallDisabled());
#else
  EXPECT_FALSE(client.IsWindowsRecallDisabled());
#endif
}
