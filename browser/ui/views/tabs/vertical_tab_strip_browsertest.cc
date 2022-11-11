/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/test/scoped_feature_list.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/vertical_tab_strip_region_view.h"
#include "brave/browser/ui/views/frame/vertical_tab_strip_widget_delegate_view.h"
#include "brave/browser/ui/views/tabs/features.h"
#include "build/build_config.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/views/frame/browser_non_client_frame_view.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/tab_strip_region_view.h"
#include "chrome/browser/ui/views/tabs/new_tab_button.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_test.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/layout_manager.h"

#if BUILDFLAG(IS_WIN)
#include "chrome/browser/ui/views/frame/glass_browser_frame_view.h"
#endif

#if BUILDFLAG(IS_LINUX)
#include "chrome/common/pref_names.h"
#endif

#if BUILDFLAG(IS_MAC)
#include "ui/views/widget/native_widget_mac.h"
#endif

#if defined(USE_AURA)
#include "chrome/browser/ui/views/frame/opaque_browser_frame_view.h"
#endif

namespace {

class TallLayoutManager : public views::FlexLayout {
 public:
  using FlexLayout::FlexLayout;
  ~TallLayoutManager() override = default;

  // views::FlexLayout:
  gfx::Size GetMinimumSize(const views::View* host) const override {
    return {300, 1000};
  }
};

}  // namespace

class VerticalTabStripBrowserTest : public InProcessBrowserTest {
 public:
  VerticalTabStripBrowserTest()
      : feature_list_(tabs::features::kBraveVerticalTabs) {}

  ~VerticalTabStripBrowserTest() override = default;

  const BraveBrowserView* browser_view() const {
    return static_cast<BraveBrowserView*>(browser()->window());
  }
  BraveBrowserView* browser_view() {
    return static_cast<BraveBrowserView*>(browser()->window());
  }
  BrowserNonClientFrameView* browser_non_client_frame_view() {
    return browser_view()->frame()->GetFrameView();
  }
  const BrowserNonClientFrameView* browser_non_client_frame_view() const {
    return browser_view()->frame()->GetFrameView();
  }

  void ToggleVerticalTabStrip() {
    brave::ToggleVerticalTabStrip(browser());
    browser_non_client_frame_view()->Layout();
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

    return native_widget->GetOverriddenWindowTitleVisibility();
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

IN_PROC_BROWSER_TEST_F(VerticalTabStripBrowserTest, ToggleVerticalTabStrip) {
  // Pre-conditions
  // The default orientation is horizontal.
  ASSERT_FALSE(tabs::features::ShouldShowVerticalTabs(browser()));
  ASSERT_EQ(browser_view()->GetWidget(),
            browser_view()->tabstrip()->GetWidget());

  // Show vertical tab strip. This will move tabstrip to its own widget.
  ToggleVerticalTabStrip();
  EXPECT_TRUE(tabs::features::ShouldShowVerticalTabs(browser()));
  EXPECT_NE(browser_view()->GetWidget(),
            browser_view()->tabstrip()->GetWidget());

  // Hide vertical tab strip and restore to the horizontal tabstrip.
  ToggleVerticalTabStrip();
  EXPECT_FALSE(tabs::features::ShouldShowVerticalTabs(browser()));
  EXPECT_EQ(browser_view()->GetWidget(),
            browser_view()->tabstrip()->GetWidget());
}

IN_PROC_BROWSER_TEST_F(VerticalTabStripBrowserTest, WindowTitle) {
  ToggleVerticalTabStrip();

#if BUILDFLAG(IS_LINUX)
  browser()->profile()->GetPrefs()->SetBoolean(prefs::kUseCustomChromeFrame,
                                               true);
#endif
  // Pre-condition: Window title is "visible" by default on vertical tabs
  ASSERT_TRUE(tabs::features::ShouldShowVerticalTabs(browser()));
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

IN_PROC_BROWSER_TEST_F(VerticalTabStripBrowserTest, NewTabVisibility) {
  EXPECT_TRUE(
      browser_view()->tab_strip_region_view()->new_tab_button()->GetVisible());

  ToggleVerticalTabStrip();

  // When there are too many tabs so it overflows, the original new tab button
  // will be hidden and vertical tabstrip region view will show it's own new tab
  // button at the bottom.
  while (!browser_view()
              ->tab_strip_region_view()
              ->new_tab_button()
              ->GetVisible()) {
    chrome::AddTabAt(browser(), {}, -1, true);
  }

  // When turning on horizontal tabstrip, the original new tab button should be
  // visible.
  ToggleVerticalTabStrip();
  EXPECT_TRUE(
      browser_view()->tab_strip_region_view()->new_tab_button()->GetVisible());
}

IN_PROC_BROWSER_TEST_F(VerticalTabStripBrowserTest, MinHeight) {
  ToggleVerticalTabStrip();

  // TabStripRegionView's min height shouldn't affect that of browser window.
  const auto min_size = browser_view()->GetMinimumSize();
  auto* layout = browser_view()->tab_strip_region_view()->SetLayoutManager(
      std::make_unique<TallLayoutManager>());
  layout->SetOrientation(views::LayoutOrientation::kVertical);
  browser_view()->tab_strip_region_view()->layout_manager_ = layout;
  EXPECT_EQ(min_size.height(), browser_view()->GetMinimumSize().height());
}

IN_PROC_BROWSER_TEST_F(VerticalTabStripBrowserTest, VisualState) {
  ToggleVerticalTabStrip();

  // Pre-condition: Floating mode is enabled by default.
  using State = VerticalTabStripRegionView::State;
  ASSERT_TRUE(tabs::features::IsFloatingVerticalTabsEnabled(browser()));
  auto* widget_delegate_view =
      browser_view()->vertical_tab_strip_widget_delegate_view();
  ASSERT_TRUE(widget_delegate_view);
  auto* region_view = widget_delegate_view->vertical_tab_strip_region_view();
  ASSERT_TRUE(region_view);
  ASSERT_EQ(State::kExpanded, region_view->state());

  // Try Expanding / collapsing
  auto* prefs = browser()->profile()->GetOriginalProfile()->GetPrefs();
  prefs->SetBoolean(brave_tabs::kVerticalTabsCollapsed, true);
  EXPECT_EQ(State::kCollapsed, region_view->state());
  prefs->SetBoolean(brave_tabs::kVerticalTabsCollapsed, false);
  EXPECT_EQ(State::kExpanded, region_view->state());
  prefs->SetBoolean(brave_tabs::kVerticalTabsCollapsed, true);

  // Check if mouse hover triggers floating mode.
  {
    base::AutoReset resetter(&region_view->mouse_events_for_test_, true);
    ui::MouseEvent event(ui::ET_MOUSE_ENTERED, gfx::PointF(), gfx::PointF(), {},
                         {}, {});
    region_view->OnMouseEntered(event);
    EXPECT_EQ(State::kFloating, region_view->state());
  }

  // Check if mouse exiting make tab strip collapsed.
  {
    base::AutoReset resetter(&region_view->mouse_events_for_test_, true);
    ui::MouseEvent event(ui::ET_MOUSE_EXITED, gfx::PointF(), gfx::PointF(), {},
                         {}, {});
    region_view->OnMouseExited(event);
    EXPECT_EQ(State::kCollapsed, region_view->state());
  }

  // When floating mode is disabled, it shouldn't be triggered.
  prefs->SetBoolean(brave_tabs::kVerticalTabsFloatingEnabled, false);
  {
    base::AutoReset resetter(&region_view->mouse_events_for_test_, true);
    ui::MouseEvent event(ui::ET_MOUSE_ENTERED, gfx::PointF(), gfx::PointF(), {},
                         {}, {});
    region_view->OnMouseEntered(event);
    EXPECT_NE(State::kFloating, region_view->state());
  }
}
