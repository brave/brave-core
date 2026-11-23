/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ephemeral_storage/ephemeral_storage_tab_helper.h"

#include <memory>
#include <string>
#include <vector>

#include "brave/browser/ephemeral_storage/ephemeral_storage_service_factory.h"
#include "brave/browser/ephemeral_storage/tld_ephemeral_lifetime.h"
#include "chrome/browser/android/tab_android.h"
#include "chrome/browser/ui/android/tab_model/tab_model_observer.h"
#include "chrome/browser/ui/android/tab_model/tab_model_test_helper.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/browser/security_principal.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/navigation_simulator.h"
#include "content/public/test/test_renderer_host.h"
#include "content/public/test/web_contents_tester.h"
#include "net/base/url_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace ephemeral_storage {

namespace {

// Samples the TLDEphemeralLifetime state at TabRemoved time, which
// OwningTestTabModel fires after WillCloseTabs but while the tab and its
// WebContents are still alive.
class TabRemovalSampler : public TabModelObserver {
 public:
  explicit TabRemovalSampler(const TLDEphemeralLifetimeKey& key) : key_(key) {}

  void TabRemoved(TabAndroid* tab) override {
    sampled_ = true;
    web_contents_alive_ = tab && tab->web_contents();
    lifetime_alive_ = TLDEphemeralLifetime::Get(key_) != nullptr;
  }

  bool sampled() const { return sampled_; }
  bool web_contents_alive() const { return web_contents_alive_; }
  bool lifetime_alive() const { return lifetime_alive_; }

 private:
  TLDEphemeralLifetimeKey key_;
  bool sampled_ = false;
  bool web_contents_alive_ = false;
  bool lifetime_alive_ = false;
};

}  // namespace

class EphemeralStorageTabHelperAndroidTest : public testing::Test {
 protected:
  content::BrowserTaskEnvironment task_environment_;
  content::RenderViewHostTestEnabler rvh_test_enabler_;
  TestingProfile profile_;
};

// The tab helper must release its TLDEphemeralLifetime when the tab closes.
// Being WebContentsUserData, waiting for its own destructor is not enough on
// Android: the WebContents outlives the tab closure by an unbounded time.
TEST_F(EphemeralStorageTabHelperAndroidTest,
       TabCloseReleasesEphemeralLifetime) {
  auto* service = EphemeralStorageServiceFactory::GetForContext(&profile_);
  ASSERT_TRUE(service);

  OwningTestTabModel model(&profile_);
  // The tab must be in the model before the helper is created: the helper
  // picks the TabModel to observe in its constructor.
  TabAndroid* tab = model.AddTabFromWebContents(
      content::WebContentsTester::CreateTestWebContents(&profile_, nullptr), 0,
      /*select=*/true);
  ASSERT_TRUE(tab);
  content::WebContents* web_contents = tab->web_contents();
  EphemeralStorageTabHelper::CreateForWebContents(web_contents, service);

  const GURL url("https://a.com/");
  content::NavigationSimulator::NavigateAndCommitFromBrowser(web_contents, url);

  const TLDEphemeralLifetimeKey key{web_contents->GetBrowserContext(),
                                    net::URLToEphemeralStorageDomain(url),
                                    web_contents->GetSiteInstance()
                                        ->GetSecurityPrincipal()
                                        .GetStoragePartitionConfig()};
  ASSERT_TRUE(TLDEphemeralLifetime::Get(key));

  TabRemovalSampler sampler(key);
  model.AddObserver(&sampler);
  model.CloseTabAt(0);
  model.RemoveObserver(&sampler);

  ASSERT_TRUE(sampler.sampled());
  EXPECT_TRUE(sampler.web_contents_alive());
  EXPECT_FALSE(sampler.lifetime_alive());
}

}  // namespace ephemeral_storage
