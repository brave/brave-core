/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/edge_reveal/transient_widget_tracker.h"

#include "base/check.h"
#include "ui/views/widget/widget.h"

TransientWidgetTracker::TransientWidgetTracker(views::Widget* host)
    : host_(host) {
  CHECK(host_);
  host_observation_.Observe(host_);
}

TransientWidgetTracker::~TransientWidgetTracker() = default;

void TransientWidgetTracker::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void TransientWidgetTracker::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void TransientWidgetTracker::NotifyAdded(views::Widget* widget) {
  CHECK(widget);
  if (!widgets_.insert(widget).second) {
    return;
  }
  for (auto& observer : observers_) {
    observer.OnTransientWidgetAdded(widget);
  }
}

void TransientWidgetTracker::NotifyRemoved(views::Widget* widget) {
  if (widgets_.erase(widget) == 0) {
    return;
  }
  for (auto& observer : observers_) {
    observer.OnTransientWidgetRemoved(widget);
  }
}

void TransientWidgetTracker::OnWidgetDestroying(views::Widget* widget) {
  if (widget != host_) {
    return;
  }
  OnHostDestroying();
  widgets_.clear();
  host_observation_.Reset();
  host_ = nullptr;
}
