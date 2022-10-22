/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/test/scoped_feature_list.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/browser/ui/views/tabs/features.h"
#include "build/build_config.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/frame/browser_non_client_frame_view.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_test.h"

#if BUILDFLAG(IS_WIN)
#include "chrome/browser/ui/views/frame/glass_browser_frame_view.h"
#endif

#if BUILDFLAG(IS_MAC)
#include "ui/views/widget/native_widget_mac.h"
#endif

#if defined(USE_AURA)
#include "chrome/browser/ui/views/frame/opaque_browser_frame_view.h"
#endif

class VerticalTabStripBrowserTest : public InProcessBrowserTest {
 public:
  VerticalTabStripBrowserTest()
      : feature_list_(tabs::features::kBraveVerticalTabs) {}

  ~VerticalTabStripBrowserTest() override = default;

  const BrowserView* browser_view() const {
    return static_cast<BrowserView*>(browser()->window());
  }
  BrowserView* browser_view() {
    return static_cast<BrowserView*>(browser()->window());
  }
  BrowserNonClientFrameView* browser_non_client_frame_view() {
    return browser_view()->frame()->GetFrameView();
  }
  const BrowserNonClientFrameView* browser_non_client_frame_view() const {
    return browser_view()->frame()->GetFrameView();
  }

  // Returns the visibility of window title is actually changed by a frame or
  // a widget. If we can't access to actual title view, returns a value which
  // window title will be synchronized to.
  bool IsWindowTitleViewVisible() const {
#if BUILDFLAG(IS_MAC)
    auto* native_widget = static_cast<const views::NativeWidgetMac*>(
        browser_view()->GetWidget()->native_widget_private());
    if (!native_widget->has_overridden_window_title_visibility()) {
      // Returns default visibility
      return browser_view()
          ->GetWidget()
          ->widget_delegate()
          ->ShouldShowWindowTitle();
    }

    return native_widget->overridden_window_title_visibility();
#elif BUILDFLAG(IS_WIN)
    if (browser_view()->GetWidget()->ShouldUseNativeFrame()) {
      return static_cast<const GlassBrowserFrameView*>(
                 browser_non_client_frame_view())
          ->window_title_for_testing()
          ->GetVisible();
    }
#endif

#if defined(USE_AURA)
    return static_cast<const OpaqueBrowserFrameView*>(
               browser_non_client_frame_view())
        ->ShouldShowWindowTitle();
#endif
  }

  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(VerticalTabStripBrowserTest, WindowTitle) {
  // Pre-condition: Window title is "visible" by default on vertical tabs
  ASSERT_TRUE(tabs::features::ShouldShowVerticalTabs());
  ASSERT_TRUE(tabs::features::ShouldShowWindowTitleForVerticalTabs(browser()));
  ASSERT_TRUE(browser_view()->ShouldShowWindowTitle());
  ASSERT_TRUE(IsWindowTitleViewVisible());

  // Hide window title bar
  brave::ToggleWindowTitleVisibilityForVerticalTabs(browser());
  browser_non_client_frame_view()->Layout();
  EXPECT_FALSE(tabs::features::ShouldShowWindowTitleForVerticalTabs(browser()));
  EXPECT_FALSE(browser_view()->ShouldShowWindowTitle());
#if !BUILDFLAG(IS_LINUX)
  // TODO(sko) For now, we can't hide window title bar entirely on Linux.
  // We're using a minimum height for it.
  EXPECT_EQ(0,
            browser_non_client_frame_view()->GetTopInset(/*restored=*/false));
#endif
  EXPECT_FALSE(IsWindowTitleViewVisible());

  // Show window title bar
  brave::ToggleWindowTitleVisibilityForVerticalTabs(browser());
  browser_non_client_frame_view()->Layout();
  EXPECT_TRUE(tabs::features::ShouldShowWindowTitleForVerticalTabs(browser()));
  EXPECT_TRUE(browser_view()->ShouldShowWindowTitle());
  EXPECT_GE(browser_non_client_frame_view()->GetTopInset(/*restored=*/false),
            0);
  EXPECT_TRUE(IsWindowTitleViewVisible());
}
