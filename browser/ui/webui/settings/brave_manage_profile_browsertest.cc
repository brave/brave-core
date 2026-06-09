// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "build/build_config.h"

#if !BUILDFLAG(IS_CHROMEOS)

#include "chrome/browser/ui/browser.h"
#include "chrome/common/chrome_isolated_world_ids.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "url/gurl.h"

namespace {

content::EvalJsResult WaitForManageProfilePickerLayout(
    content::WebContents* web_contents) {
  constexpr char kScript[] = R"(
  new Promise((resolve) => {
    const expectedColumnCount = 7;
    const timeoutMs = 10000;
    const startTime = performance.now();
    let lastStatus = 'Manage Profile picker layout did not run';

    function deepQuery(root, selector) {
      const direct = root.querySelector(selector);
      if (direct) {
        return direct;
      }
      for (const element of root.querySelectorAll('*')) {
        const found =
            element.shadowRoot && deepQuery(element.shadowRoot, selector);
        if (found) {
          return found;
        }
      }
      return null;
    }

    function renderedColumnCount(pickerSelector) {
      const page = deepQuery(document, 'settings-manage-profile');
      const picker = page?.shadowRoot?.querySelector(pickerSelector);
      const grid = picker?.shadowRoot?.querySelector('cr-grid');
      const gridElement = grid?.shadowRoot?.querySelector('#grid');
      if (!gridElement) {
        return null;
      }

      const gridTemplateColumns =
          getComputedStyle(gridElement).gridTemplateColumns.trim();
      if (!gridTemplateColumns || gridTemplateColumns === 'none') {
        return null;
      }

      return gridTemplateColumns.split(/\s+/).length;
    }

    function checkPickerLayout(pickerSelector) {
      const actualColumnCount = renderedColumnCount(pickerSelector);
      if (actualColumnCount === expectedColumnCount) {
        return 'ready';
      }
      if (actualColumnCount === null) {
        return `${pickerSelector} grid not ready`;
      }

      return `${pickerSelector} rendered ${actualColumnCount} columns`;
    }

    function getLayoutStatus() {
      for (const pickerSelector of [
        'cr-theme-color-picker',
        'cr-profile-avatar-selector',
      ]) {
        const status = checkPickerLayout(pickerSelector);
        if (status !== 'ready') {
          return status;
        }
      }

      return 'ready';
    }

    function maybeFinish() {
      lastStatus = getLayoutStatus();
      if (lastStatus === 'ready' ||
          performance.now() - startTime >= timeoutMs) {
        resolve(lastStatus);
        return;
      }
      setTimeout(maybeFinish, 50);
    }

    maybeFinish();
  })
)";

  return content::EvalJs(web_contents, kScript,
                         content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                         ISOLATED_WORLD_ID_BRAVE_INTERNAL);
}

}  // namespace

class BraveManageProfileBrowserTest : public InProcessBrowserTest {
 public:
  BraveManageProfileBrowserTest() = default;
  ~BraveManageProfileBrowserTest() override = default;
};

// Verifies Brave renders Manage Profile theme/avatar pickers in seven columns.
IN_PROC_BROWSER_TEST_F(BraveManageProfileBrowserTest,
                       ManageProfilePickerUsesSevenColumns) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), GURL("chrome://settings/manageProfile")));

  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_NE(web_contents, nullptr);
  ASSERT_TRUE(content::WaitForLoadStop(web_contents));
  EXPECT_EQ(web_contents->GetURL(), GURL("chrome://settings/manageProfile"));

  ASSERT_EQ("ready", WaitForManageProfilePickerLayout(web_contents));
}

#endif  // !BUILDFLAG(IS_CHROMEOS)
