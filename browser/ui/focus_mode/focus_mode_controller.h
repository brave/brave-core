/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_FOCUS_MODE_FOCUS_MODE_CONTROLLER_H_
#define BRAVE_BROWSER_UI_FOCUS_MODE_FOCUS_MODE_CONTROLLER_H_

#include "base/observer_list.h"
#include "ui/base/unowned_user_data/scoped_unowned_user_data.h"

class BrowserWindowInterface;

// Orchestrates Focus Mode for a single browser window.
class FocusModeController {
 public:
  class Observer : public base::CheckedObserver {
   public:
    virtual void OnFocusModeToggled(bool enabled) = 0;
  };

  DECLARE_USER_DATA(FocusModeController);

  // `host` is the UnownedUserDataHost of the browser window this controller
  // belongs to.
  explicit FocusModeController(ui::UnownedUserDataHost& host);
  FocusModeController(const FocusModeController&) = delete;
  FocusModeController& operator=(const FocusModeController&) = delete;
  ~FocusModeController();

  // Returns the instance owned by `browser`, or nullptr. Null unless Focus Mode
  // is supported for `browser`.
  static FocusModeController* From(BrowserWindowInterface* browser);
  static const FocusModeController* From(const BrowserWindowInterface* browser);

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  bool IsEnabled() const;
  void SetEnabled(bool enabled);
  void ToggleEnabled();

 private:
  base::ObserverList<Observer> observers_;
  bool enabled_ = false;
  ui::ScopedUnownedUserData<FocusModeController> scoped_unowned_user_data_;
};

#endif  // BRAVE_BROWSER_UI_FOCUS_MODE_FOCUS_MODE_CONTROLLER_H_
