/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/edge_reveal/edge_reveal_watcher.h"

#include <utility>

#include "base/check.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"

EdgeRevealWatcher::EdgeRevealWatcher(
    views::Widget* host,
    EdgeContainsViewCallback edge_contains_view,
    RevealReasonsChangedCallback reveal_reasons_changed,
    std::unique_ptr<TransientWidgetTracker> tracker)
    : host_widget_(host),
      edge_contains_view_(std::move(edge_contains_view)),
      reveal_reasons_changed_(std::move(reveal_reasons_changed)),
      tracker_(std::move(tracker)) {
  CHECK(host_widget_);
  CHECK(edge_contains_view_);
  CHECK(reveal_reasons_changed_);

  if (!tracker_) {
    tracker_ = TransientWidgetTracker::Create(host_widget_.get());
  }

  host_observation_.Observe(host_widget_);
  focus_observation_.Observe(host_widget_->GetFocusManager());
  tracker_observation_.Observe(tracker_.get());

  Recompute();
}

EdgeRevealWatcher::~EdgeRevealWatcher() = default;

void EdgeRevealWatcher::OnTransientWidgetAdded(views::Widget* widget) {
  CHECK(widget);
  if (!transients_.insert(widget).second) {
    return;
  }
  transient_observations_.AddObservation(widget);
  Recompute();
}

void EdgeRevealWatcher::OnTransientWidgetRemoved(views::Widget* widget) {
  if (transients_.erase(widget) == 0) {
    return;
  }
  transient_observations_.RemoveObservation(widget);
  Recompute();
}

void EdgeRevealWatcher::OnDidChangeFocus(views::View* before,
                                         views::View* now) {
  Recompute();
}

void EdgeRevealWatcher::OnWidgetActivationChanged(views::Widget* widget,
                                                  bool active) {
  Recompute();
}

void EdgeRevealWatcher::OnWidgetVisibilityChanged(views::Widget* widget,
                                                  bool visible) {
  Recompute();
}

void EdgeRevealWatcher::OnWidgetDestroying(views::Widget* widget) {
  if (widget == host_widget_) {
    tracker_observation_.Reset();
    tracker_.reset();
    transient_observations_.RemoveAllObservations();
    transients_.clear();
    focus_observation_.Reset();
    host_observation_.Reset();
    host_widget_ = nullptr;
    return;
  }

  if (transients_.erase(widget) > 0) {
    transient_observations_.RemoveObservation(widget);
    Recompute();
  }
}

bool EdgeRevealWatcher::IsInAnyTarget(const views::View* view) const {
  return view && edge_contains_view_.Run(view);
}

bool EdgeRevealWatcher::IsAnchoredInTarget(views::Widget* widget) const {
  auto* delegate = widget->widget_delegate();
  if (!delegate) {
    return false;
  }
  auto* bubble = delegate->AsBubbleDialogDelegate();
  if (!bubble) {
    return false;
  }
  views::View* anchor = bubble->GetAnchorView();
  return anchor && IsInAnyTarget(anchor);
}

void EdgeRevealWatcher::Recompute() {
  RevealReasons new_reasons;

  if (host_widget_) {
    if (auto* fm = host_widget_->GetFocusManager()) {
      if (IsInAnyTarget(fm->GetFocusedView())) {
        new_reasons.Put(RevealReason::kFocus);
      }
    }
  }

  for (views::Widget* widget : transients_) {
    if (widget->IsVisible()) {
      if (IsAnchoredInTarget(widget)) {
        new_reasons.Put(RevealReason::kAnchoredBubble);
      } else {
        new_reasons.Put(RevealReason::kVisibleTransientChild);
      }
    }
  }

  if (new_reasons == reasons_) {
    return;
  }
  reasons_ = new_reasons;
  reveal_reasons_changed_.Run(reasons_);
}
