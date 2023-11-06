/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/immersive_context_win.h"

#include "brave/browser/ui/views/frame/immersive_fullscreen_controller_win.h"
#include "ui/aura/window.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/views/controls/menu/menu_controller.h"
#include "ui/views/widget/widget.h"

ImmersiveContextWin::ImmersiveContextWin() = default;

ImmersiveContextWin::~ImmersiveContextWin() = default;

void ImmersiveContextWin::OnEnteringOrExitingImmersive(
    ImmersiveFullscreenControllerWin* controller,
    bool entering) {}

gfx::Rect ImmersiveContextWin::GetDisplayBoundsInScreen(views::Widget* widget) {
  display::Display display =
      display::Screen::GetScreen()->GetDisplayNearestWindow(
          widget->GetNativeWindow());
  return display.bounds();
}

bool ImmersiveContextWin::DoesAnyWindowHaveCapture() {
  return views::MenuController::GetActiveInstance() != nullptr;
}
