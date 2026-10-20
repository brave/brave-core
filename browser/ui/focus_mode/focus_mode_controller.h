/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_FOCUS_MODE_FOCUS_MODE_CONTROLLER_H_
#define BRAVE_BROWSER_UI_FOCUS_MODE_FOCUS_MODE_CONTROLLER_H_

#include "base/callback_list.h"
#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "ui/base/unowned_user_data/scoped_unowned_user_data.h"

class BrowserWindowInterface;
class PrefService;

// Orchestrates Focus Mode for a single browser window.
class FocusModeController {
 public:
  class Observer : public base::CheckedObserver {
   public:
    virtual void OnFocusModeToggled(bool enabled) = 0;
  };

  DECLARE_USER_DATA(FocusModeController);

  // `browser` is the browser window this controller belongs to. `prefs` is the
  // profile PrefService used to initialize and persist the Focus Mode state.
  FocusModeController(BrowserWindowInterface& browser, PrefService* prefs);

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
  void OnDidBecomeActive(BrowserWindowInterface* browser);
  void UpdatePref();

  base::ObserverList<Observer> observers_;
  bool enabled_ = false;
  base::CallbackListSubscription did_become_active_subscription_;
  ui::ScopedUnownedUserData<FocusModeController> scoped_unowned_user_data_;
  raw_ptr<PrefService> prefs_ = nullptr;
};

#endif  // BRAVE_BROWSER_UI_FOCUS_MODE_FOCUS_MODE_CONTROLLER_H_
