/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_FOCUS_MODE_FOCUS_MODE_CONTROLLER_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_FOCUS_MODE_FOCUS_MODE_CONTROLLER_H_

#include "base/memory/raw_ref.h"

namespace views {
class Widget;
}

// Orchestrates Focus Mode for a single browser window.
class FocusModeController {
 public:
  class Delegate {
   public:
    virtual void OnFocusModeChanged(bool enabled) = 0;

   protected:
    virtual ~Delegate() = default;
  };

  FocusModeController(Delegate* delegate, views::Widget* widget);
  FocusModeController(const FocusModeController&) = delete;
  FocusModeController& operator=(const FocusModeController&) = delete;
  ~FocusModeController();

  void SetEnabled(bool enabled);
  bool IsEnabled() const;

 private:
  raw_ref<Delegate> delegate_;
  raw_ref<views::Widget> widget_;
  bool enabled_ = false;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_FOCUS_MODE_FOCUS_MODE_CONTROLLER_H_
