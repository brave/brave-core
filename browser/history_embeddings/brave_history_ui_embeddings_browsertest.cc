/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/strings/escape.h"
#include "base/strings/strcat.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/local_ai/core/pref_names.h"
#include "chrome/browser/history_embeddings/history_embeddings_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/webui_url_constants.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/history_embeddings/core/history_embeddings_features.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace history_embeddings {

// The Semantic history search setting is live, but the embeddings service is
// built from it once at profile setup, so the two can disagree until the
// browser relaunches. brave://history carries no toggle, so the only value it
// runs on is the session-captured one that
// `chromium_src/chrome/browser/ui/webui/history/history_ui.cc` substitutes for
// upstream's `enableHistoryEmbeddings`. These tests assert the page never runs
// a search against a service that was never built, because the `Search()` it
// would reach CHECKs that service is non-null.
class BraveHistoryUIEmbeddingsBrowserTest : public InProcessBrowserTest {
 public:
  BraveHistoryUIEmbeddingsBrowserTest() {
    feature_list_.InitAndEnableFeature(kHistoryEmbeddings);
  }

  void SetSemanticHistorySearchEnabled(bool enabled) {
    browser()->GetProfile()->GetPrefs()->SetBoolean(
        local_ai::prefs::kBraveHistoryEmbeddingsEnabled, enabled);
  }

  bool ServiceExists() {
    return HistoryEmbeddingsServiceFactory::GetForProfile(
               browser()->GetProfile()) != nullptr;
  }

  void NavigateToHistory() { NavigateToHistoryWithQuery(std::string()); }

  // A query in the URL is applied as the page loads, which is what makes the
  // page reach for the embeddings service on the loadTimeData value alone.
  void NavigateToHistoryWithQuery(const std::string& query) {
    GURL url(chrome::kChromeUIHistoryURL);
    if (!query.empty()) {
      url = GURL(base::StrCat(
          {chrome::kChromeUIHistoryURL, "?q=", base::EscapeQueryParamValue(
                                                   query, /*use_plus=*/true)}));
    }
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
    ASSERT_TRUE(content::WaitForLoadStop(web_contents()));
    ASSERT_TRUE(content::ExecJs(
        web_contents(), "customElements.whenDefined('history-app');"));
  }

  // Asked for on each use rather than held: the browser tears the tab strip
  // down while the fixture is still alive, so a member would outlive it and
  // trip the dangling-pointer check.
  content::WebContents* web_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  // Whether the page will run an embeddings search at all: the setting is on
  // and the service it needs was built. Upstream gates every call into that
  // service on this, including the `Search()` that CHECKs it is non-null.
  // `loadTimeData` is a module on brave://history, not a global.
  bool SearchEnabled() {
    return content::EvalJs(
               web_contents(),
               "import('chrome://resources/js/load_time_data.js')"
               ".then(m => m.loadTimeData.getBoolean("
               "'enableHistoryEmbeddings'))")
        .ExtractBool();
  }

  // Leaves a multi-word query in the search box, which is what makes the page
  // reach for the embeddings service: upstream only runs an embeddings search
  // at two or more words.
  void SetLeftoverQuery() {
    ASSERT_TRUE(content::ExecJs(
        web_contents(),
        "document.querySelector('history-app').shadowRoot"
        "    .querySelector('history-toolbar').searchField"
        "    .setValue('semantic history')"));
  }

  // The element the page only renders once it decides to run an embeddings
  // search. Upstream gates it on `shouldShowHistoryEmbeddings_()`, which needs
  // the search flag *and* a two-or-more-word query.
  bool HasEmbeddingsSearchElement() {
    return content::EvalJs(
               web_contents(),
               "!!document.querySelector('history-app').shadowRoot"
               "    .querySelector('#historyEmbeddingsContainer"
               " cr-history-embeddings')")
        .ExtractBool();
  }

  // The query reaches the app an update cycle after it is typed:
  // `setValue()` updates `history-query-manager.queryState` synchronously, but
  // the app only learns of it through the `query-state-changed` notify that
  // `CrLitElement::updated()` dispatches, so the app re-renders later. Bounded
  // so a render that never comes fails here rather than hanging until the
  // browser-test timeout.
  void WaitForEmbeddingsSearchElement() {
    ASSERT_EQ("rendered",
              content::EvalJs(
                  web_contents(),
                  "new Promise(resolve => {"
                  "  const start = performance.now();"
                  "  const check = () => {"
                  "    if (document.querySelector('history-app').shadowRoot"
                  "        .querySelector('#historyEmbeddingsContainer"
                  " cr-history-embeddings')) {"
                  "      resolve('rendered');"
                  "    } else if (performance.now() - start >= 10000) {"
                  "      resolve('timed out waiting for render');"
                  "    } else {"
                  "      setTimeout(check, 20);"
                  "    }"
                  "  };"
                  "  check();"
                  "})"));
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

// A fresh profile that never touched the setting: no service, so no search.
IN_PROC_BROWSER_TEST_F(BraveHistoryUIEmbeddingsBrowserTest,
                       NoSearchWhileTheSettingIsUntouched) {
  ASSERT_FALSE(browser()->GetProfile()->GetPrefs()->GetBoolean(
      local_ai::prefs::kBraveHistoryEmbeddingsEnabled));
  NavigateToHistory();

  EXPECT_FALSE(SearchEnabled());
}

// Regression test for a crash. Turning the setting on mid-session leaves it
// disagreeing with the service built at profile setup, so the page must keep
// the search off even with a query already in the box. The query arrives in the
// URL so the page applies it as it loads, which is the path that runs on the
// loadTimeData value alone.
IN_PROC_BROWSER_TEST_F(BraveHistoryUIEmbeddingsBrowserTest,
                       NoSearchWhenTurnedOnMidSession) {
  SetSemanticHistorySearchEnabled(true);
  ASSERT_FALSE(ServiceExists());

  NavigateToHistoryWithQuery("semantic history");

  EXPECT_FALSE(SearchEnabled());
  EXPECT_FALSE(HasEmbeddingsSearchElement());
}

// Leaves the setting on for the run below.
IN_PROC_BROWSER_TEST_F(BraveHistoryUIEmbeddingsBrowserTest,
                       PRE_SearchRunsAfterRelaunch) {
  SetSemanticHistorySearchEnabled(true);
}

// The setting was on at profile setup, so the service is built for it and the
// same query now reaches it.
IN_PROC_BROWSER_TEST_F(BraveHistoryUIEmbeddingsBrowserTest,
                       SearchRunsAfterRelaunch) {
  ASSERT_TRUE(browser()->GetProfile()->GetPrefs()->GetBoolean(
      local_ai::prefs::kBraveHistoryEmbeddingsEnabled));
  ASSERT_TRUE(ServiceExists());
  NavigateToHistory();

  EXPECT_TRUE(SearchEnabled());

  SetLeftoverQuery();

  ASSERT_NO_FATAL_FAILURE(WaitForEmbeddingsSearchElement());
}

// Leaves the setting on for the run below.
IN_PROC_BROWSER_TEST_F(BraveHistoryUIEmbeddingsBrowserTest,
                       PRE_SearchSurvivesTurningTheSettingOffMidSession) {
  SetSemanticHistorySearchEnabled(true);
}

// Turning the setting off mid-session leaves the search running, because the
// service built at profile setup is still there and the page runs on the value
// it was built with. The setting takes effect on the next relaunch, the same
// way turning it on does.
IN_PROC_BROWSER_TEST_F(BraveHistoryUIEmbeddingsBrowserTest,
                       SearchSurvivesTurningTheSettingOffMidSession) {
  NavigateToHistory();
  ASSERT_TRUE(SearchEnabled());

  SetSemanticHistorySearchEnabled(false);
  NavigateToHistory();

  EXPECT_TRUE(SearchEnabled());
  EXPECT_TRUE(ServiceExists());
}

// Leaves the setting off for the run below, after a session that had it on.
IN_PROC_BROWSER_TEST_F(BraveHistoryUIEmbeddingsBrowserTest,
                       PRE_NoSearchAfterRelaunchOnceTurnedOff) {
  SetSemanticHistorySearchEnabled(false);
}

// Once the relaunch happens, the setting is off at profile setup, so no service
// is built and the page runs no search.
IN_PROC_BROWSER_TEST_F(BraveHistoryUIEmbeddingsBrowserTest,
                       NoSearchAfterRelaunchOnceTurnedOff) {
  ASSERT_FALSE(browser()->GetProfile()->GetPrefs()->GetBoolean(
      local_ai::prefs::kBraveHistoryEmbeddingsEnabled));
  NavigateToHistory();

  EXPECT_FALSE(SearchEnabled());
}

}  // namespace history_embeddings
