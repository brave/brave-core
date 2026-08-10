/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>
#include <string_view>

#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/ui/focus_mode/focus_mode_controller.h"
#include "brave/browser/ui/focus_mode/focus_mode_features.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/focus_mode_title_bar_view.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/contents_container_view.h"
#include "chrome/browser/ui/views/frame/multi_contents_view.h"
#include "chrome/browser/ui/views/frame/multi_contents_view_mini_toolbar.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/scoped_animation_duration_scale_mode.h"
#include "ui/views/controls/label.h"

// Tests the BraveBrowserView wiring that displays the active tab's domain in
// the mini toolbar while Focus Mode is enabled. The mini toolbar's own display
// behavior is covered by the split view browser tests.
class FocusModeMiniToolbarBrowserTestBase : public InProcessBrowserTest {
 protected:
  explicit FocusModeMiniToolbarBrowserTestBase(std::string_view url_display)
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
    feature_list_.InitAndEnableFeatureWithParameters(
        features::kBraveFocusMode,
        {{"FocusModeUrlDisplay", std::string(url_display)}});
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    // Resolve all hosts to the test servers so that non-localhost hostnames
    // (used to obtain a "Not Secure" security level over HTTP) work.
    host_resolver()->AddRule("*", "127.0.0.1");

    embedded_test_server()->ServeFilesFromSourceDirectory("chrome/test/data");
    ASSERT_TRUE(embedded_test_server()->Start());

    https_server_.SetSSLConfig(net::EmbeddedTestServer::CERT_TEST_NAMES);
    https_server_.ServeFilesFromSourceDirectory("chrome/test/data");
    ASSERT_TRUE(https_server_.Start());
  }

  BraveBrowserView* browser_view() {
    return BraveBrowserView::From(
        BrowserView::GetBrowserViewForBrowser(browser()));
  }

  MultiContentsViewMiniToolbar* active_mini_toolbar() {
    return browser_view()
        ->multi_contents_view()
        ->GetActiveContentsContainerView()
        ->mini_toolbar();
  }

  void SetFocusModeEnabled(bool enabled) {
    browser()->GetFeatures().focus_mode_controller()->SetEnabled(enabled);
  }

  bool WaitForMiniToolbarVisible(bool visible) {
    return base::test::RunUntil(
        [&]() { return active_mini_toolbar()->GetVisible() == visible; });
  }

  bool DomainContains(std::u16string_view host) {
    const std::u16string domain(
        active_mini_toolbar()->domain_label_for_testing()->GetText());
    return domain.find(host) != std::u16string::npos;
  }

  net::EmbeddedTestServer https_server_;

 private:
  base::test::ScopedFeatureList feature_list_;
  gfx::ScopedAnimationDurationScaleMode zero_duration_mode_{
      gfx::ScopedAnimationDurationScaleMode::ZERO_DURATION};
};

class FocusModeMiniToolbarBrowserTest
    : public FocusModeMiniToolbarBrowserTestBase {
 protected:
  FocusModeMiniToolbarBrowserTest()
      : FocusModeMiniToolbarBrowserTestBase("mini-toolbar") {}
};

class FocusModeTitleBarUrlDisplayBrowserTest
    : public FocusModeMiniToolbarBrowserTestBase {
 protected:
  FocusModeTitleBarUrlDisplayBrowserTest()
      : FocusModeMiniToolbarBrowserTestBase("title-bar") {}
};

IN_PROC_BROWSER_TEST_F(FocusModeMiniToolbarBrowserTest, MiniToolbarWiring) {
  // The title bar is not used to display the URL in this mode.
  EXPECT_FALSE(browser_view()->focus_mode_title_bar_for_testing());

  // Outside of a split view the mini toolbar is hidden until Focus Mode is
  // enabled.
  ASSERT_TRUE(WaitForMiniToolbarVisible(false));

  // Focus Mode stays disabled unless the active tab is secure.
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), https_server_.GetURL("a.test", "/empty.html")));

  SetFocusModeEnabled(true);
  ASSERT_TRUE(WaitForMiniToolbarVisible(true));
  EXPECT_TRUE(DomainContains(u"a.test"));

  SetFocusModeEnabled(false);
  EXPECT_TRUE(WaitForMiniToolbarVisible(false));
}

IN_PROC_BROWSER_TEST_F(FocusModeMiniToolbarBrowserTest,
                       InsecurePageHidesMiniToolbar) {
  SetFocusModeEnabled(true);

  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), https_server_.GetURL("a.test", "/empty.html")));
  ASSERT_TRUE(WaitForMiniToolbarVisible(true));

  // Plain HTTP page ("Not Secure"): Focus Mode is suspended.
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), embedded_test_server()->GetURL("a.test", "/empty.html")));
  EXPECT_TRUE(WaitForMiniToolbarVisible(false));
}

IN_PROC_BROWSER_TEST_F(FocusModeTitleBarUrlDisplayBrowserTest,
                       MiniToolbarStaysHidden) {
  ASSERT_TRUE(browser_view()->focus_mode_title_bar_for_testing());

  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), https_server_.GetURL("a.test", "/empty.html")));

  SetFocusModeEnabled(true);
  ASSERT_TRUE(browser_view()->focus_mode_title_bar_for_testing()->GetVisible());
  EXPECT_TRUE(WaitForMiniToolbarVisible(false));
}
