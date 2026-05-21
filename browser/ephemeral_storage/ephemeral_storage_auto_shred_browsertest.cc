/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_shields/brave_shields_settings_service_factory.h"
#include "brave/browser/ephemeral_storage/ephemeral_storage_browsertest.h"
#include "brave/components/brave_shields/core/browser/brave_shields_settings_service.h"
#include "brave/components/brave_shields/core/browser/brave_shields_utils.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/content_settings/core/browser/cookie_settings.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "content/public/test/browser_test.h"
#include "net/base/features.h"

namespace ephemeral_storage {

class EphemeralStorageAutoShredBrowserTest
    : public EphemeralStorageBrowserTest {
 public:
  EphemeralStorageAutoShredBrowserTest() {
    scoped_feature_list_.InitWithFeatures(
        {brave_shields::features::kBraveShredFeature,
         net::features::kBraveForgetFirstPartyStorage},
        {});
  }
  ~EphemeralStorageAutoShredBrowserTest() override = default;

  void SetUpOnMainThread() override {
    EphemeralStorageBrowserTest::SetUpOnMainThread();

    auto* profile = browser()->profile();
    brave_shields_settings_ =
        BraveShieldsSettingsServiceFactory::GetForProfile(profile);
  }

  void TearDownOnMainThread() override {
    EphemeralStorageBrowserTest::TearDownOnMainThread();
    brave_shields_settings_ = nullptr;
  }

 protected:
  raw_ptr<brave_shields::BraveShieldsSettingsService> brave_shields_settings_ =
      nullptr;
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(EphemeralStorageAutoShredBrowserTest,
                       PRE_OnAppCloseShredAfterRestart) {
  const GURL a_site_set_cookie_url(
      "https://a.com/set-cookie?name=acom;path=/"
      ";SameSite=None;Secure;Max-Age=600");
  const GURL b_site_set_cookie_url(
      "https://b.com/set-cookie?name=bcom;path=/"
      ";SameSite=None;Secure;Max-Age=600");
  const GURL c_site_set_cookie_url(
      "https://c.com/set-cookie?name=ccom;path=/"
      ";SameSite=None;Secure;Max-Age=600");

  brave_shields_settings_->SetAutoShredMode(
      brave_shields::mojom::AutoShredMode::APP_EXIT, a_site_set_cookie_url);
  brave_shields_settings_->SetAutoShredMode(
      brave_shields::mojom::AutoShredMode::APP_EXIT, b_site_set_cookie_url);

  // Cookies should NOT exist for a.com, b.com, c.com.
  EXPECT_EQ(0u, GetAllCookies().size());

  EXPECT_TRUE(LoadURLInNewTab(a_site_set_cookie_url));
  EXPECT_TRUE(LoadURLInNewTab(b_site_set_cookie_url));
  EXPECT_TRUE(LoadURLInNewTab(c_site_set_cookie_url));

  // Cookies SHOULD exist for a.com.
  EXPECT_EQ(3u, GetAllCookies().size());
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageAutoShredBrowserTest,
                       OnAppCloseShredAfterRestart) {
  EXPECT_EQ(2u, WaitForCleanupAfterKeepAlive());
  EXPECT_EQ(1u, GetAllCookies().size());
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageAutoShredBrowserTest,
                       PRE_OnAppCloseShredGlobalAfterRestart) {
  const GURL a_site_set_cookie_url(
      "https://a.com/set-cookie?name=acom;path=/"
      ";SameSite=None;Secure;Max-Age=600");
  const GURL b_site_set_cookie_url(
      "https://b.com/set-cookie?name=bcom;path=/"
      ";SameSite=None;Secure;Max-Age=600");
  const GURL c_site_set_cookie_url(
      "https://c.com/set-cookie?name=ccom;path=/"
      ";SameSite=None;Secure;Max-Age=600");

  // Set a global auto-shred
  brave_shields_settings_->SetAutoShredMode(
      brave_shields::mojom::AutoShredMode::APP_EXIT, GURL());
  // Set b.com to be never shredded
  brave_shields_settings_->SetAutoShredMode(
      brave_shields::mojom::AutoShredMode::NEVER, b_site_set_cookie_url);

  // Cookies should NOT exist for a.com, b.com, c.com.
  EXPECT_EQ(0u, GetAllCookies().size());

  EXPECT_TRUE(LoadURLInNewTab(a_site_set_cookie_url));
  EXPECT_TRUE(LoadURLInNewTab(b_site_set_cookie_url));
  EXPECT_TRUE(LoadURLInNewTab(c_site_set_cookie_url));

  // Cookies SHOULD exist for a.com, b.com, c.com.
  EXPECT_EQ(3u, GetAllCookies().size());
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageAutoShredBrowserTest,
                       OnAppCloseShredGlobalAfterRestart) {
  EXPECT_EQ(2u, WaitForCleanupAfterKeepAlive());
  EXPECT_EQ(1u, GetAllCookies().size());
  // Make sure that only b.com has not been cleaned
  EXPECT_EQ("name=bcom",
            content::GetCookies(browser()->profile(),
                                https_server_.GetURL("b.com", "/")));
}

}  // namespace ephemeral_storage
