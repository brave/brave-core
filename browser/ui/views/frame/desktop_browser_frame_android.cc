// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/browser/ui/views/frame/desktop_browser_frame_android.h"

#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/ui/views/frame/browser_desktop_window_tree_host.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "ui/base/hit_test.h"
#include "ui/base/models/simple_menu_model.h"
#include "ui/gfx/font.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"
#include "ui/wm/core/visibility_controller.h"

using aura::Window;

///////////////////////////////////////////////////////////////////////////////
// DesktopBrowserFrameAura, public:

DesktopBrowserFrameAndroid::DesktopBrowserFrameAndroid(
    BrowserFrame* browser_frame,
    BrowserView* browser_view)
    : browser_view_(browser_view),
      browser_frame_(browser_frame) {
}

///////////////////////////////////////////////////////////////////////////////
// DesktopBrowserFrameAndroid, protected:

DesktopBrowserFrameAndroid::~DesktopBrowserFrameAndroid() {
}

////////////////////////////////////////////////////////////////////////////////
// DesktopBrowserFrameAndroid, NativeBrowserFrame implementation:

views::Widget::InitParams DesktopBrowserFrameAndroid::GetWidgetParams() {
  views::Widget::InitParams params;
  return params;
}

bool DesktopBrowserFrameAndroid::UseCustomFrame() const {
  return true;
}

bool DesktopBrowserFrameAndroid::UsesNativeSystemMenu() const {
  return true;
}

int DesktopBrowserFrameAndroid::GetMinimizeButtonOffset() const {
  return 0;
}

bool DesktopBrowserFrameAndroid::ShouldSaveWindowPlacement() const {
  // The placement can always be stored.
  return true;
}

void DesktopBrowserFrameAndroid::GetWindowPlacement(
    gfx::Rect* bounds,
    ui::WindowShowState* show_state) const {
  *show_state = ui::SHOW_STATE_NORMAL;
}

content::KeyboardEventProcessingResult
DesktopBrowserFrameAndroid::PreHandleKeyboardEvent(
    const content::NativeWebKeyboardEvent& event) {
  return content::KeyboardEventProcessingResult::NOT_HANDLED;
}

bool DesktopBrowserFrameAndroid::HandleKeyboardEvent(
    const content::NativeWebKeyboardEvent& event) {
  return false;
}
