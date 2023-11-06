/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_IMMERSIVE_CONTEXT_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_IMMERSIVE_CONTEXT_H_

namespace gfx {
class Rect;
}

namespace views {
class Widget;
}

class ImmersiveFullscreenControllerWin;

// ImmersiveContext abstracts away all the windowing related calls so that
// ImmersiveFullscreenController does not depend upon aura.
class ImmersiveContext {
 public:
  virtual ~ImmersiveContext();

  // Returns the singleton instance.
  static ImmersiveContext* Get();

  // Used to setup state necessary for entering or existing immersive mode.
  virtual void OnEnteringOrExitingImmersive(
      ImmersiveFullscreenControllerWin* controller,
      bool entering) = 0;

  // Returns the bounds of the display the widget is on, in screen coordinates.
  virtual gfx::Rect GetDisplayBoundsInScreen(views::Widget* widget) = 0;

  // Returns true if any window has capture.
  virtual bool DoesAnyWindowHaveCapture() = 0;

 protected:
  ImmersiveContext();
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_IMMERSIVE_CONTEXT_H_
