// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/gestures/gesture_context_menu_util.h"

#include <memory>

#include "base/functional/bind.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/platform_browser_test.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"

namespace gestures {

namespace {

// `ConsumeSuppressContextMenuForWebContents`'s suppress/defer branches are
// only reachable through `GestureController`'s own real mouse-input
// handling (there's no public test seam to set that state directly), so
// those are covered by exercising `GestureController` itself rather than
// here; this only covers the lookup wrapper's own behavior.
class GestureContextMenuUtilBrowserTest : public PlatformBrowserTest {
 public:
  // Bound by `IN_PROC_BROWSER_TEST_F`'s generated test body, a derived
  // class, so this must be `public` rather than `protected`.
  void OnShowContextMenu() { ++show_context_menu_count_; }

  size_t show_context_menu_count() const { return show_context_menu_count_; }

 private:
  size_t show_context_menu_count_ = 0;
};

}  // namespace

IN_PROC_BROWSER_TEST_F(GestureContextMenuUtilBrowserTest,
                       ReturnsFalseWhenWebContentsHasNoAssociatedTab) {
  const content::WebContents::CreateParams create_params(
      chrome_test_utils::GetActiveWebContents(this)->GetBrowserContext());
  const std::unique_ptr<content::WebContents> web_contents =
      content::WebContents::Create(create_params);

  const bool suppressed = ConsumeSuppressContextMenuForWebContents(
      web_contents.get(),
      base::BindOnce(&GestureContextMenuUtilBrowserTest::OnShowContextMenu,
                     base::Unretained(this)));

  EXPECT_FALSE(suppressed);
  EXPECT_EQ(0U, show_context_menu_count());
}

IN_PROC_BROWSER_TEST_F(GestureContextMenuUtilBrowserTest,
                       ReturnsFalseWhenThereIsNoActiveOrSuppressedGesture) {
  content::WebContents* const web_contents =
      chrome_test_utils::GetActiveWebContents(this);

  const bool suppressed = ConsumeSuppressContextMenuForWebContents(
      web_contents,
      base::BindOnce(&GestureContextMenuUtilBrowserTest::OnShowContextMenu,
                     base::Unretained(this)));

  EXPECT_FALSE(suppressed);
  EXPECT_EQ(0U, show_context_menu_count());
}

}  // namespace gestures
