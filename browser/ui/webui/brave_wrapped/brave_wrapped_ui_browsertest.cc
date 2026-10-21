// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <string>

#include "base/test/scoped_feature_list.h"
#include "brave/browser/ui/brave_ui_features.h"
#include "brave/components/brave_perf_predictor/common/pref_names.h"
#include "brave/components/constants/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/incognito_allowed_url.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace {

constexpr char kWrappedUrl[] = "chrome://wrapped/";

constexpr char kGetHeading[] = R"(
  new Promise((resolve) => {
    const heading = document.querySelector('h1')
    if (heading) {
      resolve(heading.textContent)
      return
    }
    const observer = new MutationObserver(() => {
      const heading = document.querySelector('h1')
      if (heading) {
        observer.disconnect()
        resolve(heading.textContent)
      }
    })
    observer.observe(document.body, {childList: true, subtree: true})
  })
)";

}  // namespace

class BraveWrappedUIBrowserTest : public InProcessBrowserTest {
 public:
  BraveWrappedUIBrowserTest() {
    feature_list_.InitAndEnableFeature(features::kBraveWrapped);
  }

 protected:
  content::WebContents* web_contents() {
    return chrome_test_utils::GetActiveWebContents(this);
  }

  void NavigateToWrapped() {
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL(kWrappedUrl)));
    ASSERT_TRUE(content::WaitForLoadStop(web_contents()));
  }

  std::string GetLoadTimeString(const std::string& key) {
    return content::EvalJs(web_contents(),
                           "loadTimeData.getString('" + key + "')")
        .ExtractString();
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(BraveWrappedUIBrowserTest, ShowsLifetimeStats) {
  PrefService* prefs = browser()->GetProfile()->GetPrefs();
  prefs->SetUint64(kAdsBlocked, 1000);
  prefs->SetUint64(kTrackersBlocked, 234);
  prefs->SetUint64(brave_perf_predictor::prefs::kBandwidthSavedBytes, 5678);

  NavigateToWrapped();
  ASSERT_TRUE(web_contents()->GetWebUI());

  EXPECT_EQ("1234", GetLoadTimeString("adsBlocked"));
  EXPECT_EQ("5678", GetLoadTimeString("bandwidthSavedBytes"));
  EXPECT_EQ("Your Brave, wrapped",
            content::EvalJs(web_contents(), kGetHeading));
}

IN_PROC_BROWSER_TEST_F(BraveWrappedUIBrowserTest, ShowsEmptyStateWithoutStats) {
  NavigateToWrapped();
  EXPECT_EQ("0", GetLoadTimeString("adsBlocked"));
  EXPECT_EQ("Still unwrapping", content::EvalJs(web_contents(), kGetHeading));
}

IN_PROC_BROWSER_TEST_F(BraveWrappedUIBrowserTest, NotAllowedInPrivateWindow) {
  EXPECT_FALSE(IsURLAllowedInIncognito(GURL(kWrappedUrl)));
}

class BraveWrappedUIDisabledBrowserTest : public InProcessBrowserTest {
 public:
  BraveWrappedUIDisabledBrowserTest() {
    feature_list_.InitAndDisableFeature(features::kBraveWrapped);
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(BraveWrappedUIDisabledBrowserTest, PageUnavailable) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL(kWrappedUrl)));
  EXPECT_FALSE(chrome_test_utils::GetActiveWebContents(this)->GetWebUI());
}
