/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_FOCUS_MODE_FOCUS_MODE_CONTROLLER_H_
#define BRAVE_BROWSER_UI_FOCUS_MODE_FOCUS_MODE_CONTROLLER_H_

#include "base/functional/callback_helpers.h"
#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "components/prefs/pref_change_registrar.h"
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

  // `host` is the UnownedUserDataHost of the browser window this controller
  // belongs to. `prefs` is the profile PrefService that backs the Focus Mode
  // enabled state.
  FocusModeController(ui::UnownedUserDataHost& host, PrefService& prefs);
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

  // Temporarily suppresses Focus Mode on this controller until the returned
  // runner is destroyed. While suppressed, `IsEnabled()` returns false.
  [[nodiscard]] base::ScopedClosureRunner Suppress();

 private:
  void EndSuppress();
  void OnPrefChanged();
  void NotifyIfChanged();

  base::ObserverList<Observer> observers_;
  PrefChangeRegistrar pref_change_registrar_;
  int suppress_count_ = 0;
  bool last_notified_enabled_ = false;
  ui::ScopedUnownedUserData<FocusModeController> scoped_unowned_user_data_;
  const raw_ref<PrefService> prefs_;
  base::WeakPtrFactory<FocusModeController> weak_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_FOCUS_MODE_FOCUS_MODE_CONTROLLER_H_
