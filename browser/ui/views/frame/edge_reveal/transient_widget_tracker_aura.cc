/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/check.h"
#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "brave/browser/ui/views/frame/edge_reveal/transient_widget_tracker.h"
#include "ui/aura/client/transient_window_client.h"
#include "ui/aura/client/transient_window_client_observer.h"
#include "ui/aura/window.h"
#include "ui/views/widget/widget.h"

namespace {

class TransientWidgetTrackerAura
    : public TransientWidgetTracker,
      public aura::client::TransientWindowClientObserver {
 public:
  explicit TransientWidgetTrackerAura(views::Widget* host)
      : TransientWidgetTracker(host), host_window_(host->GetNativeWindow()) {
    CHECK(host_window_);
    auto* client = aura::client::GetTransientWindowClient();
    client_observation_.Observe(client);
    for (aura::Window* transient : client->GetTransientChildren(host_window_)) {
      if (auto* widget = views::Widget::GetWidgetForNativeView(transient)) {
        NotifyAdded(widget);
      }
    }
  }

  TransientWidgetTrackerAura(const TransientWidgetTrackerAura&) = delete;
  TransientWidgetTrackerAura& operator=(const TransientWidgetTrackerAura&) =
      delete;

  ~TransientWidgetTrackerAura() override = default;

 protected:
  void OnHostDestroying() override {
    client_observation_.Reset();
    host_window_ = nullptr;
  }

 private:
  void OnTransientChildWindowAdded(aura::Window* parent,
                                   aura::Window* transient) override {
    if (parent != host_window_) {
      return;
    }
    if (auto* widget = views::Widget::GetWidgetForNativeView(transient)) {
      NotifyAdded(widget);
    }
  }

  void OnTransientChildWindowRemoved(aura::Window* parent,
                                     aura::Window* transient) override {
    if (parent != host_window_) {
      return;
    }
    if (auto* widget = views::Widget::GetWidgetForNativeView(transient)) {
      NotifyRemoved(widget);
    }
  }

  raw_ptr<aura::Window> host_window_;
  base::ScopedObservation<aura::client::TransientWindowClient,
                          aura::client::TransientWindowClientObserver>
      client_observation_{this};
};

}  // namespace

std::unique_ptr<TransientWidgetTracker> TransientWidgetTracker::Create(
    views::Widget* host) {
  return std::make_unique<TransientWidgetTrackerAura>(host);
}
