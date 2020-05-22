// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/ui/views/controls/native/native_view_host_android.h"

#include <memory>

#include "base/logging.h"
#include "base/optional.h"
#include "build/build_config.h"
#include "ui/base/cursor/cursor.h"
#include "ui/base/hit_test.h"
#include "ui/compositor/paint_recorder.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/views/controls/native/native_view_host.h"
#include "ui/views/painter.h"
#include "ui/views/view_class_properties.h"
#include "ui/views/widget/widget.h"

namespace views {

NativeViewHostAndroid::NativeViewHostAndroid(NativeViewHost* host) : host_(host) {}

NativeViewHostAndroid::~NativeViewHostAndroid() {
}

////////////////////////////////////////////////////////////////////////////////
// NativeViewHostAura, NativeViewHostWrapper implementation:
void NativeViewHostAndroid::AttachNativeView() {
}

void NativeViewHostAndroid::SetParentAccessible(
    gfx::NativeViewAccessible accessible) {
}

void NativeViewHostAndroid::NativeViewDetaching(bool destroyed) {
}

void NativeViewHostAndroid::AddedToWidget() {
}

void NativeViewHostAndroid::RemovedFromWidget() {
}

bool NativeViewHostAndroid::SetCustomMask(std::unique_ptr<ui::LayerOwner> mask) {
  return false;
}

void NativeViewHostAndroid::SetHitTestTopInset(int top_inset) {
}

void NativeViewHostAndroid::InstallClip(int x, int y, int w, int h) {
}

int NativeViewHostAndroid::GetHitTestTopInset() const {
  return top_inset_;
}

bool NativeViewHostAndroid::HasInstalledClip() {
  return !!clip_rect_;
}

void NativeViewHostAndroid::UninstallClip() {
  clip_rect_.reset();
}

void NativeViewHostAndroid::ShowWidget(int x,
                                    int y,
                                    int w,
                                    int h,
                                    int native_w,
                                    int native_h) {
}

void NativeViewHostAndroid::HideWidget() {
}

void NativeViewHostAndroid::SetFocus() {
}

gfx::NativeView NativeViewHostAndroid::GetNativeViewContainer() const {
  return gfx::NativeView();
}

gfx::NativeViewAccessible NativeViewHostAndroid::GetNativeViewAccessible() {
  return nullptr;
}

gfx::NativeCursor NativeViewHostAndroid::GetCursor(int x, int y) {
  return gfx::kNullCursor;
}

void NativeViewHostAndroid::SetVisible(bool visible) {
}

// static
NativeViewHostWrapper* NativeViewHostWrapper::CreateWrapper(
    NativeViewHost* host) {
  return new NativeViewHostAndroid(host);
}

void NativeViewHostAndroid::CreateClippingWindow() {
}

void NativeViewHostAndroid::AddClippingWindow() {
}

void NativeViewHostAndroid::RemoveClippingWindow() {
}

void NativeViewHostAndroid::InstallMask() {
}

void NativeViewHostAndroid::UninstallMask() {
}

void NativeViewHostAndroid::UpdateInsets() {
}

}  // namespace views
