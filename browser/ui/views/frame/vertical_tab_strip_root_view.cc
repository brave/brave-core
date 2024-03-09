/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/vertical_tab_strip_root_view.h"

#include "chrome/browser/ui/views/frame/browser_root_view.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/views/view_utils.h"

VerticalTabStripRootView::VerticalTabStripRootView(BrowserView* browser_view,
                                                   views::Widget* widget)
    : BrowserRootView(browser_view, widget) {}

VerticalTabStripRootView::~VerticalTabStripRootView() = default;

bool VerticalTabStripRootView::OnMousePressed(const ui::MouseEvent& event) {
#if defined(USE_AURA)
  const bool result = RootView::OnMousePressed(event);
  auto* focus_manager = GetFocusManager();
  DCHECK(focus_manager);

  // When vertical tab strip area is clicked, shortcut handling process
  // could get broken on Windows. There are 2 paths where shortcut is handled.
  // One is BrowserView::AcceleratorPressed(), and the other is
  // BrowserView::PreHandleKeyboardEvent(). When web view has focus, the
  // first doesn't deal with it and the latter is responsible for the
  // shortcuts. when users click the vertical tab strip area with web view
  // focused, both path don't handle it. This is because focused view state of
  // views/ framework and focused native window state of Aura is out of sync.
  // So as a workaround, resets the focused view state so that shortcuts can
  // be handled properly. This shouldn't change the actually focused view, and
  // is just reset the status.
  // https://github.com/brave/brave-browser/issues/28090
  // https://github.com/brave/brave-browser/issues/27812
  if (auto* focused_view = focus_manager->GetFocusedView();
      focused_view && views::IsViewClass<views::WebView>(focused_view)) {
    focus_manager->ClearFocus();
    focus_manager->RestoreFocusedView();
  }

  return result;
#else
  // On Mac, the parent widget doesn't get activated in this case. Then
  // shortcut handling could malfunction. So activate it.
  // https://github.com/brave/brave-browser/issues/29993
  auto* widget = GetWidget();
  DCHECK(widget);
  widget = widget->GetTopLevelWidget();
  widget->Activate();

  return RootView::OnMousePressed(event);
#endif
}

bool VerticalTabStripRootView::OnMouseWheel(const ui::MouseWheelEvent& event) {
  return views::internal::RootView::OnMouseWheel(event);
}

void VerticalTabStripRootView::OnMouseExited(const ui::MouseEvent& event) {
  views::internal::RootView::OnMouseExited(event);
}

void VerticalTabStripRootView::PaintChildren(
    const views::PaintInfo& paint_info) {
  views::internal::RootView::PaintChildren(paint_info);
}

BEGIN_METADATA(VerticalTabStripRootView)
END_METADATA
