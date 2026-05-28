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
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/top_container_view.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/scoped_animation_duration_scale_mode.h"
#include "ui/views/focus/focus_manager.h"
#include "ui/views/view.h"

class FocusModeTopOverlayBrowserTest : public InProcessBrowserTest {
 protected:
  FocusModeTopOverlayBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS),
        https_server_expired_(net::EmbeddedTestServer::TYPE_HTTPS) {
    feature_list_.InitAndEnableFeature(features::kBraveFocusMode);
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    // Resolve all hosts to the test servers so that non-localhost hostnames
    // (used to obtain a "Not Secure" security level over HTTP) work.
    host_resolver()->AddRule("*", "127.0.0.1");

    embedded_test_server()->ServeFilesFromSourceDirectory("chrome/test/data");
    ASSERT_TRUE(embedded_test_server()->Start());

    https_server_.SetSSLConfig(net::EmbeddedTestServer::CERT_OK);
    https_server_.ServeFilesFromSourceDirectory("chrome/test/data");
    ASSERT_TRUE(https_server_.Start());

    https_server_expired_.SetSSLConfig(net::EmbeddedTestServer::CERT_EXPIRED);
    https_server_expired_.ServeFilesFromSourceDirectory("chrome/test/data");
    ASSERT_TRUE(https_server_expired_.Start());
  }

  BraveBrowserView* browser_view() {
    return BraveBrowserView::From(
        BrowserView::GetBrowserViewForBrowser(browser()));
  }

  FocusModeController* focus_mode_controller() {
    return browser()->GetFeatures().focus_mode_controller();
  }

  content::WebContents* active_web_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
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

  bool WaitForOverlayActive(bool active) {
    auto* overlay = browser_view()->focus_mode_top_overlay();
    return base::test::RunUntil([&]() { return overlay->active() == active; });
  }

  net::EmbeddedTestServer https_server_;
  net::EmbeddedTestServer https_server_expired_;

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

IN_PROC_BROWSER_TEST_F(FocusModeTopOverlayBrowserTest,
                       PreservesFocusWhenReparenting) {
  auto* overlay = browser_view()->focus_mode_top_overlay();
  ASSERT_TRUE(overlay);
  auto* top_container = browser_view()->top_container();
  ASSERT_TRUE(top_container);
  auto* focus_manager = browser_view()->GetFocusManager();
  ASSERT_TRUE(focus_manager);

  // Focus the location bar (omnibox), which lives inside the top container.
  chrome::FocusLocationBar(browser());
  views::View* focused = focus_manager->GetFocusedView();
  ASSERT_TRUE(focused);
  ASSERT_TRUE(top_container->Contains(focused));

  // Activating Focus Mode reparents the top container into the overlay. Focus
  // should travel with it.
  focus_mode_controller()->SetEnabled(true);
  ASSERT_TRUE(overlay->active());
  EXPECT_EQ(focus_manager->GetFocusedView(), focused);
  EXPECT_TRUE(overlay->Contains(focused));

  // Deactivating reparents the top container back into the browser view. Focus
  // should again be preserved.
  focus_mode_controller()->SetEnabled(false);
  ASSERT_FALSE(overlay->active());
  EXPECT_EQ(focus_manager->GetFocusedView(), focused);
  EXPECT_TRUE(top_container->Contains(focused));
}

IN_PROC_BROWSER_TEST_F(FocusModeTopOverlayBrowserTest,
                       InsecurePagesDeactivateOverlay) {
  auto* overlay = browser_view()->focus_mode_top_overlay();
  ASSERT_TRUE(overlay);
  auto* top_container = browser_view()->top_container();
  ASSERT_TRUE(top_container);

  focus_mode_controller()->SetEnabled(true);

  // Secure page: the overlay is active and hosts the top container.
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), https_server_.GetURL("/empty.html")));
  ASSERT_TRUE(WaitForOverlayActive(true));
  ASSERT_EQ(top_container->parent(), overlay);

  // Plain HTTP page ("Not Secure"): the overlay deactivates.
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(),
      embedded_test_server()->GetURL("insecure.test", "/empty.html")));
  EXPECT_TRUE(WaitForOverlayActive(false));
  EXPECT_EQ(top_container->parent(), browser_view());

  // Returning to a secure page re-activates the overlay.
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), https_server_.GetURL("/empty.html")));
  ASSERT_TRUE(WaitForOverlayActive(true));
  ASSERT_EQ(top_container->parent(), overlay);

  // Certificate error page (interstitial commits at the requested URL): the
  // overlay deactivates.
  ui_test_utils::NavigateToURLBlockUntilNavigationsComplete(
      browser(), https_server_expired_.GetURL("/empty.html"), 1);
  EXPECT_TRUE(WaitForOverlayActive(false));
  EXPECT_EQ(top_container->parent(), browser_view());
}

IN_PROC_BROWSER_TEST_F(FocusModeTopOverlayBrowserTest,
                       PermissionPromptRevealsOverlay) {
  auto* overlay = browser_view()->focus_mode_top_overlay();
  ASSERT_TRUE(overlay);

  // A secure context is required both to keep Focus Mode active and to request
  // geolocation.
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), https_server_.GetURL("/empty.html")));
  focus_mode_controller()->SetEnabled(true);
  ASSERT_TRUE(overlay->active());

  // With nothing holding it open, the overlay slides out of view.
  ASSERT_TRUE(WaitForOverlayHidden());

  // Requesting a permission shows an anchored prompt, which should reveal the
  // overlay.
  content::ExecuteScriptAsync(
      active_web_contents(),
      "navigator.geolocation.getCurrentPosition(() => {}, () => {});");
  ASSERT_TRUE(WaitForOverlayRevealed());

  // The origin-bearing location bar is now visible within the overlay.
  auto* location_bar = browser_view()->GetLocationBarView();
  ASSERT_TRUE(location_bar);
  EXPECT_TRUE(overlay->Contains(location_bar));
  EXPECT_TRUE(location_bar->IsDrawn());
  EXPECT_EQ(overlay->bounds().y(), 0);
}
