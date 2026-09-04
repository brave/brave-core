/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_shields/brave_shields_settings_service_factory.h"
#include "brave/browser/ephemeral_storage/ephemeral_storage_browsertest.h"
#include "brave/components/brave_shields/core/browser/brave_shields_settings_service.h"
#include "brave/components/brave_shields/core/browser/brave_shields_utils.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/ephemeral_storage/ephemeral_storage_pref_names.h"
#include "brave/components/ephemeral_storage/ephemeral_storage_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/content_settings/core/browser/cookie_settings.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/test_utils.h"
#include "net/base/features.h"

namespace ephemeral_storage {

namespace {

size_t GetOriginsQueuedForCleanupCount(Profile* profile) {
  return profile->GetPrefs()
      ->GetList(kFirstPartyStorageOriginsToCleanup)
      .size();
}

}  // namespace

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

    auto* profile = browser()->GetProfile();
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

  // Simulate that the tabs were closed more than 30 seconds ago
  ExpireFirstPartyStorageOrigins(true);
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageAutoShredBrowserTest,
                       OnAppCloseShredAfterRestart) {
  // Nothing to clean — everything was already cleared when the browser started
  EXPECT_EQ(0u, WaitForCleanupAfterKeepAlive());
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

  // Simulate that the tabs were closed more than 30 seconds ago
  ExpireFirstPartyStorageOrigins(true);
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageAutoShredBrowserTest,
                       OnAppCloseShredGlobalAfterRestart) {
  EXPECT_EQ(0u, WaitForCleanupAfterKeepAlive());
  EXPECT_EQ(1u, GetAllCookies().size());
  // Make sure that only b.com has not been cleaned
  EXPECT_EQ("name=bcom",
            content::GetCookies(browser()->GetProfile(),
                                https_server_.GetURL("b.com", "/")));
}

// Closing a tab queues its shred behind the Ephemeral Storage keepalive. If the
// browser quits before that keepalive elapses, the queued shred must survive
// the restart: it either runs at the next start or stays queued until it is
// due. Dropping it would leave the site's storage on disk forever.
IN_PROC_BROWSER_TEST_F(EphemeralStorageAutoShredBrowserTest,
                       PRE_PRE_ShredQueuedWithinKeepAliveSurvivesRestart) {
  const GURL a_site_set_cookie_url(
      "https://a.com/set-cookie?name=acom;path=/"
      ";SameSite=None;Secure;Max-Age=600");

  brave_shields_settings_->SetAutoShredMode(
      brave_shields::mojom::AutoShredMode::LAST_TAB_CLOSED,
      a_site_set_cookie_url);

  // Cookies should NOT exist for a.com.
  EXPECT_EQ(0u, GetAllCookies().size());

  content::WebContents* a_tab = LoadURLInNewTab(a_site_set_cookie_url);
  ASSERT_TRUE(a_tab);

  // Cookies SHOULD exist for a.com.
  EXPECT_EQ(1u, GetAllCookies().size());

  // Close the a.com tab. The shred is deferred by the keepalive, so nothing is
  // cleaned yet, but the area is queued in prefs with the current close time.
  CloseWebContents(a_tab);
  content::RunAllTasksUntilIdle();

  EXPECT_EQ(1u, GetOriginsQueuedForCleanupCount(browser()->GetProfile()));
  EXPECT_EQ(1u, GetAllCookies().size());

  // The browser quits here, well within the keepalive, so the deferred cleanup
  // timer never fires.
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageAutoShredBrowserTest,
                       PRE_ShredQueuedWithinKeepAliveSurvivesRestart) {
  // Let the startup cleanup run. The close time is only seconds old, so the
  // keepalive has not elapsed yet.
  content::RunAllTasksUntilIdle();

  const bool still_queued =
      GetOriginsQueuedForCleanupCount(browser()->GetProfile()) == 1u;
  const bool already_shredded = GetAllCookies().empty();
  EXPECT_TRUE(still_queued || already_shredded)
      << "the a.com shred queued before the restart was dropped without being "
         "performed";

  // Simulate the keepalive elapsing, so the next start must shred a.com.
  ExpireFirstPartyStorageOrigins(false);
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageAutoShredBrowserTest,
                       ShredQueuedWithinKeepAliveSurvivesRestart) {
  // The queued shred is now due and must have happened on this start.
  content::RunAllTasksUntilIdle();
  WaitForCleanupAfterKeepAlive();
  EXPECT_EQ(0u, GetAllCookies().size());
}

}  // namespace ephemeral_storage
