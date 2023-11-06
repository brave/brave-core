/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_IMMERSIVE_FULLSCREEN_CONTROLLER_DELEGATE_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_IMMERSIVE_FULLSCREEN_CONTROLLER_DELEGATE_H_

#include <vector>

#include "base/component_export.h"

namespace gfx {
class Rect;
}

class ImmersiveFullscreenControllerDelegate {
 public:
  // Called when a reveal of the top-of-window views starts.
  virtual void OnImmersiveRevealStarted() = 0;

  // Called when the top-of-window views have finished closing. This call
  // implies a visible fraction of 0. SetVisibleFraction(0) may not be called
  // prior to OnImmersiveRevealEnded().
  virtual void OnImmersiveRevealEnded() = 0;

  // Called as a result of enabling immersive fullscreen via SetEnabled().
  virtual void OnImmersiveFullscreenEntered() = 0;

  // Called as a result of disabling immersive fullscreen via SetEnabled().
  virtual void OnImmersiveFullscreenExited() = 0;

  // Called to update the fraction of the top-of-window views height which is
  // visible.
  virtual void SetVisibleFraction(double visible_fraction) = 0;

  // Returns a list of rects whose union makes up the top-of-window views.
  // The returned list is used for hittesting when the top-of-window views
  // are revealed. GetVisibleBoundsInScreen() must return a valid value when
  // not in immersive fullscreen for the sake of SetupForTest().
  virtual std::vector<gfx::Rect> GetVisibleBoundsInScreen() const = 0;

  // Re-layout the frame. Called when |EnableForWidget| is called
  // but full state did not change.
  virtual void Relayout() {}

 protected:
  virtual ~ImmersiveFullscreenControllerDelegate() {}
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_IMMERSIVE_FULLSCREEN_CONTROLLER_DELEGATE_H_
