// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_TABS_PUBLIC_VERTICAL_TAB_CONTROLLER_H_
#define BRAVE_BROWSER_UI_TABS_PUBLIC_VERTICAL_TAB_CONTROLLER_H_

#include <map>
#include <memory>
#include <optional>
#include <string>

#include "base/auto_reset.h"
#include "base/callback_list.h"
#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "ui/base/unowned_user_data/scoped_unowned_user_data.h"

class FocusModeController;
class PrefChangeRegistrar;
class PrefService;

class VerticalTabController {
 public:
  DECLARE_USER_DATA(VerticalTabController);

  // Returns the instance owned by `browser`, or nullptr. Also returns nullptr
  // if `browser` is nullptr.
  static VerticalTabController* From(BrowserWindowInterface* browser);
  static const VerticalTabController* From(
      const BrowserWindowInterface* browser);

  // `host` is the UnownedUserDataHost of the browser window this controller
  // belongs to. `browser` is null in unit tests.
  VerticalTabController(ui::UnownedUserDataHost& host,
                        BrowserWindowInterface* browser,
                        BrowserWindowInterface::Type type,
                        PrefService* prefs,
                        FocusModeController* focus_mode_controller);
  VerticalTabController(const VerticalTabController&) = delete;
  VerticalTabController& operator=(const VerticalTabController&) = delete;
  ~VerticalTabController();

  // Returns true if the current |browser| might ever support vertical tabs.
  bool SupportsBraveVerticalTabs() const;

  // Uses this window's override when one is set. Otherwise every window
  // follows the profile pref. An override is cleared once it matches the
  // profile pref, so a set override is a disagreement.
  bool ShouldShowBraveVerticalTabs() const;

  // No-op unless this window supports vertical tabs. Matching the profile pref
  // clears the override. Never writes the profile pref.
  void SetVerticalTabsEnabledForWindow(bool enabled);

  // Drops this window's override so it follows the profile pref. No effect
  // when it is already following the pref.
  void FollowProfilePref();

  // Null when this window follows the profile pref.
  std::optional<bool> vertical_tabs_enabled_override() const {
    return enabled_override_;
  }

  base::CallbackListSubscription AddEnabledChangedCallback(
      base::RepeatingCallback<void()> callback);

  // Session restore: the next normal window uses the override in
  // `extra_data`. Other window types leave a pending override unchanged.
  [[nodiscard]] static base::AutoReset<std::optional<bool>>
  InheritEnabledOverrideFromSession(
      BrowserWindowInterface::Type type,
      const std::map<std::string, std::string>& extra_data);

  // Returns true when we should show window title on window frame when vertical
  // tab strip is enabled.
  bool ShouldShowWindowTitleForVerticalTabs() const;

  // Returns true if we should trigger floating vertical tab strip on mouse
  // over.
  bool IsFloatingVerticalTabsEnabled() const;

  bool IsVerticalTabOnRight() const;

  // Returns true if we should hide vertical tab completely when collapsed. In
  // this case vertical tab strip will be almost a few pixels wide vertical bar
  // when collapsed. And will expand to show tab icons when mouse is over the
  // bar regardless of the setting of kVerticalTabsFloatingEnabled.
  bool ShouldHideVerticalTabsCompletelyWhenCollapsed() const;

  // Returns true when the vertical tab toggle button (the button used to
  // expand/collapse the vertical tab strip) should be shown.
  bool ShouldShowVerticalTabToggleButton() const;

 private:
  friend class VerticalTabControllerUnitTest;

  [[nodiscard]] static base::AutoReset<std::optional<bool>>
  InheritEnabledOverrideForTesting(std::optional<bool> enabled);

  bool EnabledOverrideMatchesProfilePref() const;
  void OnProfilePrefChanged();
  void SyncTreeTabs();
  void WriteEnabledOverrideToSession();

  BrowserWindowInterface::Type type_;
  raw_ptr<BrowserWindowInterface> browser_;
  raw_ptr<PrefService> prefs_;
  raw_ptr<FocusModeController> focus_mode_controller_;
  std::optional<bool> enabled_override_;
  base::RepeatingCallbackList<void()> enabled_changed_callbacks_;
  // Destroyed before `enabled_changed_callbacks_`.
  std::unique_ptr<PrefChangeRegistrar> pref_registrar_;

  ui::ScopedUnownedUserData<VerticalTabController> scoped_unowned_user_data_;
  base::WeakPtrFactory<VerticalTabController> weak_ptr_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_TABS_PUBLIC_VERTICAL_TAB_CONTROLLER_H_
