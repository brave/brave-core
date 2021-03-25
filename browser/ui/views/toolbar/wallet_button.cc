// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/toolbar/wallet_button.h"

#include "brave/app/brave_command_ids.h"
#include "brave/app/vector_icons/vector_icons.h"
#include "brave/browser/ui/brave_view_ids.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "chrome/browser/themes/theme_properties.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_service.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/paint_vector_icon.h"

WalletButton::WalletButton(PressedCallback callback,
                           PrefService* prefs)
    : ToolbarButton(callback), prefs_(prefs) {
  SetID(BRAVE_VIEW_ID_WALLET_BUTTON);
  set_tag(IDC_TOGGLE_WALLET);

  pref_change_registrar_.Init(prefs_);
  pref_change_registrar_.Add(
      kShowWalletIcon,
      base::BindRepeating(&WalletButton::OnPreferenceChanged,
                          base::Unretained(this)));
  UpdateVisibility();
}

WalletButton::~WalletButton() = default;

const char* WalletButton::GetClassName() const {
  return "WalletButton";
}

void WalletButton::UpdateImageAndText() {
  const ui::ThemeProvider* tp = GetThemeProvider();
  SkColor icon_color = tp->GetColor(ThemeProperties::COLOR_TOOLBAR_BUTTON_ICON);
  SetImage(views::Button::STATE_NORMAL,
           gfx::CreateVectorIcon(kWalletToolbarButtonIcon, icon_color));
  SetTooltipText(l10n_util::GetStringUTF16(IDS_TOOLTIP_WALLET));
}

void WalletButton::UpdateVisibility() {
  SetVisible(prefs_->GetBoolean(kShowWalletIcon));
}

void WalletButton ::OnPreferenceChanged() {
  UpdateVisibility();
}
