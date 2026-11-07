// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <string>

#include "base/test/scoped_feature_list.h"
#include "brave/browser/ui/brave_ui_features.h"
#include "brave/components/brave_perf_predictor/common/pref_names.h"
#include "brave/components/brave_shields/core/common/pref_names.h"
#include "chrome/browser/profiles/profile.h"
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

constexpr char kRecapUrl[] = "chrome://recap/";

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

class BraveRecapUIBrowserTest : public InProcessBrowserTest {
 public:
  BraveRecapUIBrowserTest() {
    feature_list_.InitAndEnableFeature(features::kBraveRecap);
  }

 protected:
  content::WebContents* web_contents() {
    return chrome_test_utils::GetActiveWebContents(this);
  }

  void NavigateToRecap() {
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL(kRecapUrl)));
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

IN_PROC_BROWSER_TEST_F(BraveRecapUIBrowserTest, ShowsLifetimeStats) {
  PrefService* prefs = GetProfile()->GetPrefs();
  prefs->SetUint64(brave_shields::prefs::kAdsBlocked, 1000);
  prefs->SetUint64(brave_shields::prefs::kTrackersBlocked, 234);
  prefs->SetUint64(brave_perf_predictor::prefs::kBandwidthSavedBytes, 5678);

  NavigateToRecap();
  ASSERT_TRUE(web_contents()->GetWebUI());

  EXPECT_EQ("1234", GetLoadTimeString("adsBlocked"));
  EXPECT_EQ("5678", GetLoadTimeString("bandwidthSavedBytes"));
  EXPECT_EQ("Your Brave Recap", content::EvalJs(web_contents(), kGetHeading));
}

IN_PROC_BROWSER_TEST_F(BraveRecapUIBrowserTest, ShowsEmptyStateWithoutStats) {
  NavigateToRecap();
  EXPECT_EQ("0", GetLoadTimeString("adsBlocked"));
  EXPECT_EQ("Nothing to recap yet",
            content::EvalJs(web_contents(), kGetHeading));
}

class BraveRecapUIDisabledBrowserTest : public InProcessBrowserTest {
 public:
  BraveRecapUIDisabledBrowserTest() {
    feature_list_.InitAndDisableFeature(features::kBraveRecap);
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(BraveRecapUIDisabledBrowserTest, PageUnavailable) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL(kRecapUrl)));
  EXPECT_FALSE(chrome_test_utils::GetActiveWebContents(this)->GetWebUI());
}
