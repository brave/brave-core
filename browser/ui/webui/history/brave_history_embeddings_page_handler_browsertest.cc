/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/strings/escape.h"
#include "base/strings/strcat.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/local_ai/core/pref_names.h"
#include "chrome/browser/history_embeddings/history_embeddings_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
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

// The Semantic History Search setting is live, but the embeddings service is
// built from it once at profile setup, so the two can disagree. These tests
// assert what brave://history itself ends up running on, because the page gates
// its calls into the embeddings service on `enableHistoryEmbeddings` and that
// service can be null.
class BraveHistoryEmbeddingsPageHandlerBrowserTest
    : public InProcessBrowserTest {
 public:
  BraveHistoryEmbeddingsPageHandlerBrowserTest() {
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

  // A query in the URL is applied as the page loads, so it can reach the
  // embeddings service before the page handler's first Mojo push lands. That
  // makes it the one path that exercises the loadTimeData snapshot rather than
  // the push.
  void NavigateToHistoryWithQuery(const std::string& query) {
    GURL url(chrome::kChromeUIHistoryURL);
    if (!query.empty()) {
      url = GURL(base::StrCat(
          {chrome::kChromeUIHistoryURL, "?q=", base::EscapeQueryParamValue(
                                                  query, /*use_plus=*/true)}));
    }
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
    ASSERT_TRUE(content::WaitForLoadStop(web_contents()));
    // The Brave overrides live in the history-app module, and app.ts subscribes
    // to the Mojo push as a side effect of loading it.
    ASSERT_TRUE(content::ExecJs(
        web_contents(), "customElements.whenDefined('history-app');"));
  }

  // Asked for on each use rather than held: the browser tears the tab strip
  // down while the fixture is still alive, so a member would outlive it and
  // trip the dangling-pointer check.
  content::WebContents* web_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  // `loadTimeData` is a module on brave://history, not a global.
  bool GetLoadTimeBoolean(const std::string& key) {
    return content::EvalJs(
               web_contents(),
               base::StrCat(
                   {"import('chrome://resources/js/load_time_data.js')"
                    ".then(m => m.loadTimeData.getBoolean('",
                    key, "'))"}))
        .ExtractBool();
  }

  // Whether the page will run an embeddings search at all: the setting is on
  // and the service it needs was built. Upstream gates every call into that
  // service on this, including the `Search()` that CHECKs it is non-null.
  bool SearchEnabled() { return GetLoadTimeBoolean("enableHistoryEmbeddings"); }

  // What the side bar toggle shows.
  bool IsToggleChecked() {
    return GetLoadTimeBoolean("braveHistoryEmbeddingsEnabled");
  }

  // Whether the side bar offers its "Relaunch" button.
  bool NeedsRestart() {
    return GetLoadTimeBoolean("braveHistoryEmbeddingsNeedsRestart");
  }

  // Flips the toggle the way the side bar does, then waits for the resulting
  // Mojo push to land back in `loadTimeData`. Sets `checked` and fires
  // `change` rather than clicking: `cr-toggle` attaches its click handler in
  // `firstUpdated()`, which never runs on an element rendered from
  // `CrLitElement::connectedCallback()`'s `ensureInitialRender()`, so a click
  // is a no-op here. Bounded so a push that never arrives fails here rather
  // than hanging until the browser-test timeout.
  void ClickToggle(bool enabled) {
    ASSERT_TRUE(content::ExecJs(
        web_contents(),
        content::JsReplace(
            "(() => {"
            "  const sb = document.querySelector('history-app').shadowRoot"
            "      .querySelector('history-side-bar');"
            "  const t = sb.shadowRoot.querySelector("
            "      '#brave-history-embeddings-toggle cr-toggle');"
            "  t.checked = $1;"
            "  t.dispatchEvent(new CustomEvent("
            "      'change', {detail: $1, bubbles: true, composed: true}));"
            "})()",
            enabled)));
    // The pref write round-trips through the browser and back as a push.
    ASSERT_EQ("pushed",
              content::EvalJs(
                  web_contents(),
                  content::JsReplace(
                      "new Promise(resolve => {"
                      "  const start = performance.now();"
                      "  const check = () => import("
                      "      'chrome://resources/js/load_time_data.js')"
                      "      .then(m => {"
                      "        if (m.loadTimeData.getBoolean("
                      "            'braveHistoryEmbeddingsEnabled') === $1) {"
                      "          resolve('pushed');"
                      "        } else if (performance.now() - start >= 10000) {"
                      "          resolve('timed out waiting for push');"
                      "        } else {"
                      "          setTimeout(check, 20);"
                      "        }"
                      "      });"
                      "  check();"
                      "})",
                      enabled)));
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

// A fresh profile that never touched the toggle: no service, and nothing
// waiting on a relaunch.
IN_PROC_BROWSER_TEST_F(BraveHistoryEmbeddingsPageHandlerBrowserTest,
                       NothingPendingWhileTheSettingIsUntouched) {
  ASSERT_FALSE(browser()->GetProfile()->GetPrefs()->GetBoolean(
      local_ai::prefs::kBraveHistoryEmbeddingsEnabled));
  NavigateToHistory();

  EXPECT_FALSE(IsToggleChecked());
  EXPECT_FALSE(SearchEnabled());
  EXPECT_FALSE(NeedsRestart());
}

// Regression test for a crash. Turning the toggle on with a leftover multi-word
// query used to leave the page believing embeddings were live, so it sent a
// search into a service that was never built and CHECK-failed. The setting is
// on and the page shows it, but the search stays off until a relaunch.
IN_PROC_BROWSER_TEST_F(BraveHistoryEmbeddingsPageHandlerBrowserTest,
                       NoSearchWhenTurnedOnWithALeftoverQuery) {
  NavigateToHistory();
  SetLeftoverQuery();

  ASSERT_NO_FATAL_FAILURE(ClickToggle(true));

  // The toggle shows the value the user just picked, and the side bar offers
  // the relaunch that will make it take effect.
  EXPECT_TRUE(IsToggleChecked());
  EXPECT_TRUE(NeedsRestart());
  // But there is no service, so the page must not reach for one.
  ASSERT_FALSE(ServiceExists());
  EXPECT_FALSE(SearchEnabled());
  EXPECT_FALSE(HasEmbeddingsSearchElement());
}

// The same crash, reached through the loadTimeData snapshot instead of the Mojo
// push. The setting is already on from an earlier page load, still with no
// service behind it, and the query arrives in the URL so the page applies it as
// it loads — before the page handler's first push can correct a bad snapshot.
IN_PROC_BROWSER_TEST_F(BraveHistoryEmbeddingsPageHandlerBrowserTest,
                       NoSearchWhenLoadedWithAQueryWhileWaitingOnRelaunch) {
  SetSemanticHistorySearchEnabled(true);
  ASSERT_FALSE(ServiceExists());

  NavigateToHistoryWithQuery("semantic history");

  EXPECT_TRUE(IsToggleChecked());
  EXPECT_TRUE(NeedsRestart());
  EXPECT_FALSE(SearchEnabled());
  EXPECT_FALSE(HasEmbeddingsSearchElement());
}

// Leaves the setting on for the run below.
IN_PROC_BROWSER_TEST_F(BraveHistoryEmbeddingsPageHandlerBrowserTest,
                       PRE_SearchRunsAfterRelaunch) {
  SetSemanticHistorySearchEnabled(true);
}

// The setting was on at profile setup, so the service is built for it and the
// same query now reaches it.
IN_PROC_BROWSER_TEST_F(BraveHistoryEmbeddingsPageHandlerBrowserTest,
                       SearchRunsAfterRelaunch) {
  ASSERT_TRUE(browser()->GetProfile()->GetPrefs()->GetBoolean(
      local_ai::prefs::kBraveHistoryEmbeddingsEnabled));
  ASSERT_TRUE(ServiceExists());
  NavigateToHistory();

  EXPECT_TRUE(IsToggleChecked());
  EXPECT_TRUE(SearchEnabled());
  EXPECT_FALSE(NeedsRestart());

  SetLeftoverQuery();

  ASSERT_NO_FATAL_FAILURE(WaitForEmbeddingsSearchElement());
}

// Leaves the setting on for the run below.
IN_PROC_BROWSER_TEST_F(BraveHistoryEmbeddingsPageHandlerBrowserTest,
                       PRE_NoSearchOnceTurnedOff) {
  SetSemanticHistorySearchEnabled(true);
}

// Turning the setting off takes the search away immediately, even though the
// service built at profile setup outlives it.
IN_PROC_BROWSER_TEST_F(BraveHistoryEmbeddingsPageHandlerBrowserTest,
                       NoSearchOnceTurnedOff) {
  NavigateToHistory();
  ASSERT_TRUE(SearchEnabled());

  ASSERT_NO_FATAL_FAILURE(ClickToggle(false));

  EXPECT_FALSE(IsToggleChecked());
  EXPECT_FALSE(SearchEnabled());
  EXPECT_TRUE(NeedsRestart());
  // The service outlives the setting, so the gate is the setting, not the
  // service.
  EXPECT_TRUE(ServiceExists());
}

}  // namespace history_embeddings
