/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/side_panel/brave_side_panel_resize_widget.h"

#include <utility>

#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/side_panel/brave_side_panel.h"
#include "build/build_config.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/views/controls/resize_area.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/widget/widget.h"

#if defined(USE_AURA)
#include "ui/aura/window.h"
#include "ui/views/view_constants_aura.h"
#endif

#if BUILDFLAG(IS_MAC)
namespace {

// Subclassed to clear resize cursor when goes out. On macOS, it seems
// widget doesn't clear current cursor(resize) when mouse goes out in some
// specific situation unexpectedly. Because of that, cursor is not changed when
// mouse moves in. Widget doesn't update its cursor if requested one is same
// with previous one. Maybe this problem happens because it's located above
// WebView.
class CustomResizeArea : public views::ResizeArea {
  METADATA_HEADER(CustomResizeArea, views::ResizeArea)
 public:
  using ResizeArea::ResizeArea;

  void OnMouseExited(const ui::MouseEvent& event) override {
    ResizeArea::OnMouseExited(event);
    GetWidget()->SetCursor(ui::Cursor());
  }
};

BEGIN_METADATA(CustomResizeArea)
END_METADATA

}  // namespace
#endif

SidePanelResizeWidget::SidePanelResizeWidget(
    BraveSidePanel* panel,
    BraveBrowserView* browser_view,
    views::ResizeAreaDelegate* resize_area_delegate)
    : panel_(panel) {
  DCHECK(browser_view->GetWidget());

  observations_.AddObservation(panel_);
  observations_.AddObservation(browser_view->contents_container());

  widget_ = std::make_unique<views::Widget>();
  views::Widget::InitParams params(
      views::Widget::InitParams::CLIENT_OWNS_WIDGET,
      views::Widget::InitParams::TYPE_CONTROL);
  params.delegate = this;
  params.name = "SidePanelResizeWidget";
  params.parent = browser_view->GetWidget()->GetNativeView();
  params.opacity = views::Widget::InitParams::WindowOpacity::kTranslucent;
  params.activatable = views::Widget::InitParams::Activatable::kNo;
  widget_->Init(std::move(params));

#if BUILDFLAG(IS_MAC)
  widget_->SetContentsView(
      std::make_unique<CustomResizeArea>(resize_area_delegate));
#else
  widget_->SetContentsView(
      std::make_unique<views::ResizeArea>(resize_area_delegate));
#endif

#if defined(USE_AURA)
  widget_->GetNativeView()->SetProperty(views::kHostViewKey,
                                        browser_view->sidebar_host_view());
#endif

  if (panel_->GetVisible()) {
    widget_->ShowInactive();
  }
}

SidePanelResizeWidget::~SidePanelResizeWidget() = default;

void SidePanelResizeWidget::OnViewBoundsChanged(views::View* observed_view) {
  auto rect = panel_->GetLocalBounds();
  auto point = rect.origin();
  views::View::ConvertPointToTarget(panel_, panel_->GetWidget()->GetRootView(),
                                    &point);
  rect.set_origin(point);
  constexpr int kWidgetNarrowWidth = 5;
  if (!panel_->IsRightAligned()) {
    rect.set_x(rect.right() - kWidgetNarrowWidth);
  }
  rect.set_width(kWidgetNarrowWidth);

  widget_->GetContentsView()->SetPreferredSize(rect.size());

#if BUILDFLAG(IS_MAC)
  // On Mac, we can't set empty bounds for the widget.
  if (rect.IsEmpty()) {
    rect.set_size({kWidgetNarrowWidth, 1});
  }
#endif

  widget_->SetBounds(rect);
}

void SidePanelResizeWidget::OnViewVisibilityChanged(
    views::View* observed_view,
    views::View* starting_view) {
  // As this widget is for resizing side panel,
  // show only this when panel is visible.
  if (panel_ != observed_view) {
    return;
  }

  panel_->GetVisible() ? widget_->ShowInactive() : widget_->Hide();
}

void SidePanelResizeWidget::OnViewIsDeleting(views::View* observed_view) {
  DCHECK(observations_.IsObservingSource(observed_view));
  observations_.RemoveObservation(observed_view);
}
