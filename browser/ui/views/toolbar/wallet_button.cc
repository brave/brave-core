// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/toolbar/wallet_button.h"

#include <vector>

#include "brave/app/vector_icons/vector_icons.h"
#include "brave/browser/brave_wallet/brave_wallet_tab_helper.h"
#include "brave/browser/ui/views/brave_icon_with_badge_image_source.h"
#include "brave/browser/ui/views/ui_utils.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/l10n/common/localization_util.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/models/simple_menu_model.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/layout/fill_layout.h"

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

const ui::ColorProvider* GetColorProviderForView(
    base::WeakPtr<WalletButton> view) {
  if (view) {
    return view->GetColorProvider();
  }

  return ui::ColorProviderManager::Get().GetColorProviderFor(
      ui::NativeTheme::GetInstanceForNativeUi()->GetColorProviderKey(nullptr));
}

}  // namespace

WalletButton::WalletButton(View* backup_anchor_view, Profile* profile)
    : ToolbarButton(
          base::BindRepeating(&WalletButton::OnWalletPressed,
                              base::Unretained(this)),
          std::make_unique<WalletButtonMenuModel>(profile->GetPrefs()),
          nullptr,
          false),  // Long-pressing is not intended for something that
                   // already shows a panel on click
      prefs_(profile->GetPrefs()),
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

  if (brave_wallet::ShouldShowTxStatusInToolbar()) {
    notification_source_ =
        std::make_unique<brave::WalletButtonNotificationSource>(
            profile, base::BindRepeating(&WalletButton::OnNotificationUpdate,
                                         weak_ptr_factory_.GetWeakPtr()));
  }
}

WalletButton::~WalletButton() = default;

void WalletButton::OnWalletPressed(const ui::Event& event) {
  if (IsShowingBubble()) {
    CloseWalletBubble();
    return;
  }

  ShowWalletBubble();
  notification_source_->MarkWalletButtonWasClicked();
}

void WalletButton::OnNotificationUpdate(bool show_suggest_badge,
                                        size_t counter) {
  show_suggest_badge_ = show_suggest_badge;
  counter_ = counter;
  UpdateImageAndText();
}

std::pair<std::string, SkColor> WalletButton::GetBadgeTextAndBackground() {
  if (counter_ > 0) {
    std::string text = counter_ > 99 ? "99+" : base::NumberToString(counter_);
    return {text, brave::kBadgeNotificationBG};
  }
  return {"", brave::kBadgeNotificationBG};
}

void WalletButton::UpdateImageAndText() {
  const ui::ColorProvider* color_provider = GetColorProvider();
  SkColor icon_color = color_provider->GetColor(kColorToolbarButtonIcon);
  auto icon = gfx::CreateVectorIcon(kWalletToolbarButtonIcon, icon_color);

  size_t icon_size = std::max(icon.width(), icon.height());
  auto badge_size = brave::BraveIconWithBadgeImageSource::GetBadgeSize();
  gfx::Size preferred_size(icon_size + badge_size.width() / 2,
                           icon_size + badge_size.height() / 2);

  auto image_source = std::make_unique<brave::BraveIconWithBadgeImageSource>(
      preferred_size,
      base::BindRepeating(&GetColorProviderForView,
                          weak_ptr_factory_.GetWeakPtr()),
      icon_size, 5u);
  image_source->SetAllowEmptyText(show_suggest_badge_);
  image_source->SetIcon(gfx::Image(icon));

  auto [text, background_color] = GetBadgeTextAndBackground();
  image_source->SetBadge(std::make_unique<IconWithBadgeImageSource::Badge>(
      text, brave::kBadgeTextColor, background_color));
  SetImage(views::Button::STATE_NORMAL,
           gfx::ImageSkia(std::move(image_source), preferred_size));
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
