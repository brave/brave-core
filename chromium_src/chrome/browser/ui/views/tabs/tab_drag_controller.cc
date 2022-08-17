/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/tabs/tab_drag_controller.h"

#define TabDragController TabDragControllerChromium
#include "src/chrome/browser/ui/views/tabs/tab_drag_controller.cc"
#undef TabDragController

#include "brave/browser/ui/views/tabs/features.h"

TabDragController::~TabDragController() = default;

// In order to use inner class defined in tab_drag_controller.cc, this function
// is defined here.
void TabDragController::Init(TabDragContext* source_context,
                             TabSlotView* source_view,
                             const std::vector<TabSlotView*>& dragging_views,
                             const gfx::Point& mouse_offset,
                             int source_view_offset,
                             ui::ListSelectionModel initial_selection_model,
                             EventSource event_source) {
  if (!tabs::features::ShouldShowVerticalTabs()) {
    TabDragControllerChromium::Init(source_context, source_view, dragging_views,
                                    mouse_offset, source_view_offset,
                                    initial_selection_model, event_source);
    return;
  }

  // Most of code here is borrowed from TabDragControllerChromium::Init() but we
  // changes coordinates for vertical tabs.
  //
  DCHECK(!dragging_views.empty());
  DCHECK(base::Contains(dragging_views, source_view));
  // Adjust offset for vertical mode.
  source_view_offset = mouse_offset.y();

  source_context_ = source_context;
  was_source_maximized_ = source_context->AsView()->GetWidget()->IsMaximized();
  was_source_fullscreen_ =
      source_context->AsView()->GetWidget()->IsFullscreen();
  // Do not release capture when transferring capture between widgets on:
  // - Desktop Linux
  //     Mouse capture is not synchronous on desktop Linux. Chrome makes
  //     transferring capture between widgets without releasing capture appear
  //     synchronous on desktop Linux, so use that.
  // - Chrome OS
  //     Releasing capture on Ash cancels gestures so avoid it.
#if BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_CHROMEOS)
  can_release_capture_ = false;
#endif
  start_point_in_screen_ = gfx::Point(mouse_offset.x(), source_view_offset);
  views::View::ConvertPointToScreen(source_view, &start_point_in_screen_);
  event_source_ = event_source;
  mouse_offset_ = mouse_offset;
  last_point_in_screen_ = start_point_in_screen_;
  last_move_screen_loc_ = start_point_in_screen_.y();
  initial_tab_positions_ = source_context->GetTabXCoordinates();

  source_context_emptiness_tracker_ =
      std::make_unique<SourceTabStripEmptinessTracker>(
          source_context_->GetTabStripModel(), this);

  header_drag_ = source_view->GetTabSlotViewType() ==
                 TabSlotView::ViewType::kTabGroupHeader;
  if (header_drag_)
    group_ = source_view->group();

  drag_data_.resize(dragging_views.size());
  for (size_t i = 0; i < dragging_views.size(); ++i)
    InitDragData(dragging_views[i], &(drag_data_[i]));
  source_view_index_ =
      std::find(dragging_views.begin(), dragging_views.end(), source_view) -
      dragging_views.begin();

  // Listen for Esc key presses.
  key_event_tracker_ = std::make_unique<KeyEventTracker>(
      base::BindOnce(&TabDragController::EndDrag, base::Unretained(this),
                     END_DRAG_COMPLETE),
      base::BindOnce(&TabDragController::EndDrag, base::Unretained(this),
                     END_DRAG_CANCEL),
      source_context_->AsView()->GetWidget()->GetNativeWindow());

  if (source_view->width() > 0) {
    offset_to_width_ratio_ = static_cast<float>(source_view->GetMirroredXInView(
                                 source_view_offset)) /
                             source_view->width();
  }
  InitWindowCreatePoint();
  initial_selection_model_ = std::move(initial_selection_model);

  // Gestures don't automatically do a capture. We don't allow multiple drags at
  // the same time, so we explicitly capture.
  if (event_source == EVENT_SOURCE_TOUCH) {
    // Taking capture may cause capture to be lost, ending the drag and
    // destroying |this|.
    base::WeakPtr<TabDragControllerChromium> ref(weak_factory_.GetWeakPtr());
    SetCapture(source_context_);
    if (!ref)
      return;
  }

  window_finder_ = std::make_unique<WindowFinder>();
}

gfx::Point TabDragController::GetAttachedDragPoint(
    const gfx::Point& point_in_screen) {
  if (!tabs::features::ShouldShowVerticalTabs())
    return TabDragControllerChromium::GetAttachedDragPoint(point_in_screen);

  DCHECK(attached_context_);  // The tab must be attached.

  gfx::Point tab_loc(point_in_screen);
  views::View::ConvertPointFromScreen(attached_context_->AsView(), &tab_loc);
  const int y = tab_loc.y() - mouse_offset_.y();
  return gfx::Point(0, std::max(0, y));
}

void TabDragController::MoveAttached(const gfx::Point& point_in_screen,
                                     bool just_attached) {
  if (!tabs::features::ShouldShowVerticalTabs()) {
    TabDragControllerChromium::MoveAttached(point_in_screen, just_attached);
    return;
  }

  DCHECK(attached_context_);
  DCHECK_EQ(current_state_, DragState::kDraggingTabs);

  gfx::Point dragged_view_point = GetAttachedDragPoint(point_in_screen);

  const int threshold = attached_context_->GetHorizontalDragThreshold();

  std::vector<TabSlotView*> views(drag_data_.size());
  for (size_t i = 0; i < drag_data_.size(); ++i)
    views[i] = drag_data_[i].attached_view;

  bool did_layout = false;
  // Update the model, moving the WebContents from one index to another. Do this
  // only if we have moved a minimum distance since the last reorder (to prevent
  // jitter), or if this the first move and the tabs are not consecutive, or if
  // we have just attached to a new tabstrip and need to move to the correct
  // initial position.
  if (just_attached ||
      (abs(point_in_screen.y() - last_move_screen_loc_) > threshold) ||
      (initial_move_ && !AreTabsConsecutive())) {
    TabStripModel* attached_model = attached_context_->GetTabStripModel();
    int to_index = attached_context_->GetInsertionIndexForDraggedBounds(
        GetDraggedViewTabStripBounds(dragged_view_point),
        GetViewsMatchingDraggedContents(attached_context_), num_dragging_tabs(),
        group_);
    bool do_move = true;
    // While dragging within a tabstrip the expectation is the insertion index
    // is based on the left edge of the tabs being dragged. OTOH when dragging
    // into a new tabstrip (attaching) the expectation is the insertion index is
    // based on the cursor. This proves problematic as insertion may change the
    // size of the tabs, resulting in the index calculated before the insert
    // differing from the index calculated after the insert. To alleviate this
    // the index is chosen before insertion, and subsequently a new index is
    // only used once the mouse moves enough such that the index changes based
    // on the direction the mouse moved relative to |attach_x_| (smaller
    // x-coordinate should yield a smaller index or larger x-coordinate yields a
    // larger index).
    if (attach_index_ != -1) {
      gfx::Point tab_strip_point(point_in_screen);
      views::View::ConvertPointFromScreen(attached_context_->AsView(),
                                          &tab_strip_point);
      const int new_x =
          attached_context_->AsView()->GetMirroredXInView(tab_strip_point.x());
      if (new_x < attach_x_)
        to_index = std::min(to_index, attach_index_);
      else
        to_index = std::max(to_index, attach_index_);
      if (to_index != attach_index_)
        attach_index_ = -1;  // Once a valid move is detected, don't constrain.
      else
        do_move = false;
    }
    if (do_move) {
      WebContents* last_contents = drag_data_.back().contents;
      int index_of_last_item =
          attached_model->GetIndexOfWebContents(last_contents);
      if (initial_move_) {
        // TabDragContext determines if the tabs needs to be animated
        // based on model position. This means we need to invoke
        // LayoutDraggedTabsAt before changing the model.
        attached_context_->LayoutDraggedViewsAt(
            views, source_view_drag_data()->attached_view, dragged_view_point,
            initial_move_);
        did_layout = true;
      }

      attached_model->MoveSelectedTabsTo(to_index);

      if (header_drag_) {
        attached_model->MoveTabGroup(group_.value());
      } else {
        UpdateGroupForDraggedTabs();
      }

      // Move may do nothing in certain situations (such as when dragging pinned
      // tabs). Make sure the tabstrip actually changed before updating
      // last_move_screen_loc_.
      if (index_of_last_item !=
          attached_model->GetIndexOfWebContents(last_contents)) {
        last_move_screen_loc_ = point_in_screen.y();
      }
    }
  }

  if (!did_layout) {
    attached_context_->LayoutDraggedViewsAt(
        views, source_view_drag_data()->attached_view, dragged_view_point,
        initial_move_);
  }

  initial_move_ = false;
}
