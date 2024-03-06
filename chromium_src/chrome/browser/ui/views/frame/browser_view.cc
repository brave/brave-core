/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// Include the corresponding header first since it defines the same macros and
// therefore avoid undef before use.
#include "chrome/browser/ui/views/frame/browser_view.h"

#include "brave/browser/ui/views/frame/brave_browser_view_layout.h"
#include "brave/browser/ui/views/frame/brave_contents_layout_manager.h"
#include "brave/browser/ui/views/frame/brave_tab_strip_region_view.h"
#include "brave/browser/ui/views/infobars/brave_infobar_container_view.h"
#include "brave/browser/ui/views/side_panel/brave_side_panel.h"
#include "brave/browser/ui/views/side_panel/brave_side_panel_coordinator.h"
#include "brave/browser/ui/views/tabs/brave_browser_tab_strip_controller.h"
#include "brave/browser/ui/views/tabs/brave_tab_strip.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "brave/browser/ui/views/toolbar/brave_toolbar_view.h"
#include "build/build_config.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/views/frame/browser_view_layout.h"
#include "chrome/browser/ui/views/side_panel/side_panel.h"
#include "chrome/browser/ui/views/theme_copying_widget.h"

#define InfoBarContainerView BraveInfoBarContainerView
#define BrowserViewLayout BraveBrowserViewLayout
#define ToolbarView BraveToolbarView
#define BrowserTabStripController BraveBrowserTabStripController
#define TabStrip BraveTabStrip
#define TabStripRegionView BraveTabStripRegionView
#define SidePanel BraveSidePanel
#define kAlignLeft kHorizontalAlignLeft
#define kAlignRight kHorizontalAlignRight
#define SidePanelCoordinator BraveSidePanelCoordinator
#define BookmarkBarView BraveBookmarkBarView
#define ContentsLayoutManager BraveContentsLayoutManager

#include "src/chrome/browser/ui/views/frame/browser_view.cc"

#undef ContentsLayoutManager
#undef BookmarkBarView
#undef SidePanelCoordinator
#undef kAlignLeft
#undef kAlignRight
#undef SidePanel
#undef TabStripRegionView
#undef TabStrip
#undef BrowserTabStripController
#undef ToolbarView
#undef BrowserViewLayout
#undef InfoBarContainerView

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_LINUX)
namespace {

// OverlayWidget is a child Widget of BrowserFrame used during immersive
// fullscreen on Windows that hosts the top container.
// Currently the only explicit reason for OverlayWidget to be its own subclass
// is to support GetAccelerator() forwarding.
class OverlayWidget : public ThemeCopyingWidget {
 public:
  explicit OverlayWidget(views::Widget* role_model)
      : ThemeCopyingWidget(role_model) {}

  OverlayWidget(const OverlayWidget&) = delete;
  OverlayWidget& operator=(const OverlayWidget&) = delete;

  ~OverlayWidget() override = default;

  // OverlayWidget hosts the top container. Views within the top container look
  // up accelerators by asking their hosting Widget. In non-immersive fullscreen
  // that would be the BrowserFrame. Give top chrome what it expects and forward
  // GetAccelerator() calls to OverlayWidget's parent (BrowserFrame).
  bool GetAccelerator(int cmd_id, ui::Accelerator* accelerator) const override {
    DCHECK(parent());
    return parent()->GetAccelerator(cmd_id, accelerator);
  }
};

// TabContainerOverlayView is a view that hosts the TabStripRegionView during
// immersive fullscreen. The TopContainerView usually draws the background for
// the tab strip. Since the tab strip has been reparented we need to handle
// drawing the background here.
class TabContainerOverlayView : public views::View {
  METADATA_HEADER(TabContainerOverlayView, views::View)
 public:
  explicit TabContainerOverlayView(base::WeakPtr<BrowserView> browser_view)
      : browser_view_(std::move(browser_view)) {}
  ~TabContainerOverlayView() override = default;

  // views::View override
  void OnPaintBackground(gfx::Canvas* canvas) override {
    SkColor frame_color = browser_view_->frame()->GetFrameView()->GetFrameColor(
        BrowserFrameActiveState::kUseCurrent);
    canvas->DrawColor(frame_color);

    auto* theme_service =
        ThemeServiceFactory::GetForProfile(browser_view_->browser()->profile());
    if (!theme_service->UsingSystemTheme()) {
#if 0
      auto* non_client_frame_view = browser_view_->frame()->GetFrameView();
      non_client_frame_view->PaintThemedFrame(canvas);
#endif
    }
  }

 private:
  // The BrowserView this overlay is created for. WeakPtr is used since
  // this view is held in a different hierarchy.
  base::WeakPtr<BrowserView> browser_view_;
};

BEGIN_METADATA(TabContainerOverlayView)
END_METADATA
}  // namespace
#endif  // BUILDFLAG(IS_WIN) || BUILDFLAG(IS_LINUX)

void BrowserView::SetNativeWindowPropertyForWidget(views::Widget* widget) {
  // Sets a kBrowserWindowKey to given child |widget| so that we can get
  // BrowserView from the |widget|.
  DCHECK(GetWidget());
  DCHECK_EQ(GetWidget(), widget->GetTopLevelWidget())
      << "The |widget| should be child of BrowserView's widget.";

  widget->SetNativeWindowProperty(kBrowserViewKey, this);
}

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_LINUX)
views::View* BrowserView::CreateWinOverlayView() {
  DCHECK(UsesImmersiveFullscreenMode());

  auto create_overlay_widget = [this](views::Widget* parent) -> views::Widget* {
    views::Widget::InitParams params;
    params.type = views::Widget::InitParams::TYPE_POPUP;
    params.child = false;
    params.parent = parent->GetNativeView();
    params.shadow_type = views::Widget::InitParams::ShadowType::kNone;
    params.activatable = views::Widget::InitParams::Activatable::kYes;
    OverlayWidget* overlay_widget = new OverlayWidget(GetWidget());

    // When the overlay is used some Views are moved to the overlay_widget. When
    // this happens we want the fullscreen state of the overlay_widget to match
    // that of BrowserView's Widget. Without this, some views would not think
    // they are in a fullscreen Widget, when we want them to behave as though
    // they are in a fullscreen Widget.
    overlay_widget->SetCheckParentForFullscreen();

    overlay_widget->Init(std::move(params));
    overlay_widget->SetNativeWindowProperty(kBrowserViewKey, this);

    return overlay_widget;
  };

  // Create the toolbar overlay widget.
  overlay_widget_ = create_overlay_widget(GetWidget());

  // Create a new TopContainerOverlayView. The tab strip, omnibox, bookmarks
  // etc. will be contained within this view. Right clicking on the blank space
  // that is not taken up by the child views should show the context menu. Set
  // the BrowserFrame as the context menu controller to handle displaying the
  // top container context menu.
  std::unique_ptr<TopContainerOverlayView> overlay_view =
      std::make_unique<TopContainerOverlayView>(weak_ptr_factory_.GetWeakPtr());
  overlay_view->set_context_menu_controller(frame());

  overlay_view->SetEventTargeter(std::make_unique<views::ViewTargeter>(
      std::make_unique<OverlayViewTargeterDelegate>()));
  overlay_view_ = overlay_view.get();
  overlay_widget_->GetRootView()->AddChildView(std::move(overlay_view));

  if (UsesImmersiveFullscreenTabbedMode()) {
    // Create the tab overlay widget as a child of overlay_widget_.
    tab_overlay_widget_ = create_overlay_widget(overlay_widget_);
    std::unique_ptr<TabContainerOverlayView> tab_overlay_view =
        std::make_unique<TabContainerOverlayView>(
            weak_ptr_factory_.GetWeakPtr());
    tab_overlay_view->set_context_menu_controller(frame());
    tab_overlay_view->SetEventTargeter(std::make_unique<views::ViewTargeter>(
        std::make_unique<OverlayViewTargeterDelegate>()));
    tab_overlay_view_ = tab_overlay_view.get();
    tab_overlay_widget_->GetRootView()->AddChildView(
        std::move(tab_overlay_view));
  }

  return overlay_view_;
}

bool BrowserView::UsesImmersiveFullscreenMode() const {
  const bool is_pwa =
      base::FeatureList::IsEnabled(features::kImmersiveFullscreenPWAs) &&
      GetIsWebAppType();
  const bool is_tabbed_window = GetSupportsTabStrip();
  return base::FeatureList::IsEnabled(features::kImmersiveFullscreen) &&
         (is_pwa || is_tabbed_window) &&
         !tabs::utils::ShouldShowVerticalTabs(browser());
}

bool BrowserView::UsesImmersiveFullscreenTabbedMode() const {
  return (GetSupportsTabStrip() &&
          base::FeatureList::IsEnabled(features::kImmersiveFullscreen) &&
          base::FeatureList::IsEnabled(features::kImmersiveFullscreenTabs) &&
          !GetIsWebAppType() &&
          !tabs::utils::ShouldShowVerticalTabs(browser()));
}
#endif  // BUILDFLAG(IS_WIN) || BUILDFLAG(IS_LINUX)
