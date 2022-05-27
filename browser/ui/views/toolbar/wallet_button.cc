// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/toolbar/wallet_button.h"

#include <string>
#include <utility>
#include <vector>

#include "brave/app/vector_icons/vector_icons.h"
#include "brave/browser/brave_wallet/brave_wallet_tab_helper.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/l10n/common/locale_util.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/models/simple_menu_model.h"
#include "ui/gfx/paint_vector_icon.h"

namespace {

content::WebContents* GetActiveWebContents() {
  return BrowserList::GetInstance()
      ->GetLastActive()
      ->tab_strip_model()
      ->GetActiveWebContents();
}

class WalletButtonMenuModel : public ui::SimpleMenuModel,
                              public ui::SimpleMenuModel::Delegate {
 public:
  explicit WalletButtonMenuModel(PrefService* prefs)
      : SimpleMenuModel(this), prefs_(prefs) {
    Build();
  }

  ~WalletButtonMenuModel() override = default;
  WalletButtonMenuModel(const WalletButtonMenuModel&) = delete;
  WalletButtonMenuModel& operator=(const WalletButtonMenuModel&) = delete;

 private:
  enum ContextMenuCommand {
    HideBraveWalletIcon,
  };

  // ui::SimpleMenuModel::Delegate override:
  void ExecuteCommand(int command_id, int event_flags) override {
    if (command_id == HideBraveWalletIcon)
      prefs_->SetBoolean(kShowWalletIconOnToolbar, false);
  }

  void Build() {
    AddItemWithStringId(HideBraveWalletIcon,
                        IDS_HIDE_BRAVE_WALLET_ICON_ON_TOOLBAR);
  }

  raw_ptr<PrefService> prefs_ = nullptr;
};

}  // namespace

WalletButton::WalletButton(View* backup_anchor_view, PrefService* prefs)
    : ToolbarButton(base::BindRepeating(&WalletButton::OnWalletPressed,
                                        base::Unretained(this)),
                    std::make_unique<WalletButtonMenuModel>(prefs),
                    nullptr,
                    false),  // Long-pressing is not intended for something that
                             // already shows a panel on click
      prefs_(prefs),
      backup_anchor_view_(backup_anchor_view) {
  pref_change_registrar_.Init(prefs_);
  pref_change_registrar_.Add(
      kShowWalletIconOnToolbar,
      base::BindRepeating(&WalletButton::OnPreferenceChanged,
                          base::Unretained(this)));

  // The MenuButtonController makes sure the panel closes when clicked if the
  // panel is already open.
  auto menu_button_controller = std::make_unique<views::MenuButtonController>(
      this,
      base::BindRepeating(&WalletButton::OnWalletPressed,
                          base::Unretained(this)),
      std::make_unique<views::Button::DefaultButtonControllerDelegate>(this));
  menu_button_controller_ = menu_button_controller.get();
  SetButtonController(std::move(menu_button_controller));

  UpdateVisibility();
}

WalletButton::~WalletButton() = default;

void WalletButton::OnWalletPressed(const ui::Event& event) {
  if (IsShowingBubble()) {
    CloseWalletBubble();
    return;
  }

  ShowWalletBubble();
}

void WalletButton::UpdateImageAndText() {
  const ui::ThemeProvider* tp = GetThemeProvider();
  SkColor icon_color = tp->GetColor(ThemeProperties::COLOR_TOOLBAR_BUTTON_ICON);
  SetImage(views::Button::STATE_NORMAL,
           gfx::CreateVectorIcon(kWalletToolbarButtonIcon, icon_color));
  SetTooltipText(
      brave_l10n::GetLocalizedResourceUTF16String(IDS_TOOLTIP_WALLET));
}

void WalletButton::UpdateVisibility() {
  SetVisible(prefs_->GetBoolean(kShowWalletIconOnToolbar));
}

void WalletButton ::OnPreferenceChanged() {
  UpdateVisibility();
}

void WalletButton::ShowWalletBubble() {
  brave_wallet::BraveWalletTabHelper::FromWebContents(GetActiveWebContents())
      ->ShowBubble();
}

void WalletButton::ShowApproveWalletBubble() {
  brave_wallet::BraveWalletTabHelper::FromWebContents(GetActiveWebContents())
      ->ShowApproveWalletBubble();
}

void WalletButton::CloseWalletBubble() {
  brave_wallet::BraveWalletTabHelper::FromWebContents(GetActiveWebContents())
      ->CloseBubble();
}

bool WalletButton::IsShowingBubble() {
  return brave_wallet::BraveWalletTabHelper::FromWebContents(
             GetActiveWebContents())
      ->IsShowingBubble();
}

bool WalletButton::IsBubbleClosedForTesting() {
  return brave_wallet::BraveWalletTabHelper::FromWebContents(
             GetActiveWebContents())
      ->IsBubbleClosedForTesting();
}

views::View* WalletButton::GetAsAnchorView() {
  View* anchor_view = this;
  if (!prefs_->GetBoolean(kShowWalletIconOnToolbar))
    anchor_view = backup_anchor_view_;
  return anchor_view;
}

BEGIN_METADATA(WalletButton, ToolbarButton)
END_METADATA
