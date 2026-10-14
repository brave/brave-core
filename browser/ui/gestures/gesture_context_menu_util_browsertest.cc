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

// The suppress/defer branches need actual mouse input to reach, so they're
// covered by `GestureController`'s own tests; this covers the lookup only.
class GestureContextMenuUtilBrowserTest : public PlatformBrowserTest {
 public:
  content::WebContents* active_web_contents() {
    return chrome_test_utils::GetActiveWebContents(this);
  }

  void ShowContextMenuCallback() { ++show_context_menu_count_; }

  size_t show_context_menu_count() const { return show_context_menu_count_; }

 private:
  size_t show_context_menu_count_ = 0;
};

IN_PROC_BROWSER_TEST_F(GestureContextMenuUtilBrowserTest,
                       ShouldNotSuppressContextMenuWhenPageHasNoTab) {
  const content::WebContents::CreateParams create_params(
      active_web_contents()->GetBrowserContext());
  const std::unique_ptr<content::WebContents> web_contents =
      content::WebContents::Create(create_params);

  const bool suppressed = ConsumeSuppressContextMenuForWebContents(
      web_contents.get(),
      base::BindOnce(
          &GestureContextMenuUtilBrowserTest::ShowContextMenuCallback,
          base::Unretained(this)));

  EXPECT_FALSE(suppressed);
  EXPECT_EQ(0U, show_context_menu_count());
}

IN_PROC_BROWSER_TEST_F(
    GestureContextMenuUtilBrowserTest,
    ShouldNotSuppressContextMenuWhenNoGestureIsActiveOrSuppressed) {
  const bool suppressed = ConsumeSuppressContextMenuForWebContents(
      active_web_contents(),
      base::BindOnce(
          &GestureContextMenuUtilBrowserTest::ShowContextMenuCallback,
          base::Unretained(this)));

  EXPECT_FALSE(suppressed);
  EXPECT_EQ(0U, show_context_menu_count());
}

}  // namespace gestures
