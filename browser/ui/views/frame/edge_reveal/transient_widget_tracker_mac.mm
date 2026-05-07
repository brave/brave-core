/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/edge_reveal/transient_widget_tracker.h"

#import <AppKit/AppKit.h>

#include <memory>

#include "base/apple/foundation_util.h"
#include "base/check.h"
#include "components/remote_cocoa/app_shim/native_widget_mac_nswindow.h"
#include "ui/views/widget/widget.h"

namespace {

class TransientWidgetTrackerMac : public TransientWidgetTracker {
 public:
  explicit TransientWidgetTrackerMac(views::Widget* host)
      : TransientWidgetTracker(host) {
    NSWindow* native = host->GetNativeWindow().GetNativeNSWindow();
    host_window_ = base::apple::ObjCCastStrict<NativeWidgetMacNSWindow>(native);

    CHECK(host_window_.childWindowAddedHandler == nil);
    CHECK(host_window_.childWindowRemovedHandler == nil);

    TransientWidgetTrackerMac* unsafe_self = this;
    host_window_.childWindowAddedHandler = ^(NSWindow* child) {
      unsafe_self->OnChildAdded(child);
    };
    host_window_.childWindowRemovedHandler = ^(NSWindow* child) {
      unsafe_self->OnChildRemoved(child);
    };

    for (NSWindow* child in host_window_.childWindows) {
      OnChildAdded(child);
    }
  }

  TransientWidgetTrackerMac(const TransientWidgetTrackerMac&) = delete;
  TransientWidgetTrackerMac& operator=(const TransientWidgetTrackerMac&) =
      delete;

  ~TransientWidgetTrackerMac() override { ReleasePlatformObservation(); }

 protected:
  void OnHostDestroying() override { ReleasePlatformObservation(); }

 private:
  void OnChildAdded(NSWindow* child) {
    if (auto* widget =
            views::Widget::GetWidgetForNativeWindow(gfx::NativeWindow(child))) {
      NotifyAdded(widget);
    }
  }

  void OnChildRemoved(NSWindow* child) {
    if (auto* widget =
            views::Widget::GetWidgetForNativeWindow(gfx::NativeWindow(child))) {
      NotifyRemoved(widget);
    }
  }

  void ReleasePlatformObservation() {
    if (!host_window_) {
      return;
    }
    host_window_.childWindowAddedHandler = nil;
    host_window_.childWindowRemovedHandler = nil;
    host_window_ = nil;
  }

  NativeWidgetMacNSWindow* __strong host_window_ = nil;
};

}  // namespace

std::unique_ptr<TransientWidgetTracker> TransientWidgetTracker::Create(
    views::Widget* host) {
  return std::make_unique<TransientWidgetTrackerMac>(host);
}
