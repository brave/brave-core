/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/feature_list.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/email_aliases/features.h"
#include "chrome/common/webui_url_constants.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/chrome_pages.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"

class BraveSettingsEmailAliasesRowBrowserTest
    : public InProcessBrowserTest,
      public testing::WithParamInterface<bool> {
 public:
  BraveSettingsEmailAliasesRowBrowserTest() {
    scoped_feature_list_.InitWithFeatureState(email_aliases::kEmailAliases,
                                              GetParam());
  }

 protected:
  content::WebContents* contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  bool FeatureEnabled() { return GetParam(); }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

// Tests that the Email Aliases link row is visible only if the feature is
// enabled and that clicking on it navigates to the Email Aliases page.
IN_PROC_BROWSER_TEST_P(BraveSettingsEmailAliasesRowBrowserTest,
                       EmailAliasesRow_VisibilityAndNavigation) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), chrome::GetSettingsUrl("autofill")));

  // Inject a local helper to pierce shadow DOMs and store the row globally.
  ASSERT_TRUE(content::EvalJs(contents(), R"JS(
    (function() {
      function deepQuerySelector(root, selector) {
        const direct = root.querySelector(selector);
        if (direct) return direct;
        const all = root.querySelectorAll('*');
        for (const el of all) {
          if (el.shadowRoot) {
            const found = deepQuerySelector(el.shadowRoot, selector);
            if (found) return found;
          }
        }
        return null;
      }
      window.emailAliasesRow = deepQuerySelector(document, '#emailAliasesLinkRow');
      return true;
    })();
  )JS").ExtractBool());

  const bool enabled = FeatureEnabled();

  // Email Aliases link row should exist only if the feature is enabled.
  EXPECT_EQ(enabled, content::EvalJs(contents(), R"JS(
    !!window.emailAliasesRow
  )JS").ExtractBool());

  if (!enabled) {
    return;
  }

  // Clicking on the Email Aliases link row should navigate to the Email Aliases
  // page.
  ASSERT_TRUE(content::EvalJs(contents(), R"JS(
    window.emailAliasesRow.click();
    true;
  )JS").ExtractBool());

  EXPECT_EQ(chrome::GetSettingsUrl("email-aliases"), contents()->GetVisibleURL());
}

INSTANTIATE_TEST_SUITE_P(All,
                         BraveSettingsEmailAliasesRowBrowserTest,
                         testing::Bool());
