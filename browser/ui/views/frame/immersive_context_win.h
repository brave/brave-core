/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_IMMERSIVE_CONTEXT_WIN_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_IMMERSIVE_CONTEXT_WIN_H_

#include "brave/browser/ui/views/frame/immersive_context.h"

class ImmersiveFullscreenControllerWin;

namespace views {
class Widget;
}

// Windows implementation of ImmersiveContext, whose goal is to abstract away
// the windowing related calls (eg aura) for
// //brave/browser/ui/views/frame/immersive.
class ImmersiveContextWin : ImmersiveContext {
 public:
  ImmersiveContextWin();
  ~ImmersiveContextWin() override;
  ImmersiveContextWin(const ImmersiveContextWin&) = delete;
  ImmersiveContextWin& operator=(const ImmersiveContextWin&) = delete;

  // ImmersiveContext:
  void OnEnteringOrExitingImmersive(
      ImmersiveFullscreenControllerWin* controller,
      bool entering) override;
  gfx::Rect GetDisplayBoundsInScreen(views::Widget* widget) override;
  bool DoesAnyWindowHaveCapture() override;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_IMMERSIVE_CONTEXT_WIN_H_
