/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#import <AppKit/AppKit.h>

#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/browser/ui/focus_mode/focus_mode_controller.h"
#include "brave/browser/ui/focus_mode/focus_mode_features.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/focus_mode_top_overlay.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "ui/base/test/scoped_fake_nswindow_fullscreen.h"
#include "ui/gfx/scoped_animation_duration_scale_mode.h"
#include "ui/views/widget/widget.h"

class BraveBrowserFrameViewMacBrowserTest : public InProcessBrowserTest {
 public:
  BraveBrowserFrameViewMacBrowserTest() {
    feature_list_.InitAndEnableFeature(features::kBraveFocusMode);
  }

 protected:
  NSWindow* GetNativeWindow() {
    return BrowserView::GetBrowserViewForBrowser(browser())
        ->GetWidget()
        ->GetNativeWindow()
        .GetNativeNSWindow();
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(BraveBrowserFrameViewMacBrowserTest,
                       TitleVisibilityPreservesTitledStyleMask) {
  NSWindow* window = GetNativeWindow();
  ASSERT_TRUE(window);
  EXPECT_TRUE(window.styleMask & NSWindowStyleMaskTitled);

  brave::ToggleVerticalTabStrip(browser());
  EXPECT_TRUE(window.styleMask & NSWindowStyleMaskTitled);
  EXPECT_EQ(window.titleVisibility, NSWindowTitleHidden);

  brave::ToggleWindowTitleVisibilityForVerticalTabs(browser());
  EXPECT_TRUE(window.styleMask & NSWindowStyleMaskTitled);
  EXPECT_EQ(window.titleVisibility, NSWindowTitleVisible);

  brave::ToggleWindowTitleVisibilityForVerticalTabs(browser());
  EXPECT_TRUE(window.styleMask & NSWindowStyleMaskTitled);
  EXPECT_EQ(window.titleVisibility, NSWindowTitleHidden);

  brave::ToggleVerticalTabStrip(browser());
  EXPECT_TRUE(window.styleMask & NSWindowStyleMaskTitled);

  PrefService* local_state = g_browser_process->local_state();
  local_state->SetBoolean(brave_tabs::kCompactHorizontalTabs, true);
  EXPECT_TRUE(window.styleMask & NSWindowStyleMaskTitled);
  EXPECT_EQ(window.titleVisibility, NSWindowTitleHidden);

  local_state->SetBoolean(brave_tabs::kCompactHorizontalTabs, false);
  EXPECT_TRUE(window.styleMask & NSWindowStyleMaskTitled);
}

IN_PROC_BROWSER_TEST_F(BraveBrowserFrameViewMacBrowserTest,
                       FocusModeAndFullscreenPreserveTitledStyleMask) {
  ui::test::ScopedFakeNSWindowFullscreen fake_fullscreen;

  NSWindow* window = GetNativeWindow();
  ASSERT_TRUE(window);
  ASSERT_TRUE(window.styleMask & NSWindowStyleMaskTitled);

  auto* focus_mode_controller = FocusModeController::From(browser());
  ASSERT_TRUE(focus_mode_controller);
  focus_mode_controller->SetEnabled(true);
  EXPECT_TRUE(window.styleMask & NSWindowStyleMaskTitled);
  EXPECT_EQ(window.titleVisibility, NSWindowTitleHidden);

  ui_test_utils::ToggleFullscreenModeAndWait(browser());
  ASSERT_TRUE(BrowserView::GetBrowserViewForBrowser(browser())->IsFullscreen());
  EXPECT_TRUE(window.styleMask & NSWindowStyleMaskTitled);

  ui_test_utils::ToggleFullscreenModeAndWait(browser());
  ASSERT_FALSE(
      BrowserView::GetBrowserViewForBrowser(browser())->IsFullscreen());
  EXPECT_TRUE(window.styleMask & NSWindowStyleMaskTitled);

  focus_mode_controller->SetEnabled(false);
  EXPECT_TRUE(window.styleMask & NSWindowStyleMaskTitled);
}

IN_PROC_BROWSER_TEST_F(BraveBrowserFrameViewMacBrowserTest,
                       FocusModeHidesWindowControlsWithoutDisablingCommands) {
  gfx::ScopedAnimationDurationScaleMode zero_duration_mode(
      gfx::ScopedAnimationDurationScaleMode::ZERO_DURATION);

  net::EmbeddedTestServer https_server(net::EmbeddedTestServer::TYPE_HTTPS);
  https_server.SetSSLConfig(net::EmbeddedTestServer::CERT_OK);
  https_server.ServeFilesFromSourceDirectory("chrome/test/data");
  ASSERT_TRUE(https_server.Start());

  NSWindow* window = GetNativeWindow();
  ASSERT_TRUE(window);

  auto* overlay =
      BraveBrowserView::From(BrowserView::GetBrowserViewForBrowser(browser()))
          ->focus_mode_top_overlay();
  ASSERT_TRUE(overlay);

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(),
                                           https_server.GetURL("/empty.html")));

  constexpr NSWindowButton kButtons[] = {
      NSWindowCloseButton, NSWindowMiniaturizeButton, NSWindowZoomButton};

  for (NSWindowButton kind : kButtons) {
    NSButton* button = [window standardWindowButton:kind];
    ASSERT_TRUE(button);
    EXPECT_FALSE([button isHidden]);
    EXPECT_EQ([button alphaValue], 1.0);
  }

  auto* focus_mode_controller = FocusModeController::From(browser());
  ASSERT_TRUE(focus_mode_controller);
  focus_mode_controller->SetEnabled(true);
  ASSERT_TRUE(overlay->active());
  ASSERT_TRUE(base::test::RunUntil(
      [&]() { return overlay->GetRevealFraction() == 0.0; }));

  for (NSWindowButton kind : kButtons) {
    NSButton* button = [window standardWindowButton:kind];
    ASSERT_TRUE(button);
    EXPECT_TRUE([button isHidden] || [button alphaValue] == 0.0);
  }

  for (SEL action : {
         @selector(performClose:), @selector(performMiniaturize:),
             @selector(performZoom:)
       }) {
    NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:@""
                                                  action:action
                                           keyEquivalent:@""];
    EXPECT_TRUE([window validateUserInterfaceItem:item]);
  }

  focus_mode_controller->SetEnabled(false);

  for (NSWindowButton kind : kButtons) {
    NSButton* button = [window standardWindowButton:kind];
    ASSERT_TRUE(button);
    EXPECT_FALSE([button isHidden]);
    EXPECT_EQ([button alphaValue], 1.0);
  }
}
