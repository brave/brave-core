// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/toolbar/wallet_button.h"

#include <utility>

#include "brave/app/vector_icons/vector_icons.h"
#include "brave/browser/ui/brave_view_ids.h"
#include "brave/common/webui_url_constants.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/themes/theme_properties.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_service.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/gfx/paint_vector_icon.h"

WalletButton::WalletButton(View* backup_anchor_view,
                           Profile* profile,
                           PrefService* prefs)
    : ToolbarButton(base::BindRepeating(&WalletButton::OnWalletPressed,
                                        base::Unretained(this))),
      prefs_(prefs),
      backup_anchor_view_(backup_anchor_view),
      profile_(profile) {
  pref_change_registrar_.Init(prefs_);
  pref_change_registrar_.Add(
      kShowWalletIconOnToolbar,
      base::BindRepeating(&WalletButton::OnPreferenceChanged,
                          base::Unretained(this)));
  InitBubbleManagerAnchor();
  UpdateVisibility();

  auto menu_button_controller = std::make_unique<views::MenuButtonController>(
      this,
      base::BindRepeating(&WalletButton::OnWalletPressed,
                          base::Unretained(this)),
      std::make_unique<views::Button::DefaultButtonControllerDelegate>(this));
  menu_button_controller_ = menu_button_controller.get();
  SetButtonController(std::move(menu_button_controller));
}

WalletButton::~WalletButton() = default;

void WalletButton::OnWalletPressed(const ui::Event& event) {
  ShowWalletBubble();
}

void WalletButton::UpdateImageAndText() {
  const ui::ThemeProvider* tp = GetThemeProvider();
  SkColor icon_color = tp->GetColor(ThemeProperties::COLOR_TOOLBAR_BUTTON_ICON);
  SetImage(views::Button::STATE_NORMAL,
           gfx::CreateVectorIcon(kWalletToolbarButtonIcon, icon_color));
  SetTooltipText(l10n_util::GetStringUTF16(IDS_TOOLTIP_WALLET));
}

void WalletButton::InitBubbleManagerAnchor() {
  View* anchor_view = this;
  if (!prefs_->GetBoolean(kShowWalletIconOnToolbar)) {
    anchor_view = backup_anchor_view_;
  }
  webui_bubble_manager_ = std::make_unique<WebUIBubbleManagerT<WalletPanelUI>>(
      anchor_view, profile_, GURL(kBraveUIWalletPanelURL),
      IDS_ACCNAME_BRAVE_WALLET_BUTTON, true);
}

void WalletButton::UpdateVisibility() {
  SetVisible(prefs_->GetBoolean(kShowWalletIconOnToolbar));
}

void WalletButton ::OnPreferenceChanged() {
  InitBubbleManagerAnchor();
  UpdateVisibility();
}

void WalletButton::OnWidgetDestroying(views::Widget* widget) {
  DCHECK_EQ(webui_bubble_manager_->GetBubbleWidget(), widget);
  DCHECK(bubble_widget_observation_.IsObservingSource(
      webui_bubble_manager_->GetBubbleWidget()));
  bubble_widget_observation_.Reset();
  pressed_lock_.reset();
}

bool WalletButton::ShowWalletBubble() {
  if (webui_bubble_manager_->GetBubbleWidget()) {
    CloseWalletBubble();
    return false;
  }

  webui_bubble_manager_->ShowBubble();

  // There should only ever be a single bubble widget active for the
  // WalletButton.
  DCHECK(!bubble_widget_observation_.IsObserving());
  bubble_widget_observation_.Observe(webui_bubble_manager_->GetBubbleWidget());

  // Hold the pressed lock while the |bubble_| is active.
  pressed_lock_ = menu_button_controller_->TakeLock();
  return true;
}

void WalletButton::CloseWalletBubble() {
  webui_bubble_manager_->CloseBubble();
}

BEGIN_METADATA(WalletButton, ToolbarButton)
END_METADATA
