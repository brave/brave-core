/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/platform_browser_test.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "third_party/blink/public/common/features.h"

namespace {

constexpr char kReportingObserver[] = "/reporting_observer.html";

}  // namespace

class ReportingObserverTest : public PlatformBrowserTest {
 public:
  void SetUpOnMainThread() override {
    PlatformBrowserTest::SetUpOnMainThread();
    content::SetupCrossSiteRedirector(embedded_test_server());

    embedded_test_server()->ServeFilesFromDirectory(
        base::PathService::CheckedGet(brave::DIR_TEST_DATA));

    ASSERT_TRUE(embedded_test_server()->Start());
  }

 protected:
  content::WebContents* web_contents() {
    return chrome_test_utils::GetActiveWebContents(this);
  }
};

IN_PROC_BROWSER_TEST_F(ReportingObserverTest, IsNoop) {
  GURL url = embedded_test_server()->GetURL(kReportingObserver);
  ASSERT_TRUE(content::NavigateToURL(web_contents(), url));

  EXPECT_EQ(true, EvalJs(web_contents(), "isReportingObserverNoop();"));
}

class ReportingObserverEnabledTest : public ReportingObserverTest {
 public:
  ReportingObserverEnabledTest() {
    feature_list_.InitAndEnableFeature(blink::features::kBraveReportingAPI);
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(ReportingObserverEnabledTest, ReceivesReports) {
  GURL url = embedded_test_server()->GetURL(kReportingObserver);
  ASSERT_TRUE(content::NavigateToURL(web_contents(), url));

  EXPECT_EQ(false, EvalJs(web_contents(), "isReportingObserverNoop();"));
}
