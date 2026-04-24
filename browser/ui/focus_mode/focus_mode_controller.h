/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_FOCUS_MODE_FOCUS_MODE_CONTROLLER_H_
#define BRAVE_BROWSER_UI_FOCUS_MODE_FOCUS_MODE_CONTROLLER_H_

#include "base/observer_list.h"

// Orchestrates Focus Mode for a single browser window.
class FocusModeController {
 public:
  class Observer : public base::CheckedObserver {
   public:
    virtual void OnFocusModeToggled(bool enabled) = 0;
  };

  FocusModeController();
  FocusModeController(const FocusModeController&) = delete;
  FocusModeController& operator=(const FocusModeController&) = delete;
  ~FocusModeController();

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  bool IsEnabled() const;
  void SetEnabled(bool enabled);
  void ToggleEnabled();

 private:
  base::ObserverList<Observer> observers_;
  bool enabled_ = false;
};

#endif  // BRAVE_BROWSER_UI_FOCUS_MODE_FOCUS_MODE_CONTROLLER_H_
