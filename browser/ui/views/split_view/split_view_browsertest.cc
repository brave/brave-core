/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/split_view/split_view.h"

#include <utility>

#include "base/containers/contains.h"
#include "base/test/run_until.h"
#include "brave/browser/brave_browser_features.h"
#include "brave/browser/ui/bookmark/bookmark_helper.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/browser/ui/tabs/brave_tab_layout_constants.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/tabs/split_view_browser_data.h"
#include "brave/browser/ui/views/brave_javascript_tab_modal_dialog_view_views.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/brave_contents_view_util.h"
#include "brave/browser/ui/views/frame/split_view/brave_contents_container_view.h"
#include "brave/browser/ui/views/frame/split_view/brave_multi_contents_view.h"
#include "brave/browser/ui/views/split_view/split_view_layout_manager.h"
#include "brave/browser/ui/views/split_view/split_view_separator.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/renderer_context_menu/render_view_context_menu.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/exclusive_access/exclusive_access_manager.h"
#include "chrome/browser/ui/exclusive_access/fullscreen_controller.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/tabs/split_tab_menu_model.h"
#include "chrome/browser/ui/tabs/tab_model.h"
#include "chrome/browser/ui/ui_features.h"
#include "chrome/browser/ui/views/frame/browser_non_client_frame_view.h"
#include "chrome/browser/ui/views/frame/multi_contents_resize_area.h"
#include "chrome/browser/ui/views/frame/multi_contents_view_mini_toolbar.h"
#include "chrome/browser/ui/views/infobars/infobar_container_view.h"
#include "chrome/browser/ui/views/tabs/tab.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "chrome/browser/ui/views/tabs/tab_style_views.h"
#include "chrome/browser/ui/webui/ntp/new_tab_ui.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/grit/brave_components_strings.h"
#include "components/infobars/content/content_infobar_manager.h"
#include "components/javascript_dialogs/tab_modal_dialog_manager.h"
#include "components/permissions/permission_request_manager.h"
#include "components/tabs/public/split_tab_visual_data.h"
#include "components/tabs/public/tab_interface.h"
#include "components/web_modal/web_contents_modal_dialog_manager.h"
#include "content/public/browser/context_menu_params.h"
#include "content/public/common/javascript_dialog_type.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "third_party/skia/include/core/SkPath.h"
#include "third_party/skia/include/core/SkRegion.h"
#include "ui/compositor/layer.h"
#include "ui/events/base_event_utils.h"
#include "ui/events/event.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/skia_conversions.h"
#include "ui/views/view_utils.h"
#include "ui/views/widget/widget.h"
#include "url/origin.h"

namespace {
ui::MouseEvent GetDummyEvent() {
  return ui::MouseEvent(ui::EventType::kMousePressed, gfx::PointF(),
                        gfx::PointF(), base::TimeTicks::Now(), 0, 0);
}
}  // namespace

using SplitViewDisabledBrowserTest = InProcessBrowserTest;

// Check brave split view is disabled by default.
IN_PROC_BROWSER_TEST_F(SplitViewDisabledBrowserTest,
                       SplitViewDisabledStateTest) {
  auto* split_view_data = browser()->GetFeatures().split_view_browser_data();
  EXPECT_FALSE(!!split_view_data);
}

class SideBySideEnabledBrowserTest : public InProcessBrowserTest {
 public:
  SideBySideEnabledBrowserTest() {
    scoped_features_.InitWithFeatures(
        /*enabled_features*/ {},
        /*disabled_features*/ {features::kBraveWebViewRoundedCorners});
  }
  ~SideBySideEnabledBrowserTest() override = default;

  TabStrip* tab_strip() {
    return BrowserView::GetBrowserViewForBrowser(browser())->tabstrip();
  }

  BraveBrowserView* brave_browser_view() const {
    return BraveBrowserView::From(
        BrowserView::GetBrowserViewForBrowser(browser()));
  }

  views::View* split_view_separator() const {
    return brave_multi_contents_view()->resize_area_for_testing();
  }

  BraveMultiContentsView* brave_multi_contents_view() const {
    return static_cast<BraveMultiContentsView*>(
        brave_browser_view()->multi_contents_view());
  }

  BrowserNonClientFrameView* browser_non_client_frame_view() {
    return brave_browser_view()->frame()->GetFrameView();
  }

  void ToggleVerticalTabStrip() {
    brave::ToggleVerticalTabStrip(browser());
    browser_non_client_frame_view()->DeprecatedLayoutImmediately();
  }

  javascript_dialogs::TabModalDialogManager* GetTabModalDialogManagerAt(
      int index) {
    return javascript_dialogs::TabModalDialogManager::FromWebContents(
        GetWebContentsAt(index));
  }

  web_modal::WebContentsModalDialogManager* GetWebModalDialogManagerAt(
      int index) {
    return web_modal::WebContentsModalDialogManager::FromWebContents(
        GetWebContentsAt(index));
  }

  content::WebContents* GetWebContentsAt(int index) {
    return browser()->tab_strip_model()->GetWebContentsAt(index);
  }

 protected:
  base::test::ScopedFeatureList scoped_features_;
};

IN_PROC_BROWSER_TEST_F(SideBySideEnabledBrowserTest,
                       BraveMultiContentsViewTest) {
  // Check SplitView feature is not enabled.
  EXPECT_FALSE(tabs::features::IsBraveSplitViewEnabled());
  auto* split_view_data = browser()->GetFeatures().split_view_browser_data();
  EXPECT_FALSE(!!split_view_data);

  // Check MultiContentsView uses our separator and initially hidden.
  EXPECT_FALSE(split_view_separator()->GetVisible());

  auto* browser_view = brave_browser_view();

  // Remove all infobars to test top container separator visibility.
  // Infobar visibility affects that separator visibility.
  // Start test w/o infobar.
  infobars::ContentInfoBarManager::FromWebContents(GetWebContentsAt(0))
      ->RemoveAllInfoBars(/*animate=*/false);
  browser_view->InvalidateLayout();
  ASSERT_TRUE(base::test::RunUntil(
      [&]() { return !browser_view->infobar_container()->GetVisible(); }));

  // separator should not be empty and visible when split view is closed.
  EXPECT_TRUE(
      browser_view->top_container_separator_for_testing()->GetVisible());
  EXPECT_NE(
      gfx::Size(),
      browser_view->top_container_separator_for_testing()->GetPreferredSize());

  chrome::NewSplitTab(browser(),
                      split_tabs::SplitTabCreatedSource::kToolbarButton);

  // separator should be empty when split view is opened.
  EXPECT_EQ(
      gfx::Size(),
      browser_view->top_container_separator_for_testing()->GetPreferredSize());
  EXPECT_TRUE(split_view_separator()->GetVisible());
  EXPECT_EQ(4, split_view_separator()->GetPreferredSize().width());

  // Check corner radius.
  auto* multi_contents_view = brave_multi_contents_view();
  ASSERT_TRUE(multi_contents_view);
  auto* start_contents_container_view =
      static_cast<BraveContentsContainerView*>(
          multi_contents_view->contents_container_views_for_testing()[0]);
  auto* end_contents_container_view = static_cast<BraveContentsContainerView*>(
      multi_contents_view->contents_container_views_for_testing()[1]);
  ASSERT_TRUE(start_contents_container_view);
  ASSERT_TRUE(end_contents_container_view);

  FullscreenController* fullscreen_controller = browser()
                                                    ->GetFeatures()
                                                    .exclusive_access_manager()
                                                    ->fullscreen_controller();
  fullscreen_controller->set_is_tab_fullscreen_for_testing(true);
  EXPECT_EQ(0, start_contents_container_view->GetCornerRadius(true));
  fullscreen_controller->set_is_tab_fullscreen_for_testing(false);

  auto* start_contents_web_view =
      multi_contents_view->start_contents_view_for_testing();
  auto* end_contents_web_view =
      multi_contents_view->end_contents_view_for_testing();
  ASSERT_TRUE(start_contents_web_view);
  ASSERT_TRUE(end_contents_web_view);
  EXPECT_EQ(start_contents_web_view->layer()->rounded_corner_radii(),
            gfx::RoundedCornersF(
                start_contents_container_view->GetCornerRadius(false)));
  EXPECT_EQ(end_contents_web_view->layer()->rounded_corner_radii(),
            gfx::RoundedCornersF(
                end_contents_container_view->GetCornerRadius(false)));

  // Check borders.
  EXPECT_EQ(gfx::Insets(BraveContentsContainerView::kBorderThickness),
            start_contents_container_view->GetBorder()->GetInsets());
  EXPECT_EQ(gfx::Insets(BraveContentsContainerView::kBorderThickness),
            end_contents_container_view->GetBorder()->GetInsets());

  ASSERT_TRUE(base::test::RunUntil([&]() {
    return start_contents_web_view->width() == end_contents_web_view->width();
  }));

  multi_contents_view->OnResize(30, false);
  multi_contents_view->OnResize(30, true);

  ASSERT_TRUE(base::test::RunUntil([&]() {
    return start_contents_web_view->width() != end_contents_web_view->width();
  }));

  // Check double click makes both contents view have same width.
  const gfx::Point point(0, 0);
  ui::MouseEvent event(ui::EventType::kMousePressed, point, point,
                       ui::EventTimeForNow(), ui::EF_LEFT_MOUSE_BUTTON,
                       ui::EF_LEFT_MOUSE_BUTTON);
  event.SetClickCount(2);
  split_view_separator()->OnMouseReleased(event);
  ASSERT_TRUE(base::test::RunUntil([&]() {
    return start_contents_web_view->width() == end_contents_web_view->width();
  }));
}

IN_PROC_BROWSER_TEST_F(SideBySideEnabledBrowserTest, SelectTabTest) {
  chrome::AddTabAt(browser(), GURL(), -1, /*foreground*/ true);
  chrome::AddTabAt(browser(), GURL(), -1, /*foreground*/ true);
  EXPECT_EQ(2, tab_strip()->GetActiveIndex());

  EXPECT_FALSE(split_view_separator()->GetVisible());

  // Created new tab(at 3) for new split view with existing tab(at 2).
  chrome::NewSplitTab(browser(),
                      split_tabs::SplitTabCreatedSource::kToolbarButton);
  EXPECT_TRUE(tab_strip()->tab_at(2)->split().has_value());
  EXPECT_FALSE(tab_strip()->tab_at(2)->IsActive());
  EXPECT_TRUE(tab_strip()->tab_at(3)->split().has_value());
  EXPECT_TRUE(tab_strip()->tab_at(3)->IsActive());
  EXPECT_TRUE(split_view_separator()->GetVisible());

  // Chromium's mini toolbar should be visible.
  EXPECT_TRUE(
      brave_multi_contents_view()->mini_toolbar_for_testing(0)->GetVisible());
  EXPECT_TRUE(
      brave_multi_contents_view()->mini_toolbar_for_testing(1)->GetVisible());

  // Check mini toolbar uses our menu model.
  brave_multi_contents_view()->mini_toolbar_for_testing(0)->OpenSplitViewMenu();
  auto* menu_model =
      static_cast<SplitTabMenuModel*>(brave_multi_contents_view()
                                          ->mini_toolbar_for_testing(0)
                                          ->menu_model_.get());

  // This id calc is copied from GetCommandIdInt() at split_tab_menu_model.cc
  // Check that method if test failed.
  int command_id =
      ExistingBaseSubMenuModel::kMinSplitTabMenuModelCommandId +
      static_cast<int>(SplitTabMenuModel::CommandId::kReversePosition);
  EXPECT_EQ(l10n_util::GetStringUTF16(IDS_IDC_SWAP_SPLIT_VIEW),
            menu_model->GetLabelForCommandId(command_id));

  // Activate non split view tab.
  tab_strip()->SelectTab(tab_strip()->tab_at(0), GetDummyEvent());
  EXPECT_EQ(0, tab_strip()->GetActiveIndex());
  EXPECT_FALSE(split_view_separator()->GetVisible());

  // Check only hovered split tab has hover animation.
  auto* hovered_split_tab = tab_strip()->tab_at(2);
  auto* not_hovered_split_tab = tab_strip()->tab_at(3);
  EXPECT_TRUE(hovered_split_tab->split().has_value());
  EXPECT_EQ(hovered_split_tab->split(), not_hovered_split_tab->split());
  hovered_split_tab->controller()->ShowHover(hovered_split_tab,
                                             TabStyle::ShowHoverStyle::kSubtle);
  ASSERT_TRUE(base::test::RunUntil([&]() {
    return hovered_split_tab->tab_style_views()->GetHoverAnimationValue() != 0;
  }));
  EXPECT_EQ(not_hovered_split_tab->tab_style_views()->GetHoverAnimationValue(),
            0);

  // Check selected split tab becomes active tab.
  tab_strip()->SelectTab(tab_strip()->tab_at(2), GetDummyEvent());
  EXPECT_EQ(2, tab_strip()->GetActiveIndex());
  EXPECT_TRUE(tab_strip()->tab_at(2)->IsActive());
  EXPECT_FALSE(tab_strip()->tab_at(3)->IsActive());
  EXPECT_TRUE(split_view_separator()->GetVisible());

  tab_strip()->SelectTab(tab_strip()->tab_at(0), GetDummyEvent());
  EXPECT_EQ(0, tab_strip()->GetActiveIndex());

  tab_strip()->SelectTab(tab_strip()->tab_at(3), GetDummyEvent());
  EXPECT_EQ(3, tab_strip()->GetActiveIndex());
  EXPECT_FALSE(tab_strip()->tab_at(2)->IsActive());
  EXPECT_TRUE(tab_strip()->tab_at(3)->IsActive());

  // Flaky with dialog test on macOS.
#if !BUILDFLAG(IS_MAC)
  // Activate split tab at 2
  tab_strip()->SelectTab(tab_strip()->tab_at(2), GetDummyEvent());
  EXPECT_EQ(2, tab_strip()->GetActiveIndex());

  // Check activated split tab is the the that owned tab modal.
  // Launch dialog from active split tab (at 2).
  bool did_suppress = false;
  GetTabModalDialogManagerAt(2)->RunJavaScriptDialog(
      GetWebContentsAt(2), GetWebContentsAt(2)->GetPrimaryMainFrame(),
      content::JAVASCRIPT_DIALOG_TYPE_ALERT, std::u16string(), std::u16string(),
      base::BindOnce([](bool, const std::u16string&) {}), &did_suppress);

  EXPECT_TRUE(GetTabModalDialogManagerAt(2)->IsShowingDialogForTesting());
  EXPECT_TRUE(GetWebModalDialogManagerAt(2)->IsDialogActive());

  // Activate non split tab at 0.
  tab_strip()->SelectTab(tab_strip()->tab_at(0), GetDummyEvent());
  EXPECT_EQ(0, tab_strip()->GetActiveIndex());

  // Activate split tab that doesn't have tab modal.
  // Check tab at 2 is activated because only a split tab that owns
  // tab modal can be activated till that modal is dismissed.
  tab_strip()->SelectTab(tab_strip()->tab_at(3), GetDummyEvent());
  EXPECT_EQ(2, tab_strip()->GetActiveIndex());
#endif
}

class SideBySideWithRoundedCornersTest : public SideBySideEnabledBrowserTest {
 public:
  SideBySideWithRoundedCornersTest() {
    // Reset to have different features config with base test class.
    scoped_features_.Reset();
    scoped_features_.InitWithFeatures(
        /*enabled_features*/ {features::kBraveWebViewRoundedCorners}, {});
  }
  ~SideBySideWithRoundedCornersTest() override = default;
};

IN_PROC_BROWSER_TEST_F(SideBySideWithRoundedCornersTest, ContentsShadowTest) {
  // Shadow if split tab is not active.
  EXPECT_TRUE(brave_browser_view()->contents_shadow_);

  chrome::NewSplitTab(browser(),
                      split_tabs::SplitTabCreatedSource::kToolbarButton);

  // No shadow if split tab is active.
  EXPECT_FALSE(brave_browser_view()->contents_shadow_);

  // Shadow if split tab is not active.
  chrome::AddTabAt(browser(), GURL(), -1, /*foreground*/ true);
  EXPECT_TRUE(brave_browser_view()->contents_shadow_);
}

// Test multi contents view's rounded corners with fullscreen state w/o split
// view.
IN_PROC_BROWSER_TEST_F(SideBySideWithRoundedCornersTest,
                       TabFullscreenStateTest) {
  auto* contents_container = brave_browser_view()->contents_container();
  auto* contents_view = brave_browser_view()->GetContentsView();

  // Check it has rounded corners.
  EXPECT_EQ(contents_container->layer()->rounded_corner_radii(),
            gfx::RoundedCornersF(BraveContentsViewUtil::kBorderRadius));
  EXPECT_EQ(contents_view->layer()->rounded_corner_radii(),
            gfx::RoundedCornersF(BraveContentsViewUtil::kBorderRadius));

  FullscreenController* fullscreen_controller = browser()
                                                    ->GetFeatures()
                                                    .exclusive_access_manager()
                                                    ->fullscreen_controller();

  // Check rounded corners are cleared in tab fullscreen.
  fullscreen_controller->set_is_tab_fullscreen_for_testing(true);
  brave_browser_view()->UpdateWebViewRoundedCorners();
  EXPECT_EQ(contents_container->layer()->rounded_corner_radii(),
            gfx::RoundedCornersF());
  EXPECT_EQ(contents_view->layer()->rounded_corner_radii(),
            gfx::RoundedCornersF());

  // Check it has rounded corners again.
  fullscreen_controller->set_is_tab_fullscreen_for_testing(false);
  brave_browser_view()->UpdateWebViewRoundedCorners();
  EXPECT_EQ(contents_container->layer()->rounded_corner_radii(),
            gfx::RoundedCornersF(BraveContentsViewUtil::kBorderRadius));
  EXPECT_EQ(contents_view->layer()->rounded_corner_radii(),
            gfx::RoundedCornersF(BraveContentsViewUtil::kBorderRadius));
}

// Use for testing brave split view and SideBySide together.
class SplitViewCommonBrowserTest : public InProcessBrowserTest,
                                   public testing::WithParamInterface<bool> {
 public:
  SplitViewCommonBrowserTest() {
    if (!IsSideBySideEnabled()) {
      scoped_features_.InitWithFeatures(
          /*enabled_features*/ {tabs::features::kBraveSplitView},
          /*disabled_features*/ {features::kSideBySide});
    }
  }
  ~SplitViewCommonBrowserTest() override = default;

  bool GetIsTabHiddenFromPermissionManagerFromTabAt(int index) {
    auto* tab_strip_model = browser()->tab_strip_model();
    return !permissions::PermissionRequestManager::FromWebContents(
                tab_strip_model->GetWebContentsAt(index))
                ->tab_is_active_for_testing();
  }

  // blocked(true) when tab at |index| has tab modal dialog.
  bool GetIsWebContentsBlockedFromTabAt(int index) {
    auto* tab_strip_model = browser()->tab_strip_model();
    return static_cast<tabs::TabModel*>(tab_strip_model->GetTabAtIndex(index))
        ->blocked();
  }

  web_modal::WebContentsModalDialogManager* GetWebModalDialogManagerAt(
      int index) {
    return web_modal::WebContentsModalDialogManager::FromWebContents(
        browser()->tab_strip_model()->GetWebContentsAt(index));
  }

  bool HasWebModalDialogAt(int index) {
    return !GetWebModalDialogManagerAt(index)->child_dialogs_.empty();
  }

  bool IsWebModalDialogVisibleAt(int index) {
    return views::Widget::GetWidgetForNativeWindow(
               GetWebModalDialogManagerAt(index)
                   ->child_dialogs_.front()
                   .manager->dialog())
        ->IsVisible();
  }

  javascript_dialogs::TabModalDialogManager* GetTabModalDialogManagerAt(
      int index) {
    return javascript_dialogs::TabModalDialogManager::FromWebContents(
        browser()->tab_strip_model()->GetWebContentsAt(index));
  }

  content::WebContents* GetWebContentsAt(int index) {
    return browser()->tab_strip_model()->GetWebContentsAt(index);
  }

  void NewSplitTab() {
    IsSideBySideEnabled()
        ? chrome::NewSplitTab(browser(),
                              split_tabs::SplitTabCreatedSource::kToolbarButton)
        : brave::NewSplitViewForTab(browser());
  }

  bool IsSideBySideEnabled() const { return GetParam(); }

  bool IsSplitTabAt(int index) {
    return IsSplitWebContents(GetWebContentsAt(index));
  }

  void SwapActiveSplitTab() {
    if (IsSideBySideEnabled()) {
      auto* tab_strip_model = browser()->tab_strip_model();
      auto split_id =
          tab_strip_model->GetSplitForTab(tab_strip_model->active_index());
      ASSERT_TRUE(split_id);
      tab_strip_model->ReverseTabsInSplit(split_id.value());
    } else {
      brave::SwapTabsInTile(browser());
    }
  }

  bool IsSplitWebContents(content::WebContents* web_contents) {
    auto tab_handle =
        tabs::TabInterface::GetFromContents(web_contents)->GetHandle();
    if (IsSideBySideEnabled()) {
      return tab_handle.Get()->IsSplit();
    }

    auto* split_view_browser_data =
        browser()->GetFeatures().split_view_browser_data();
    CHECK(split_view_browser_data);
    return split_view_browser_data->GetTile(tab_handle).has_value();
  }

  ContentsWebView* GetContentsWebView() {
    return BrowserView::GetBrowserViewForBrowser(browser())
        ->contents_web_view();
  }

  TabStrip* tab_strip() {
    return BrowserView::GetBrowserViewForBrowser(browser())->tabstrip();
  }

 private:
  base::test::ScopedFeatureList scoped_features_;
};

IN_PROC_BROWSER_TEST_P(SplitViewCommonBrowserTest, SplitTabInsetsTest) {
  brave::ToggleVerticalTabStrip(browser());

  auto* tab_strip_model = browser()->tab_strip_model();
  tab_strip_model->SetTabPinned(0, true);
  chrome::AddTabAt(browser(), GURL(), -1, /*foreground*/ true);
  chrome::AddTabAt(browser(), GURL(), -1, /*foreground*/ true);
  NewSplitTab();
  EXPECT_EQ(3, tab_strip_model->active_index());
  EXPECT_FALSE(IsSplitTabAt(0));
  EXPECT_FALSE(IsSplitTabAt(1));
  EXPECT_TRUE(IsSplitTabAt(2));
  EXPECT_TRUE(IsSplitTabAt(3));
  EXPECT_TRUE(tab_strip_model->IsTabPinned(0));
  EXPECT_FALSE(tab_strip_model->IsTabPinned(1));
  EXPECT_FALSE(tab_strip_model->IsTabPinned(2));
  EXPECT_FALSE(tab_strip_model->IsTabPinned(3));

  // Get normal tab's border insets.
  const auto insets = tab_strip()->tab_at(1)->GetBorder()->GetInsets();

  // Check split tab's first & second tabs' insets are different.
  // value 4 here is copied from |kPaddingForVerticalTabInTile| in
  // brave_tab_style_views.inc.cc.
  EXPECT_EQ(tab_strip()->tab_at(2)->GetBorder()->GetInsets(),
            insets + gfx::Insets::TLBR(4, 0, 0, 0));
  EXPECT_EQ(tab_strip()->tab_at(3)->GetBorder()->GetInsets(),
            insets + gfx::Insets::TLBR(0, 0, 4, 0));

  SwapActiveSplitTab();
  EXPECT_EQ(2, tab_strip()->GetActiveIndex());

  // Check split tabs have proper insets after swap
  EXPECT_EQ(tab_strip()->tab_at(2)->GetBorder()->GetInsets(),
            insets + gfx::Insets::TLBR(4, 0, 0, 0));
  EXPECT_EQ(tab_strip()->tab_at(3)->GetBorder()->GetInsets(),
            insets + gfx::Insets::TLBR(0, 0, 4, 0));

  // Check pinned split tabs have same insets with other pinned tab
  chrome::PinTab(browser());
  ASSERT_TRUE(base::test::RunUntil([&]() {
    return tab_strip_model->IsTabPinned(1) && tab_strip_model->IsTabPinned(2);
  }));

  EXPECT_FALSE(IsSplitTabAt(0));
  EXPECT_TRUE(IsSplitTabAt(1));
  EXPECT_TRUE(IsSplitTabAt(2));
  EXPECT_FALSE(IsSplitTabAt(3));
  EXPECT_TRUE(tab_strip_model->IsTabPinned(0));
  EXPECT_TRUE(tab_strip_model->IsTabPinned(1));
  EXPECT_TRUE(tab_strip_model->IsTabPinned(2));
  EXPECT_FALSE(tab_strip_model->IsTabPinned(3));

  EXPECT_EQ(tab_strip()->tab_at(0)->GetBorder()->GetInsets(),
            tab_strip()->tab_at(1)->GetBorder()->GetInsets());
  EXPECT_EQ(tab_strip()->tab_at(0)->GetBorder()->GetInsets(),
            tab_strip()->tab_at(2)->GetBorder()->GetInsets());

  tab_strip_model->ActivateTabAt(3);
  NewSplitTab();
  EXPECT_TRUE(IsSplitTabAt(3));
  EXPECT_TRUE(IsSplitTabAt(4));

  // vertical tab off.
  brave::ToggleVerticalTabStrip(browser());

  chrome::AddTabAt(browser(), GURL(), -1, /*foreground*/ true);
  EXPECT_EQ(5, tab_strip()->GetActiveIndex());
  NewSplitTab();
  EXPECT_TRUE(IsSplitTabAt(5));
  EXPECT_TRUE(IsSplitTabAt(6));

  // Check split tabs(at 3, 4) created at vertical tab and split tabs(5, 6)
  // created at horizontal tab have same insets.
  EXPECT_EQ(tab_strip()->tab_at(3)->GetBorder()->GetInsets(),
            tab_strip()->tab_at(5)->GetBorder()->GetInsets());
  EXPECT_EQ(tab_strip()->tab_at(4)->GetBorder()->GetInsets(),
            tab_strip()->tab_at(6)->GetBorder()->GetInsets());
}

// Check split view works with pinned tabs.
IN_PROC_BROWSER_TEST_P(SplitViewCommonBrowserTest, SplitViewWithPinnedTabTest) {
  auto* tab_strip_model = browser()->tab_strip_model();
  tab_strip_model->SetTabPinned(0, true);
  chrome::AddTabAt(browser(), GURL(), -1, /*foreground*/ true);
  EXPECT_EQ(1, tab_strip_model->active_index());
  NewSplitTab();
  EXPECT_EQ(2, tab_strip_model->active_index());

  brave::ToggleVerticalTabStrip(browser());
  chrome::AddTabAt(browser(), GURL(), -1, /*foreground*/ true);
  EXPECT_EQ(3, tab_strip_model->active_index());
  NewSplitTab();
  EXPECT_EQ(4, tab_strip_model->active_index());
  EXPECT_TRUE(IsSplitTabAt(4));

  // Pin active tab(split tab at 4).
  chrome::PinTab(browser());
}

IN_PROC_BROWSER_TEST_P(SplitViewCommonBrowserTest, BookmarksBarVisibilityTest) {
  auto* prefs = browser()->profile()->GetPrefs();
  auto* tab_strip_model = browser()->tab_strip_model();
  NewSplitTab();

  // Check no bookmarks when any split tab is activated.
  brave::SetBookmarkState(brave::BookmarkBarState::kNever, prefs);
  ASSERT_TRUE(IsSplitWebContents(GetWebContentsAt(0)));
  ASSERT_TRUE(IsSplitWebContents(GetWebContentsAt(1)));

  // Wait newly created tab to get its valid url via GetLastCommittedURL().
  EXPECT_TRUE(content::WaitForLoadStop(GetWebContentsAt(1)));
  EXPECT_FALSE(NewTabUI::IsNewTab(GetWebContentsAt(0)->GetLastCommittedURL()));
  EXPECT_TRUE(NewTabUI::IsNewTab(GetWebContentsAt(1)->GetLastCommittedURL()));
  EXPECT_EQ(1, tab_strip_model->active_index());
  EXPECT_EQ(BookmarkBar::HIDDEN,
            BookmarkBarController::From(browser())->bookmark_bar_state());
  tab_strip_model->ActivateTabAt(0);
  EXPECT_EQ(0, tab_strip_model->active_index());
  EXPECT_EQ(BookmarkBar::HIDDEN,
            BookmarkBarController::From(browser())->bookmark_bar_state());

  // With SideBySide, bookmarks bar is shown always if one of split tab is NTP.
  // Otherwise, it's shown only when active split tab is NTP.
  brave::SetBookmarkState(brave::BookmarkBarState::kNtp, prefs);
  EXPECT_EQ(IsSideBySideEnabled() ? BookmarkBar::SHOW : BookmarkBar::HIDDEN,
            BookmarkBarController::From(browser())->bookmark_bar_state());
  tab_strip_model->ActivateTabAt(1);
  EXPECT_EQ(BookmarkBar::SHOW,
            BookmarkBarController::From(browser())->bookmark_bar_state());

  // Check bookmarks is shown always.
  brave::SetBookmarkState(brave::BookmarkBarState::kAlways, prefs);
  EXPECT_EQ(BookmarkBar::SHOW,
            BookmarkBarController::From(browser())->bookmark_bar_state());
  tab_strip_model->ActivateTabAt(0);
  EXPECT_EQ(BookmarkBar::SHOW,
            BookmarkBarController::From(browser())->bookmark_bar_state());

// Upstream's window fullscreen test is disabled on macOS.
// See the comment of SideBySideBrowserTest at browser_browsertest.cc.
#if !BUILDFLAG(IS_MAC)
  ui_test_utils::ToggleFullscreenModeAndWait(browser());
  EXPECT_TRUE(browser()->window()->IsFullscreen());

  // Same reason with above for having different result with SideBySide
  // enabled state.
  EXPECT_EQ(IsSideBySideEnabled() ? BookmarkBar::SHOW : BookmarkBar::HIDDEN,
            BookmarkBarController::From(browser())->bookmark_bar_state());

  tab_strip_model->ActivateTabAt(1);
  EXPECT_EQ(BookmarkBar::SHOW,
            BookmarkBarController::From(browser())->bookmark_bar_state());
#endif
}

IN_PROC_BROWSER_TEST_P(
    SplitViewCommonBrowserTest,
    JavascriptTabModalDialogView_DialogShouldBeCenteredToRelatedWebView) {
  NewSplitTab();
  auto* active_contents = chrome_test_utils::GetActiveWebContents(this);
  ASSERT_TRUE(IsSplitWebContents(active_contents));
  auto* dialog = new BraveJavaScriptTabModalDialogViewViews(
      active_contents, active_contents, u"title",
      content::JAVASCRIPT_DIALOG_TYPE_ALERT, u"message", u"default prompt",
      base::DoNothing(), base::DoNothing());
  ASSERT_TRUE(dialog);
  auto* widget = dialog->GetWidget();
  ASSERT_TRUE(widget);

#if BUILDFLAG(IS_MAC)
  ASSERT_TRUE(base::test::RunUntil([&]() {
    const auto dialog_bounds = widget->GetWindowBoundsInScreen();
    auto web_view_bounds = GetContentsWebView()->GetLocalBounds();
    views::View::ConvertRectToScreen(GetContentsWebView(), &web_view_bounds);
    return web_view_bounds.CenterPoint().x() == dialog_bounds.CenterPoint().x();
  }));
#else
  // On macOS, this check is flaky. It seems widget position is not updated
  // immediately. So, used loop like above on macOS.
  // Why not using above checking in loop on all platform?
  // Above checking in loop causes another weird |Widget::native_widget_|
  // invalidation in the loop on other platforms(win/linux). Not sure why.
  // Fortunately, below checking works well. So, testing differently on macOS
  // and others.
  const auto dialog_bounds = widget->GetWindowBoundsInScreen();
  auto web_view_bounds = GetContentsWebView()->GetLocalBounds();
  views::View::ConvertRectToScreen(GetContentsWebView(), &web_view_bounds);
  EXPECT_EQ(web_view_bounds.CenterPoint().x(), dialog_bounds.CenterPoint().x());
#endif
}

// This test can be flaky depending on the screen size. Our macOS CI doesn't
// seem to have a large enough screen to run this test.
#if BUILDFLAG(IS_MAC)
#define MAYBE_JavascriptTabModalDialogView_DialogShouldBeCenteredToRelatedWebView_InVerticalTab \
  DISABLED_JavascriptTabModalDialogView_DialogShouldBeCenteredToRelatedWebView_InVerticalTab
#else
#define MAYBE_JavascriptTabModalDialogView_DialogShouldBeCenteredToRelatedWebView_InVerticalTab \
  JavascriptTabModalDialogView_DialogShouldBeCenteredToRelatedWebView_InVerticalTab
#endif

IN_PROC_BROWSER_TEST_P(
    SplitViewCommonBrowserTest,
    MAYBE_JavascriptTabModalDialogView_DialogShouldBeCenteredToRelatedWebView_InVerticalTab) {
  brave::ToggleVerticalTabStrip(browser());
  NewSplitTab();
  auto* active_contents = chrome_test_utils::GetActiveWebContents(this);
  ASSERT_TRUE(IsSplitWebContents(active_contents));

  auto* dialog = new BraveJavaScriptTabModalDialogViewViews(
      active_contents, active_contents, u"title",
      content::JAVASCRIPT_DIALOG_TYPE_ALERT, u"message", u"default prompt",
      base::DoNothing(), base::DoNothing());
  ASSERT_TRUE(dialog);
  auto* widget = dialog->GetWidget();
  ASSERT_TRUE(widget);

  const auto dialog_bounds = widget->GetWindowBoundsInScreen();

  auto web_view_bounds = GetContentsWebView()->GetLocalBounds();
  views::View::ConvertRectToScreen(GetContentsWebView(), &web_view_bounds);

  EXPECT_EQ(web_view_bounds.CenterPoint().x(), dialog_bounds.CenterPoint().x());
}

IN_PROC_BROWSER_TEST_P(SplitViewCommonBrowserTest, InactiveSplitTabTest) {
  NewSplitTab();
  auto* tab_strip_model = browser()->tab_strip_model();

  tab_strip_model->ActivateTabAt(1);
  EXPECT_TRUE(tab_strip_model->GetTabAtIndex(1)->IsActivated());
  EXPECT_TRUE(GetIsTabHiddenFromPermissionManagerFromTabAt(0));

  // Final state is arrived in async sometimes.
  ASSERT_TRUE(base::test::RunUntil(
      [&]() { return !GetIsTabHiddenFromPermissionManagerFromTabAt(1); }));

  tab_strip_model->ActivateTabAt(0);
  EXPECT_TRUE(tab_strip_model->GetTabAtIndex(0)->IsActivated());
  ASSERT_TRUE(base::test::RunUntil(
      [&]() { return !GetIsTabHiddenFromPermissionManagerFromTabAt(0); }));
  EXPECT_TRUE(GetIsTabHiddenFromPermissionManagerFromTabAt(1));

  chrome::AddTabAt(browser(), GURL(), -1, /*foreground*/ true);
  EXPECT_TRUE(tab_strip_model->GetTabAtIndex(2)->IsActivated());
  EXPECT_TRUE(GetIsTabHiddenFromPermissionManagerFromTabAt(0));
  EXPECT_TRUE(GetIsTabHiddenFromPermissionManagerFromTabAt(1));
  EXPECT_FALSE(GetIsTabHiddenFromPermissionManagerFromTabAt(2));

  tab_strip_model->ActivateTabAt(1);
  EXPECT_TRUE(tab_strip_model->GetTabAtIndex(1)->IsActivated());
  EXPECT_TRUE(GetIsTabHiddenFromPermissionManagerFromTabAt(0));
  ASSERT_TRUE(base::test::RunUntil(
      [&]() { return !GetIsTabHiddenFromPermissionManagerFromTabAt(1); }));
  EXPECT_TRUE(GetIsTabHiddenFromPermissionManagerFromTabAt(2));

  // Check proper state is set after restored.
  browser()->window()->Minimize();
  browser()->window()->Restore();
  EXPECT_TRUE(GetIsTabHiddenFromPermissionManagerFromTabAt(0));
  ASSERT_TRUE(base::test::RunUntil(
      [&]() { return !GetIsTabHiddenFromPermissionManagerFromTabAt(1); }));
  EXPECT_TRUE(GetIsTabHiddenFromPermissionManagerFromTabAt(2));

  EXPECT_TRUE(tab_strip_model->GetTabAtIndex(1)->IsActivated());
  EXPECT_FALSE(GetIsWebContentsBlockedFromTabAt(0));
  EXPECT_FALSE(GetIsWebContentsBlockedFromTabAt(1));

  // Launch dialog from inactive split tab (at 0).
  bool did_suppress = false;
  GetTabModalDialogManagerAt(0)->RunJavaScriptDialog(
      GetWebContentsAt(0), GetWebContentsAt(0)->GetPrimaryMainFrame(),
      content::JAVASCRIPT_DIALOG_TYPE_ALERT, std::u16string(), std::u16string(),
      base::BindOnce([](bool, const std::u16string&) {}), &did_suppress);

  if (!IsSideBySideEnabled()) {
    // false because tab modal manager not yet launched dialog as tab is hidden.
    EXPECT_FALSE(GetTabModalDialogManagerAt(0)->IsShowingDialogForTesting());
    EXPECT_FALSE(GetWebModalDialogManagerAt(0)->IsDialogActive());
    EXPECT_FALSE(GetIsWebContentsBlockedFromTabAt(0));

    // Activate split tab at 0.
    tab_strip_model->ActivateTabAt(0);
    ASSERT_TRUE(base::test::RunUntil([&]() { return HasWebModalDialogAt(0); }));

    // true because tab/web modal manager launched dialog as tab is activated.
    // check modal dialog at tab 0 is visible.
    ASSERT_TRUE(
        base::test::RunUntil([&]() { return IsWebModalDialogVisibleAt(0); }));
  } else {
    // True because tab modal manager will active the tab when showing a dialog.
    EXPECT_EQ(0, tab_strip_model->active_index());
    EXPECT_TRUE(GetTabModalDialogManagerAt(0)->IsShowingDialogForTesting());
    EXPECT_TRUE(GetWebModalDialogManagerAt(0)->IsDialogActive());
    EXPECT_TRUE(GetIsWebContentsBlockedFromTabAt(0));
  }

  // Activate split tab at 1.
  tab_strip_model->ActivateTabAt(1);

  if (!IsSideBySideEnabled()) {
    // Check modal dialog at tab 0 is hidden.
    EXPECT_EQ(1, tab_strip_model->active_index());
    ASSERT_TRUE(
        base::test::RunUntil([&]() { return !IsWebModalDialogVisibleAt(0); }));
  } else {
    // In SideBySide, active tab is still tab at 0 because it's not allowed to
    // activate another split tab when curren split tab has dialog.
    EXPECT_EQ(0, tab_strip_model->active_index());
  }

  // still true as modal was created.
  EXPECT_TRUE(GetTabModalDialogManagerAt(0)->IsShowingDialogForTesting());
  EXPECT_TRUE(GetWebModalDialogManagerAt(0)->IsDialogActive());
  EXPECT_TRUE(GetIsWebContentsBlockedFromTabAt(0));

  // tab at 1 doesn't have any modal dialog.
  EXPECT_FALSE(GetTabModalDialogManagerAt(1)->IsShowingDialogForTesting());

  // Launch dialog from active split tab (at 1) and check dialog is shown
  // immediately.
  GetTabModalDialogManagerAt(1)->RunJavaScriptDialog(
      GetWebContentsAt(1), GetWebContentsAt(1)->GetPrimaryMainFrame(),
      content::JAVASCRIPT_DIALOG_TYPE_ALERT, std::u16string(), std::u16string(),
      base::BindOnce([](bool, const std::u16string&) {}), &did_suppress);
  ASSERT_TRUE(base::test::RunUntil([&]() { return HasWebModalDialogAt(1); }));
  ASSERT_TRUE(
      base::test::RunUntil([&]() { return IsWebModalDialogVisibleAt(1); }));
}

namespace {

class LoadObserver : public content::WebContentsObserver {
 public:
  explicit LoadObserver(content::WebContents* web_contents)
      : WebContentsObserver(web_contents) {}

  void DidStopLoading() override { did_load_ = true; }

  bool did_load() const { return did_load_; }

 private:
  bool did_load_ = false;
};

}  // namespace

IN_PROC_BROWSER_TEST_P(SplitViewCommonBrowserTest, SplitViewReloadTest) {
  NewSplitTab();
  content::WaitForLoadStop(GetWebContentsAt(0));
  content::WaitForLoadStop(GetWebContentsAt(1));

  auto* tab_strip_model = browser()->tab_strip_model();
  EXPECT_EQ(1, tab_strip_model->active_index());
  EXPECT_EQ(2, tab_strip_model->count());
  EXPECT_TRUE(IsSplitTabAt(0));
  EXPECT_TRUE(IsSplitTabAt(1));

  // Check only active split tab(at 1) is loaded when split tab is active.
  {
    LoadObserver observer_0(GetWebContentsAt(0));
    LoadObserver observer_1(GetWebContentsAt(1));

    chrome::Reload(browser(), WindowOpenDisposition::CURRENT_TAB);
    content::WaitForLoadStop(GetWebContentsAt(0));
    content::WaitForLoadStop(GetWebContentsAt(1));

    EXPECT_FALSE(observer_0.did_load());
    EXPECT_TRUE(observer_1.did_load());
  }

  // Create another active tab.
  chrome::AddTabAt(browser(), GURL(), -1, /*foreground*/ true);
  content::WaitForLoadStop(GetWebContentsAt(2));
  EXPECT_EQ(2, tab_strip_model->active_index());
  EXPECT_EQ(3, tab_strip_model->count());
  EXPECT_FALSE(IsSplitTabAt(2));

  // Check only non-split active tab is loaded.
  {
    LoadObserver observer_0(GetWebContentsAt(0));
    LoadObserver observer_1(GetWebContentsAt(1));
    LoadObserver observer_2(GetWebContentsAt(2));

    chrome::Reload(browser(), WindowOpenDisposition::CURRENT_TAB);
    content::WaitForLoadStop(GetWebContentsAt(0));
    content::WaitForLoadStop(GetWebContentsAt(1));
    content::WaitForLoadStop(GetWebContentsAt(2));

    EXPECT_FALSE(observer_0.did_load());
    EXPECT_FALSE(observer_1.did_load());
    EXPECT_TRUE(observer_2.did_load());
  }

  // Activate split tab at 0 and check only active split tab is loaded.
  tab_strip_model->ActivateTabAt(0);
  {
    LoadObserver observer_0(GetWebContentsAt(0));
    LoadObserver observer_1(GetWebContentsAt(1));
    LoadObserver observer_2(GetWebContentsAt(2));

    chrome::Reload(browser(), WindowOpenDisposition::CURRENT_TAB);
    content::WaitForLoadStop(GetWebContentsAt(0));
    content::WaitForLoadStop(GetWebContentsAt(1));
    content::WaitForLoadStop(GetWebContentsAt(2));

    EXPECT_TRUE(observer_0.did_load());
    EXPECT_FALSE(observer_1.did_load());
    EXPECT_FALSE(observer_2.did_load());
  }

  // Select all tabs and check all tabs(split & normal tabs) are reloaded
  // as we only filter inactive split tab when reloading if only one pair
  // of split tab is selected.
  tab_strip_model->ExtendSelectionTo(2);
  {
    LoadObserver observer_0(GetWebContentsAt(0));
    LoadObserver observer_1(GetWebContentsAt(1));
    LoadObserver observer_2(GetWebContentsAt(2));

    chrome::Reload(browser(), WindowOpenDisposition::CURRENT_TAB);
    content::WaitForLoadStop(GetWebContentsAt(0));
    content::WaitForLoadStop(GetWebContentsAt(1));
    content::WaitForLoadStop(GetWebContentsAt(2));

    EXPECT_TRUE(observer_0.did_load());
    EXPECT_TRUE(observer_1.did_load());
    EXPECT_TRUE(observer_2.did_load());
  }
}

IN_PROC_BROWSER_TEST_P(SplitViewCommonBrowserTest, SplitViewCloseTabTest) {
  NewSplitTab();

  auto* tab_strip_model = browser()->tab_strip_model();
  EXPECT_EQ(1, tab_strip_model->active_index());
  EXPECT_EQ(2, tab_strip_model->count());
  EXPECT_TRUE(IsSplitTabAt(0));
  EXPECT_TRUE(IsSplitTabAt(1));

  // Check only active tab is closed from split tab when split tab is
  // the only selected tabs.
  chrome::CloseTab(browser());
  EXPECT_EQ(0, tab_strip_model->active_index());
  EXPECT_EQ(1, tab_strip_model->count());

  // Create another active tab.
  chrome::AddTabAt(browser(), GURL(), -1, /*foreground*/ true);
  NewSplitTab();

  EXPECT_EQ(2, tab_strip_model->active_index());
  EXPECT_EQ(3, tab_strip_model->count());
  EXPECT_TRUE(IsSplitTabAt(1));
  EXPECT_TRUE(IsSplitTabAt(2));
  chrome::AddTabAt(browser(), GURL(), -1, /*foreground*/ true);

  // Make tabs at 1, 2, 3 selected.
  // Check selected tab is not the only split tab,
  // we'll close all selected tabs.
  tab_strip_model->ExtendSelectionTo(1);
  EXPECT_TRUE(IsSplitTabAt(1));
  EXPECT_TRUE(IsSplitTabAt(2));
  EXPECT_EQ(1, tab_strip_model->active_index());
  EXPECT_EQ(4, tab_strip_model->count());

  // Check all selected tabs are closed (tab at 1, 2, 3).
  chrome::CloseTab(browser());
  EXPECT_EQ(0, tab_strip_model->active_index());
  EXPECT_EQ(1, tab_strip_model->count());
}

INSTANTIATE_TEST_SUITE_P(
    /* no prefix */,
    SplitViewCommonBrowserTest,
    ::testing::Bool());

class SplitViewBrowserTest : public InProcessBrowserTest {
 public:
  SplitViewBrowserTest() {
    scoped_features_.InitWithFeatures(
        /*enabled_features*/ {tabs::features::kBraveSplitView},
        /*disabled_features*/ {features::kSideBySide});
  }
  ~SplitViewBrowserTest() override = default;

  BraveBrowserView& browser_view() {
    return *static_cast<BraveBrowserView*>(
        BrowserView::GetBrowserViewForBrowser(browser()));
  }

  ContentsContainerView& secondary_contents_container() {
    return *browser_view().split_view_->secondary_contents_container_view_;
  }

  views::WebView& secondary_contents_view() {
    return *browser_view().split_view_->secondary_contents_web_view();
  }

  ScrimView& secondary_contents_scrim_view() {
    return *secondary_contents_container().contents_scrim_view();
  }

  views::WebView& secondary_dev_tools() {
    return *secondary_contents_container().devtools_web_view();
  }

  SplitView& split_view() { return *browser_view().split_view_; }

  SplitViewSeparator& split_view_separator() {
    return *browser_view().split_view_->split_view_separator_;
  }

  BrowserNonClientFrameView& browser_non_client_frame_view() {
    return *browser_view().frame()->GetFrameView();
  }

  void ToggleVerticalTabStrip() {
    brave::ToggleVerticalTabStrip(browser());
    browser_non_client_frame_view().DeprecatedLayoutImmediately();
  }

  TabStrip& tab_strip() {
    return *BrowserView::GetBrowserViewForBrowser(browser())->tabstrip();
  }

  TabStripModel& tab_strip_model() { return *(browser()->tab_strip_model()); }

 private:
  base::test::ScopedFeatureList scoped_features_;
};

IN_PROC_BROWSER_TEST_F(SplitViewBrowserTest,
                       SplitViewContainsContentsContainer) {
  EXPECT_EQ(browser_view().contents_container()->parent(), &split_view());
  EXPECT_EQ(
      static_cast<BraveBrowserViewLayout*>(browser_view().GetLayoutManager())
          ->contents_container(),
      &split_view());

  // MultiContentsView is not initialized if we don't enable
  // features::kSideBySide.
  EXPECT_FALSE(browser_view().multi_contents_view());
}

// MacOS does not need views window scrim. We use sheet to show window modals
// (-[NSWindow beginSheet:]), which natively draws a scrim since macOS 11.
#if !BUILDFLAG(IS_MAC)
IN_PROC_BROWSER_TEST_F(SplitViewBrowserTest, ScrimForSecondaryContents) {
  if (!base::FeatureList::IsEnabled(features::kScrimForBrowserWindowModal)) {
    GTEST_SKIP();
  }

  brave::NewSplitViewForTab(browser());

  auto child_widget_delegate = std::make_unique<views::WidgetDelegate>();
  auto child_widget = std::make_unique<views::Widget>();
  child_widget_delegate->SetModalType(ui::mojom::ModalType::kWindow);
  views::Widget::InitParams params(
      views::Widget::InitParams::CLIENT_OWNS_WIDGET,
      views::Widget::InitParams::TYPE_WINDOW);
  params.delegate = child_widget_delegate.get();
  params.parent = secondary_contents_container().GetWidget()->GetNativeView();
  child_widget->Init(std::move(params));

  child_widget->Show();
  EXPECT_TRUE(secondary_contents_scrim_view().GetVisible());
  child_widget->Hide();
  EXPECT_FALSE(secondary_contents_scrim_view().GetVisible());
  child_widget->Show();
  EXPECT_TRUE(secondary_contents_scrim_view().GetVisible());
  // Destroy the child widget, the parent should be notified about child modal
  // visibility change.
  child_widget.reset();
  EXPECT_FALSE(secondary_contents_scrim_view().GetVisible());
}
#endif  // !BUILDFLAG(IS_MAC)

IN_PROC_BROWSER_TEST_F(SplitViewBrowserTest,
                       GetAccessiblePaneContainsSecondaryViews) {
  secondary_contents_container().SetVisible(true);
  secondary_contents_view().SetVisible(true);
  secondary_dev_tools().SetVisible(true);
  std::vector<views::View*> panes;
  static_cast<views::WidgetDelegate*>(&browser_view())
      ->GetAccessiblePanes(&panes);
  EXPECT_TRUE(base::Contains(panes, &secondary_contents_view()));
  EXPECT_TRUE(base::Contains(panes, &secondary_dev_tools()));

  secondary_contents_view().SetVisible(false);
  secondary_dev_tools().SetVisible(false);
  panes.clear();
  static_cast<views::WidgetDelegate*>(&browser_view())
      ->GetAccessiblePanes(&panes);
  EXPECT_FALSE(base::Contains(panes, &secondary_contents_view()));
  EXPECT_FALSE(base::Contains(panes, &secondary_dev_tools()));
}

IN_PROC_BROWSER_TEST_F(SplitViewBrowserTest,
                       TilingTwoTabsMakesSecondaryWebViewVisible) {
  // Given secondary web view is hidden as there's no tiled tabs
  ASSERT_FALSE(secondary_contents_container().GetVisible());

  EXPECT_FALSE(split_view_separator().GetVisible());
  EXPECT_FALSE(split_view_separator().menu_button_widget_->IsVisible());

  // When tiling tabs and one of them is the active tab,
  brave::NewSplitViewForTab(browser());
  auto* split_view_data = browser()->GetFeatures().split_view_browser_data();
  ASSERT_TRUE(split_view_data);
  ASSERT_TRUE(split_view_data->IsTabTiled(
      tab_strip_model().GetTabAtIndex(0)->GetHandle()));
  ASSERT_TRUE(split_view_data->IsTabTiled(
      tab_strip_model().GetTabAtIndex(1)->GetHandle()));

  // Secondary web view should become visible
  EXPECT_TRUE(secondary_contents_container().GetVisible());
  EXPECT_TRUE(split_view_separator().GetVisible());
  EXPECT_TRUE(split_view_separator().menu_button_widget_->IsVisible());
}

IN_PROC_BROWSER_TEST_F(SplitViewBrowserTest,
                       BreakingTileMakesSecondaryWebViewHidden) {
  // Given there were tiled tabs
  brave::NewSplitViewForTab(browser());
  auto* split_view_data = browser()->GetFeatures().split_view_browser_data();
  ASSERT_TRUE(split_view_data);
  ASSERT_TRUE(split_view_data->IsTabTiled(
      tab_strip_model().GetTabAtIndex(0)->GetHandle()));
  ASSERT_TRUE(split_view_data->IsTabTiled(
      tab_strip_model().GetTabAtIndex(1)->GetHandle()));
  EXPECT_TRUE(split_view_separator().GetVisible());
  EXPECT_TRUE(split_view_separator().menu_button_widget_->IsVisible());

  // When breaking the tile
  split_view_data->BreakTile(tab_strip_model().GetTabAtIndex(0)->GetHandle());
  ASSERT_FALSE(split_view_data->IsTabTiled(
      tab_strip_model().GetTabAtIndex(0)->GetHandle()));

  // Then, the secondary web view should become hidden
  EXPECT_FALSE(secondary_contents_container().GetVisible());
  EXPECT_FALSE(split_view_separator().GetVisible());
  EXPECT_FALSE(split_view_separator().menu_button_widget_->IsVisible());
}

IN_PROC_BROWSER_TEST_F(SplitViewBrowserTest,
                       ActivateNonTiledTabShouldHideSecondaryWebView) {
  // Given there were tiled tabs and non tiled tab, and split view is visible
  brave::NewSplitViewForTab(browser());
  auto* split_view_data = browser()->GetFeatures().split_view_browser_data();
  ASSERT_TRUE(split_view_data);
  ASSERT_TRUE(split_view_data->IsTabTiled(
      tab_strip_model().GetTabAtIndex(0)->GetHandle()));
  ASSERT_TRUE(split_view_data->IsTabTiled(
      tab_strip_model().GetTabAtIndex(1)->GetHandle()));
  chrome::AddTabAt(browser(), GURL(), -1, /*foreground*/ false);
  ASSERT_TRUE(secondary_contents_container().GetVisible());
  EXPECT_TRUE(split_view_separator().GetVisible());
  EXPECT_TRUE(split_view_separator().menu_button_widget_->IsVisible());

  // When activating non-tiled tab
  tab_strip_model().ActivateTabAt(2);

  // Then, the secondary web view should become hidden
  ASSERT_TRUE(base::test::RunUntil(
      [&]() { return !secondary_contents_container().GetVisible(); }));
  EXPECT_FALSE(split_view_separator().GetVisible());
  EXPECT_FALSE(split_view_separator().menu_button_widget_->IsVisible());
}

IN_PROC_BROWSER_TEST_F(SplitViewBrowserTest,
                       ActivateTiledTabsShouldShowWebView) {
  // Given there were tiled tabs and non tiled tab, and the non tiled tab is the
  // active tab
  brave::NewSplitViewForTab(browser());
  auto* split_view_data = browser()->GetFeatures().split_view_browser_data();
  ASSERT_TRUE(split_view_data);
  ASSERT_TRUE(split_view_data->IsTabTiled(
      tab_strip_model().GetTabAtIndex(0)->GetHandle()));
  ASSERT_TRUE(split_view_data->IsTabTiled(
      tab_strip_model().GetTabAtIndex(1)->GetHandle()));
  chrome::AddTabAt(browser(), GURL(), -1, /*foreground*/ true);
  ASSERT_FALSE(secondary_contents_container().GetVisible());

  // When activating a tiled tab
  tab_strip_model().ActivateTabAt(0);

  // Then, the secondary web view should show up
  ASSERT_TRUE(base::test::RunUntil(
      [&]() { return secondary_contents_container().GetVisible(); }));
}

IN_PROC_BROWSER_TEST_F(SplitViewBrowserTest,
                       SecondaryWebViewShouldHoldNonActiveTiledTab) {
  // Given that two tabs are tiled
  brave::NewSplitViewForTab(browser());
  auto* split_view_data = browser()->GetFeatures().split_view_browser_data();
  ASSERT_TRUE(split_view_data);
  ASSERT_TRUE(split_view_data->IsTabTiled(
      tab_strip_model().GetTabAtIndex(0)->GetHandle()));
  ASSERT_TRUE(split_view_data->IsTabTiled(
      tab_strip_model().GetTabAtIndex(1)->GetHandle()));
  ASSERT_TRUE(secondary_contents_container().GetVisible());
  auto tile = *split_view_data->GetTile(
      tab_strip_model().GetTabAtIndex(0)->GetHandle());

  // When the tile.first is active contents,
  tab_strip_model().ActivateTabAt(
      tab_strip_model().GetIndexOfTab(tile.first.Get()));
  auto active_tab_handle =
      tab_strip_model()
          .GetTabAtIndex(tab_strip_model().GetIndexOfWebContents(
              tab_strip_model().GetActiveWebContents()))
          ->GetHandle();
  ASSERT_EQ(active_tab_handle, tile.first);

  // Then the secondary web view should hold the tile.second
  EXPECT_EQ(tab_strip_model().GetWebContentsAt(
                tab_strip_model().GetIndexOfTab(tile.second.Get())),
            secondary_contents_view().web_contents());

  // On the other hand, when tile.second is active contents,
  tab_strip_model().ActivateTabAt(
      tab_strip_model().GetIndexOfTab(tile.second.Get()));
  active_tab_handle =
      tab_strip_model()
          .GetTabAtIndex(tab_strip_model().GetIndexOfWebContents(
              tab_strip_model().GetActiveWebContents()))
          ->GetHandle();
  ASSERT_EQ(active_tab_handle, tile.second);

  // Then the secondary web view should hold the tile.first
  EXPECT_EQ(tab_strip_model().GetWebContentsAt(
                tab_strip_model().GetIndexOfTab(tile.first.Get())),
            secondary_contents_view().web_contents());
}

IN_PROC_BROWSER_TEST_F(SplitViewBrowserTest, SplitViewSizeDelta) {
  // Given there are two tiles
  brave::NewSplitViewForTab(browser());
  chrome::AddTabAt(browser(), GURL(), -1, /*foreground*/ true);
  brave::NewSplitViewForTab(browser());

  // When size delta is set
  auto* split_view_layout_manager =
      static_cast<SplitViewLayoutManager*>(split_view().GetLayoutManager());
  constexpr int kSizeDelta = 100;
  split_view_layout_manager->set_split_view_size_delta(kSizeDelta);

  // Then these should be persisted during tab activation.
  tab_strip_model().ActivateTabAt(0);
  EXPECT_EQ(0, split_view_layout_manager->split_view_size_delta());

  tab_strip_model().ActivateTabAt(3);
  EXPECT_EQ(kSizeDelta, split_view_layout_manager->split_view_size_delta());
}

IN_PROC_BROWSER_TEST_F(SplitViewBrowserTest, SplitViewTabPathTest) {
  brave::NewSplitViewForTab(browser());
  int active_index = tab_strip_model().active_index();
  ASSERT_NE(TabStripModel::kNoTab, active_index);

  TabStrip* tab_strip = browser_view().tabstrip();
  Tab* tab = tab_strip->tab_at(active_index);

  SkPath mask = tab->tab_style_views()->GetPath(TabStyle::PathType::kFill,
                                                /* scale */ 1.0,
                                                /* force_active */ false,
                                                TabStyle::RenderUnits::kDips);
  SkRegion clip_region;
  clip_region.setRect({0, 0, 200, 200});
  SkRegion mask_region;
  ASSERT_TRUE(mask_region.setPath(mask, clip_region));

  EXPECT_EQ(brave_tabs::kHorizontalSplitViewTabVerticalSpacing,
            mask_region.getBounds().top());
  EXPECT_EQ(brave_tabs::kHorizontalTabInset, mask_region.getBounds().left());
  EXPECT_EQ(GetLayoutConstant(TAB_STRIP_HEIGHT) -
                GetLayoutConstant(TABSTRIP_TOOLBAR_OVERLAP) -
                (brave_tabs::kHorizontalSplitViewTabVerticalSpacing * 2),
            mask_region.getBounds().height());
}

IN_PROC_BROWSER_TEST_F(SplitViewBrowserTest, SplitViewFullscreenTest) {
  brave::NewSplitViewForTab(browser());

  // In split view tile, both contents are visible and have its border.
  EXPECT_TRUE(browser_view().contents_container()->GetVisible());
  EXPECT_TRUE(browser_view().contents_container()->GetBorder());
  EXPECT_TRUE(secondary_contents_container().GetVisible());
  EXPECT_TRUE(secondary_contents_container().GetBorder());

  // Simulate tab-fullscreen state change.
  FullscreenController* fullscreen_controller = browser()
                                                    ->GetFeatures()
                                                    .exclusive_access_manager()
                                                    ->fullscreen_controller();
  fullscreen_controller->set_is_tab_fullscreen_for_testing(true);
  split_view().OnFullscreenStateChanged();

  // In tab full screen, only primary content is visible w/o border.
  EXPECT_TRUE(browser_view().contents_container()->GetVisible());
  EXPECT_FALSE(browser_view().contents_container()->GetBorder());
  EXPECT_FALSE(secondary_contents_container().GetVisible());
  EXPECT_FALSE(secondary_contents_container().GetBorder());

  fullscreen_controller->set_is_tab_fullscreen_for_testing(false);
  split_view().OnFullscreenStateChanged();

  EXPECT_TRUE(browser_view().contents_container()->GetVisible());
  EXPECT_TRUE(browser_view().contents_container()->GetBorder());
  EXPECT_TRUE(secondary_contents_container().GetVisible());
  EXPECT_TRUE(secondary_contents_container().GetBorder());
}

class SplitViewCookieBrowserTest : public SplitViewBrowserTest {
 public:
  void SetUpOnMainThread() override {
    SplitViewBrowserTest::SetUpOnMainThread();
    embedded_test_server()->RegisterRequestHandler(base::BindRepeating(
        [](const net::test_server::HttpRequest& request)
            -> std::unique_ptr<net::test_server::HttpResponse> {
          auto response =
              std::make_unique<net::test_server::BasicHttpResponse>();
          response->set_content(request.headers.count("Cookie")
                                    ? request.headers.at("Cookie")
                                    : "NONE");
          response->set_code(net::HTTP_OK);
          return response;
        }));

    ASSERT_TRUE(embedded_test_server()->Start());
  }

  void TearDownOnMainThread() override {
    ASSERT_TRUE(embedded_test_server()->ShutdownAndWaitUntilComplete());
    SplitViewBrowserTest::TearDownOnMainThread();
  }
};

class MockBraveRenderViewContextMenu : public BraveRenderViewContextMenu {
 public:
  MockBraveRenderViewContextMenu(content::RenderFrameHost& render_frame_host,
                                 const content::ContextMenuParams& params)
      : BraveRenderViewContextMenu(render_frame_host, params) {}
  ~MockBraveRenderViewContextMenu() override = default;

  void Show() override {}
};

IN_PROC_BROWSER_TEST_F(SplitViewCookieBrowserTest, CookieTest) {
  // Navigate to a page and sets a cookie.
  GURL target_url = embedded_test_server()->GetURL("/");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), target_url));
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(
      content::ExecJs(web_contents->GetPrimaryMainFrame(),
                      "document.cookie = 'Strict=value1; SameSite=Strict;'"));
  ASSERT_TRUE(content::ExecJs(web_contents->GetPrimaryMainFrame(),
                              "document.cookie = 'Lax=value2; SameSite=Lax;'"));

  // Navigate to a page that sub page to check cookies.
  target_url = embedded_test_server()->GetURL("/sub");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), target_url));
  auto cookie =
      content::EvalJs(web_contents->GetPrimaryMainFrame(), "document.cookie")
          .ExtractString();
  EXPECT_EQ(cookie, "Strict=value1; Lax=value2");

  // Open up different site
  chrome::AddTabAt(browser(), GURL("about:blank"), /*index*/ 1,
                   /*foreground*/ true);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL("about:blank")));

  // Triggers "Open link in split view" context menu command.
  auto* initiator_contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  content::ContextMenuParams params;
  params.page_url = initiator_contents->GetLastCommittedURL();
  params.frame_url =
      initiator_contents->GetPrimaryMainFrame()->GetLastCommittedURL();
  params.frame_origin = url::Origin::Create(params.frame_url);
  params.link_url = target_url;

  auto menu = static_cast<std::unique_ptr<RenderViewContextMenuBase>>(
      std::make_unique<MockBraveRenderViewContextMenu>(
          *initiator_contents->GetPrimaryMainFrame(), params));
  menu->Init();

  ASSERT_TRUE(
      menu->IsCommandIdEnabled(IDC_CONTENT_CONTEXT_OPENLINK_SPLIT_VIEW));
  menu->ExecuteCommand(IDC_CONTENT_CONTEXT_OPENLINK_SPLIT_VIEW, 0);

  // Split view should be created with the target url
  auto* split_contents = browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(brave::IsTabsTiled(
      browser(), {tab_strip_model().GetIndexOfWebContents(split_contents)}));
  ASSERT_EQ(3, browser()->tab_strip_model()->count());

  // Getting cookies from client site should be same
  ASSERT_TRUE(
      base::test::RunUntil([&]() { return !split_contents->IsLoading(); }));
  auto client_cookies =
      content::EvalJs(split_contents->GetPrimaryMainFrame(), "document.cookie")
          .ExtractString();
  ASSERT_EQ(client_cookies, "Strict=value1; Lax=value2");

  // But cookies that server has got must not contain Strict value as it was
  // requested from different site
  std::string cookies_server_got =
      content::EvalJs(split_contents->GetPrimaryMainFrame(),
                      "document.body.innerHTML")
          .ExtractString();
  EXPECT_FALSE(base::Contains(cookies_server_got, "Strict=value1"))
      << cookies_server_got;
  EXPECT_TRUE(base::Contains(cookies_server_got, "Lax=value2"))
      << cookies_server_got;
}
