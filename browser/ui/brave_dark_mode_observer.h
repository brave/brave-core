/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_BRAVE_DARK_MODE_OBSERVER_H_
#define BRAVE_BROWSER_UI_BRAVE_DARK_MODE_OBSERVER_H_

#include "base/gtest_prod_util.h"
#include "chrome/browser/ui/dark_mode_observer.h"

// This class is introduced to handle two different native themes that brave
// uses for brave theme. DarkModeObserver only observes default NativeTheme.
// However, brave also uses NativeDarkThemeAura for dark theme.
// So, DarkModeObserver should also observe NativeDarkThemeAura when current
// active brave theme is dark.
// Observed NativeTheme is changed when native theme is updated.
class BraveDarkModeObserver : public DarkModeObserver {
 public:
  using DarkModeObserver::DarkModeObserver;

  void Start() override;

 private:
  FRIEND_TEST_ALL_PREFIXES(BraveDarkModeObserverTest,
                           ObserveProperNativeThemeTest);

  // DarkModeObserver overrides:
  void OnNativeThemeUpdated(ui::NativeTheme* observed_theme) override;

  void ResetThemeObserver();

  static ui::NativeTheme* current_native_theme_for_testing_;

  DISALLOW_COPY_AND_ASSIGN(BraveDarkModeObserver);
};

#endif  // BRAVE_BROWSER_UI_BRAVE_DARK_MODE_OBSERVER_H_
