/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/focus_mode_top_overlay.h"

#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/ui/focus_mode/focus_mode_controller.h"
#include "brave/browser/ui/focus_mode/focus_mode_features.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/top_container_view.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_test.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/scoped_animation_duration_scale_mode.h"

class FocusModeTopOverlayBrowserTest : public InProcessBrowserTest {
 protected:
  FocusModeTopOverlayBrowserTest() {
    feature_list_.InitAndEnableFeature(features::kBraveFocusMode);
  }

  BraveBrowserView* browser_view() {
    return BraveBrowserView::From(
        BrowserView::GetBrowserViewForBrowser(browser()));
  }

  FocusModeController* focus_mode_controller() {
    return browser()->GetFeatures().focus_mode_controller();
  }

  bool WaitForOverlayRevealed() {
    auto* overlay = browser_view()->focus_mode_top_overlay();
    return base::test::RunUntil(
        [&]() { return overlay->GetRevealFraction() == 1.0; });
  }

  bool WaitForOverlayHidden() {
    auto* overlay = browser_view()->focus_mode_top_overlay();
    return base::test::RunUntil(
        [&]() { return overlay->GetRevealFraction() == 0.0; });
  }

 private:
  base::test::ScopedFeatureList feature_list_;
  gfx::ScopedAnimationDurationScaleMode zero_duration_mode_{
      gfx::ScopedAnimationDurationScaleMode::ZERO_DURATION};
};

IN_PROC_BROWSER_TEST_F(FocusModeTopOverlayBrowserTest,
                       FocusModeActivatesOverlay) {
  auto* overlay = browser_view()->focus_mode_top_overlay();
  ASSERT_TRUE(overlay);
  EXPECT_FALSE(overlay->active());

  auto* top_container = browser_view()->top_container();
  ASSERT_TRUE(top_container);
  EXPECT_EQ(top_container->parent(), browser_view());

  // Activate Focus Mode and wait for slide-out.
  focus_mode_controller()->SetEnabled(true);
  EXPECT_TRUE(overlay->active());
  ASSERT_TRUE(WaitForOverlayHidden());

  // Verify that the top container and tabstrip are reparented correctly.
  EXPECT_EQ(top_container->parent(), overlay);
  EXPECT_TRUE(
      overlay->Contains(browser_view()->horizontal_tab_strip_for_testing()));
  EXPECT_TRUE(overlay->Contains(browser_view()->toolbar()));

  // Activating Focus Mode moves the overlay to the end of the child list so
  // that it renders above its siblings, while the find bar host view remains
  // the last child to keep the find bar widget on top of the overlay.
  auto& browser_view_children = browser_view()->children();
  ASSERT_GT(browser_view_children.size(), 1ul);
  ASSERT_EQ(browser_view_children.back(), browser_view()->find_bar_host_view());
  EXPECT_EQ(browser_view_children[browser_view_children.size() - 2], overlay);

  // When hidden, the overlay is slid completely out of the window bounds.
  EXPECT_EQ(overlay->bounds().bottom(), 0);
  EXPECT_EQ(overlay->width(), browser_view()->width());

  // Verify that `RevealTemporarily` reveals the overlay.
  overlay->RevealTemporarily(base::Milliseconds(100));
  ASSERT_TRUE(WaitForOverlayRevealed());

  // When revealed, the overlay sits at the top edge, spans the browser width,
  // and is as tall as the top container.
  EXPECT_EQ(overlay->bounds().y(), 0);
  EXPECT_EQ(overlay->width(), browser_view()->width());
  EXPECT_EQ(overlay->height(), top_container->height());

  ASSERT_TRUE(WaitForOverlayHidden());

  // Deactivate Focus Mode.
  focus_mode_controller()->SetEnabled(false);
  EXPECT_FALSE(overlay->active());
  EXPECT_EQ(top_container->parent(), browser_view());
}
