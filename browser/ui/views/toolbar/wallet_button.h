// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_TOOLBAR_WALLET_BUTTON_H_
#define BRAVE_BROWSER_UI_VIEWS_TOOLBAR_WALLET_BUTTON_H_

#include "chrome/browser/ui/views/toolbar/toolbar_button.h"
#include "components/prefs/pref_change_registrar.h"

class PrefService;

class WalletButton : public ToolbarButton {
 public:
  WalletButton(PressedCallback callback, PrefService* prefs);
  ~WalletButton() override;

  WalletButton(const WalletButton&) = delete;
  WalletButton& operator=(const WalletButton&) = delete;

  void UpdateImageAndText();
  void UpdateVisibility();

 private:

  void OnPreferenceChanged();

  // ToolbarButton:
  const char* GetClassName() const override;

  PrefService* prefs_ = nullptr;
  PrefChangeRegistrar pref_change_registrar_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TOOLBAR_WALLET_BUTTON_H_
