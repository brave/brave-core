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
    RevealReasonsChangedCallback reveal_reasons_changed)
    : host_widget_(host),
      edge_contains_view_(std::move(edge_contains_view)),
      reveal_reasons_changed_(std::move(reveal_reasons_changed)) {
  CHECK(host_widget_);
  CHECK(host_widget_->GetFocusManager());
  CHECK(edge_contains_view_);
  CHECK(reveal_reasons_changed_);

  host_observation_.Observe(host_widget_);
  focus_observation_.Observe(host_widget_->GetFocusManager());

  Recompute();
}

EdgeRevealWatcher::~EdgeRevealWatcher() = default;

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
    child_widget_observations_.RemoveAllObservations();
    focus_observation_.Reset();
    host_observation_.Reset();
    host_widget_ = nullptr;
  } else if (child_widget_observations_.IsObservingSource(widget)) {
    child_widget_observations_.RemoveObservation(widget);
    Recompute();
  }
}

void EdgeRevealWatcher::OnWidgetChildAdded(views::Widget* widget,
                                           views::Widget* child) {
  if (widget != host_widget_) {
    return;
  }
  if (!child_widget_observations_.IsObservingSource(child)) {
    child_widget_observations_.AddObservation(child);
    Recompute();
  }
}

void EdgeRevealWatcher::OnWidgetChildRemoved(views::Widget* widget,
                                             views::Widget* child) {
  if (widget != host_widget_) {
    return;
  }
  if (child_widget_observations_.IsObservingSource(child)) {
    child_widget_observations_.RemoveObservation(child);
    Recompute();
  }
}

bool EdgeRevealWatcher::EdgeContainsView(const views::View* view) const {
  return view && edge_contains_view_.Run(view);
}

bool EdgeRevealWatcher::IsAnchoredToEdge(views::Widget* widget) const {
  auto* delegate = widget->widget_delegate();
  if (!delegate) {
    return false;
  }
  auto* bubble = delegate->AsBubbleDialogDelegate();
  if (!bubble) {
    return false;
  }
  return EdgeContainsView(bubble->GetAnchorView());
}

void EdgeRevealWatcher::Recompute() {
  RevealReasons new_reasons;

  if (host_widget_) {
    if (auto* fm = host_widget_->GetFocusManager()) {
      if (EdgeContainsView(fm->GetFocusedView())) {
        new_reasons.Put(RevealReason::kFocus);
      }
    }
  }

  for (auto widget : child_widget_observations_.sources()) {
    if (widget->IsVisible()) {
      if (IsAnchoredToEdge(widget)) {
        new_reasons.Put(RevealReason::kAnchoredBubble);
      } else {
        new_reasons.Put(RevealReason::kVisibleChildWidget);
      }
    }
  }

  if (new_reasons != reasons_) {
    reasons_ = new_reasons;
    reveal_reasons_changed_.Run(reasons_);
  }
}
