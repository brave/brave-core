/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>
#include <tuple>

#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/psst/psst_log_service_factory.h"
#include "brave/components/psst/core/browser/psst_log_service.h"
#include "brave/components/psst/core/common/constants.h"
#include "brave/components/psst/core/common/features.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/url_constants.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace psst {

namespace {

// Resolves with the row count once the rendered table holds exactly $1 rows.
// Driven by the page's own mutations rather than polled from C++, following
// the MutationObserver-promise pattern used by
// chrome/browser/dom_distiller/dom_distiller_viewer_source_browsertest.cc.
constexpr char kWaitForRowCountScript[] = R"(
    new Promise((resolve) => {
      const expected = $1;
      const count = () => document.querySelectorAll('tbody tr').length;
      if (count() === expected) {
        resolve(count());
        return;
      }
      const observer = new MutationObserver(() => {
        if (count() === expected) {
          observer.disconnect();
          resolve(count());
        }
      });
      observer.observe(document.getElementById('root'),
                       { childList: true, subtree: true });
    })
  )";

GURL InternalsURL() {
  return GURL(
      base::StrCat({content::kChromeUIScheme, "://", kPsstInternalsHost}));
}

}  // namespace

class PsstInternalsUIBrowserTest : public InProcessBrowserTest {
 public:
  PsstInternalsUIBrowserTest() {
    feature_list_.InitAndEnableFeature(features::kEnablePsst);
  }
  ~PsstInternalsUIBrowserTest() override = default;

 protected:
  content::WebContents* web_contents() {
    return browser()->GetActiveTabInterface()->GetContents();
  }

  PsstLogService* log_service() {
    return PsstLogServiceFactory::GetForProfile(GetProfile());
  }

  void AddEntries(size_t count) {
    for (size_t i = 0; i < count; ++i) {
      log_service()->AddNativeEntry(
          mojom::PsstLogLevel::kInfo, /*tab_id=*/1,
          GURL("https://example.com/"),
          base::StrCat({"entry ", base::NumberToString(i)}));
    }
  }

  // Returns the rendered row count, so a mismatch reports both numbers.
  int RowCountWhenSettled(size_t expected) {
    const std::string script = base::ReplaceStringPlaceholders(
        kWaitForRowCountScript, {base::NumberToString(expected)}, nullptr);
    return content::EvalJs(web_contents(), script).ExtractInt();
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(PsstInternalsUIBrowserTest, RendersBufferedEntries) {
  ASSERT_TRUE(log_service());
  AddEntries(3);

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), InternalsURL()));
  EXPECT_EQ(InternalsURL(), web_contents()->GetLastCommittedURL());
  EXPECT_EQ(3, RowCountWhenSettled(3));
}

// Entries added while the page is open are pushed to it.
IN_PROC_BROWSER_TEST_F(PsstInternalsUIBrowserTest, PushesNewEntries) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), InternalsURL()));
  ASSERT_EQ(0, RowCountWhenSettled(0));

  AddEntries(2);
  EXPECT_EQ(2, RowCountWhenSettled(2));
}

IN_PROC_BROWSER_TEST_F(PsstInternalsUIBrowserTest, ClearEmptiesThePage) {
  AddEntries(1);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), InternalsURL()));
  ASSERT_EQ(1, RowCountWhenSettled(1));

  log_service()->Clear();
  EXPECT_EQ(0, RowCountWhenSettled(0));
}

// There is no log service for an off-the-record profile, so the page must not
// be reachable from a private window.
IN_PROC_BROWSER_TEST_F(PsstInternalsUIBrowserTest, NotAvailableOffTheRecord) {
  BrowserWindowInterface* const incognito = CreateIncognitoBrowser();
  ASSERT_FALSE(PsstLogServiceFactory::GetForProfile(incognito->GetProfile()));

  content::WebContents* const incognito_contents =
      incognito->GetActiveTabInterface()->GetContents();
  std::ignore = ui_test_utils::NavigateToURL(incognito, InternalsURL());
  EXPECT_EQ(content::PAGE_TYPE_ERROR, incognito_contents->GetController()
                                          .GetLastCommittedEntry()
                                          ->GetPageType());
}

class PsstInternalsUIDisabledBrowserTest : public InProcessBrowserTest {
 public:
  PsstInternalsUIDisabledBrowserTest() {
    feature_list_.InitAndDisableFeature(features::kEnablePsst);
  }
  ~PsstInternalsUIDisabledBrowserTest() override = default;

 private:
  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(PsstInternalsUIDisabledBrowserTest, NotAvailable) {
  ASSERT_FALSE(PsstLogServiceFactory::GetForProfile(GetProfile()));

  content::WebContents* const contents =
      browser()->GetActiveTabInterface()->GetContents();
  std::ignore = ui_test_utils::NavigateToURL(browser(), InternalsURL());
  EXPECT_EQ(content::PAGE_TYPE_ERROR,
            contents->GetController().GetLastCommittedEntry()->GetPageType());
}

}  // namespace psst
