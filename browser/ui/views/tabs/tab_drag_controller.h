/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TABS_TAB_DRAG_CONTROLLER_H_
#define BRAVE_BROWSER_UI_VIEWS_TABS_TAB_DRAG_CONTROLLER_H_

#include <optional>
#include <vector>

#include "brave/browser/ui/tabs/split_view_browser_data.h"
#include "brave/browser/ui/views/frame/vertical_tab_strip_region_view.h"
#include "chrome/browser/ui/views/tabs/tab_drag_controller.h"

class TabDragController : public TabDragControllerChromium {
 public:
  TabDragController();
  ~TabDragController() override;

  // Making this virtual method is really painful as "Init" is too common name.
  // So, just hide Chromium's Init and make clients use this version
  Liveness Init(TabDragContext* source_context,
                TabSlotView* source_view,
                const std::vector<raw_ptr<TabSlotView, VectorExperimental>>&
                    dragging_views,
                const gfx::Point& mouse_offset,
                int source_view_offset,
                ui::ListSelectionModel initial_selection_model,
                ui::mojom::DragEventSource event_source);

  // TabDragControllerChromium:
  gfx::Point GetAttachedDragPoint(const gfx::Point& point_in_screen) override;
  void MoveAttached(const gfx::Point& point_in_screen,
                    bool just_attached) override;
  views::Widget* GetAttachedBrowserWidget() override;

  Liveness GetLocalProcessWindow(const gfx::Point& screen_point,
                                 bool exclude_dragged_view,
                                 gfx::NativeWindow* window) override;

  void DetachAndAttachToNewContext(ReleaseCapture release_capture,
                                   TabDragContext* target_context,
                                   const gfx::Point& point_in_screen,
                                   bool set_capture = true) override;

  gfx::Rect CalculateNonMaximizedDraggedBrowserBounds(
      views::Widget* widget,
      const gfx::Point& point_in_screen) override;
  gfx::Rect CalculateDraggedBrowserBounds(
      TabDragContext* source,
      const gfx::Point& point_in_screen,
      std::vector<gfx::Rect>* drag_bounds) override;
  [[nodiscard]] Liveness ContinueDragging(
      const gfx::Point& point_in_screen) override;

  void InitDragData(TabSlotView* view, TabDragData* drag_data) override;

 private:
  gfx::Vector2d GetVerticalTabStripWidgetOffset();

  bool is_showing_vertical_tabs_ = false;

  VerticalTabStripRegionView::ScopedStateResetter vertical_tab_state_resetter_;

  SplitViewBrowserData::OnTabDragEndedClosure on_tab_drag_ended_closure_;

  base::WeakPtrFactory<TabDragController> weak_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TABS_TAB_DRAG_CONTROLLER_H_
