/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/test/scoped_feature_list.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/webui_url_constants.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/sync/base/features.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace {

// The history sync promo offers Google sign-in, which brave://history must
// never do: its button reaches `SigninViewController::ShowDiceSigninTab()`,
// which NOTREACHED()s in DCHECK builds because Brave pins account consistency
// to kDisabled. Upstream decides it in
// `BrowsingHistoryHandler::ShouldShowHistoryPageHistorySyncPromo()` from the
// shown/dismissed pref counters alone, so a signed-out profile would get it up
// to five times; `rewrite/chrome/browser/ui/webui/history/
// browsing_history_handler.cc.yaml` makes that answer always false.
class BraveHistorySyncPromoBrowserTest : public InProcessBrowserTest {
 public:
  BraveHistorySyncPromoBrowserTest() {
    // The page also gates the promo on this feature, enabled by default on
    // desktop since 7ac98441fb0ba. Pinned on here so the test keeps proving the
    // suppression rather than silently riding an upstream default.
    feature_list_.InitAndEnableFeature(syncer::kUnoPhase2FollowUp);
  }

  void NavigateToHistory() {
    ASSERT_TRUE(ui_test_utils::NavigateToURL(
        browser(), GURL(chrome::kChromeUIHistoryURL)));
    ASSERT_TRUE(content::WaitForLoadStop(web_contents()));
    ASSERT_TRUE(content::ExecJs(web_contents(),
                                "customElements.whenDefined('history-app');"));
  }

  // Asked for on each use rather than held: the browser tears the tab strip
  // down while the fixture is still alive, so a member would outlive it and
  // trip the dangling-pointer check.
  content::WebContents* web_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  // The sibling promo from the same `not is_chromeos` block, rendered
  // unconditionally (just hidden). Proves the block is in the DOM and the
  // selector root is right, so the assertion below fails loudly if upstream
  // moves this markup instead of passing on a stale selector.
  bool HasCrossDeviceSigninPromoElement() {
    return content::EvalJs(
               web_contents(),
               "!!document.querySelector('history-app').shadowRoot"
               "    .querySelector('#historyCrossDeviceSigninPromo')")
        .ExtractBool();
  }

  // Whether `<history-sync-promo>` ever shows up. The decision reaches the page
  // over Mojo after the load settles, so an immediate query could miss a promo
  // that is merely late; this keeps looking for one until the window is up and
  // only then reports it absent.
  bool SyncPromoElementStaysAbsent() {
    return content::EvalJs(
               web_contents(),
               "new Promise(resolve => {"
               "  const start = performance.now();"
               "  const check = () => {"
               "    if (document.querySelector('history-app').shadowRoot"
               "        .querySelector('history-sync-promo')) {"
               "      resolve(false);"
               "    } else if (performance.now() - start >= 2000) {"
               "      resolve(true);"
               "    } else {"
               "      setTimeout(check, 20);"
               "    }"
               "  };"
               "  check();"
               "})")
        .ExtractBool();
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

// A fresh test profile is signed out, which upstream reports as history sync
// kTurnedOff — the state that lets the promo through.
IN_PROC_BROWSER_TEST_F(BraveHistorySyncPromoBrowserTest,
                       NoSyncPromoWhenSignedOut) {
  NavigateToHistory();

  ASSERT_TRUE(HasCrossDeviceSigninPromoElement());
  EXPECT_TRUE(SyncPromoElementStaysAbsent());
}

}  // namespace
