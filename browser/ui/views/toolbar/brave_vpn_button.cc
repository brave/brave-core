/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/toolbar/brave_vpn_button.h"

#include <optional>
#include <utility>

#include "base/memory/raw_ptr.h"
#include "base/notreached.h"
#include "brave/app/brave_command_ids.h"
#include "brave/app/vector_icons/vector_icons.h"
#include "brave/browser/brave_vpn/brave_vpn_service_factory.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/components/brave_vpn/browser/brave_vpn_service.h"
#include "brave/components/l10n/common/localization_util.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/views/chrome_layout_provider.h"
#include "chrome/browser/ui/views/toolbar/toolbar_ink_drop_util.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/compositor/layer.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/color_utils.h"
#include "ui/gfx/geometry/rect_f.h"
#include "ui/gfx/geometry/rrect_f.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/gfx/skia_util.h"
#include "ui/menus/simple_menu_model.h"
#include "ui/views/animation/ink_drop.h"
#include "ui/views/animation/ink_drop_host.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/controls/highlight_path_generator.h"

using ConnectionState = brave_vpn::mojom::ConnectionState;
using PurchasedState = brave_vpn::mojom::PurchasedState;

namespace {

// For error icon's inner color.
class ConnectErrorIconBackground : public views::Background {
 public:
  explicit ConnectErrorIconBackground(SkColor color) {
    SetNativeControlColor(color);
  }

  ConnectErrorIconBackground(const ConnectErrorIconBackground&) = delete;
  ConnectErrorIconBackground& operator=(const ConnectErrorIconBackground&) =
      delete;

  void Paint(gfx::Canvas* canvas, views::View* view) const override {
    auto bounds = view->GetLocalBounds();
    bounds.Inset(gfx::Insets::TLBR(2, 4, 2, 4));
    canvas->FillRect(bounds, get_color());
  }
};

class VPNButtonMenuModel : public ui::SimpleMenuModel,
                           public ui::SimpleMenuModel::Delegate,
                           public brave_vpn::BraveVPNServiceObserver {
 public:
  explicit VPNButtonMenuModel(Browser* browser)
      : SimpleMenuModel(this),
        browser_(browser),
        service_(brave_vpn::BraveVpnServiceFactory::GetForProfile(
            browser_->profile())) {
    CHECK(service_);
    Observe(service_);
    Build(service_->is_purchased_user());
  }

  ~VPNButtonMenuModel() override = default;
  VPNButtonMenuModel(const VPNButtonMenuModel&) = delete;
  VPNButtonMenuModel& operator=(const VPNButtonMenuModel&) = delete;

 private:
  // ui::SimpleMenuModel::Delegate override:
  void ExecuteCommand(int command_id, int event_flags) override {
    chrome::ExecuteCommand(browser_, command_id);
  }

  // BraveVPNServiceObserver overrides:
  void OnPurchasedStateChanged(
      brave_vpn::mojom::PurchasedState state,
      const std::optional<std::string>& description) override {
    // Rebuild menu items based on purchased state change.
    Build(service_->is_purchased_user());
  }

  void Build(bool purchased) {
    // Clear all menu items and re-build as purchased state can be updated
    // during the runtime.
    Clear();
    AddItemWithStringId(IDC_TOGGLE_BRAVE_VPN_TOOLBAR_BUTTON,
                        IDS_BRAVE_VPN_HIDE_VPN_BUTTON_MENU_ITEM);
    if (purchased) {
      AddItemWithStringId(IDC_SEND_BRAVE_VPN_FEEDBACK,
                          IDS_BRAVE_VPN_SHOW_FEEDBACK_MENU_ITEM);
      AddItemWithStringId(IDC_ABOUT_BRAVE_VPN,
                          IDS_BRAVE_VPN_ABOUT_VPN_MENU_ITEM);
      AddItemWithStringId(IDC_MANAGE_BRAVE_VPN_PLAN,
                          IDS_BRAVE_VPN_MANAGE_MY_PLAN_MENU_ITEM);
    }
  }

  raw_ptr<Browser> browser_ = nullptr;
  raw_ptr<brave_vpn::BraveVpnService> service_ = nullptr;
};

}  // namespace

BraveVPNButton::BraveVPNButton(Browser* browser)
    : ToolbarButton(base::BindRepeating(&BraveVPNButton::OnButtonPressed,
                                        base::Unretained(this)),
                    std::make_unique<VPNButtonMenuModel>(browser),
                    nullptr,
                    false),  // Long-pressing is not intended for something that
                             // already shows a panel on click
      browser_(browser),
      service_(brave_vpn::BraveVpnServiceFactory::GetForProfile(
          browser_->profile())) {
  CHECK(service_);
  UpdateButtonState();
  Observe(service_);

  // The MenuButtonController makes sure the panel closes when clicked if the
  // panel is already open.
  auto menu_button_controller = std::make_unique<views::MenuButtonController>(
      this,
      base::BindRepeating(&BraveVPNButton::OnButtonPressed,
                          base::Unretained(this)),
      std::make_unique<views::Button::DefaultButtonControllerDelegate>(this));
  menu_button_controller_ = menu_button_controller.get();
  SetButtonController(std::move(menu_button_controller));

  SetTextSubpixelRenderingEnabled(false);
  label()->SetText(brave_l10n::GetLocalizedResourceUTF16String(
      IDS_BRAVE_VPN_TOOLBAR_BUTTON_TEXT));
  gfx::FontList font_list = views::Label::GetDefaultFontList();
  constexpr int kFontSize = 12;
  label()->SetFontList(
      font_list.DeriveWithSizeDelta(kFontSize - font_list.GetFontSize()));

  // W/o layer, ink drop affects text color.
  label()->SetPaintToLayer();

  // To clear previous pixels.
  label()->layer()->SetFillsBoundsOpaquely(false);

  // Set image positions first. then label.
  SetHorizontalAlignment(gfx::ALIGN_LEFT);

  // Views resulting in focusable nodes later on in the accessibility tree need
  // to have an accessible name for screen readers to see what they are about.
  // TODO(simonhong): Re-visit this name.
  SetAccessibleName(brave_l10n::GetLocalizedResourceUTF16String(
      IDS_BRAVE_VPN_TOOLBAR_BUTTON_TEXT));

  constexpr int kBraveAvatarImageLabelSpacing = 4;
  SetImageLabelSpacing(kBraveAvatarImageLabelSpacing);
}

BraveVPNButton::~BraveVPNButton() = default;

void BraveVPNButton::OnConnectionStateChanged(ConnectionState state) {
  if (IsErrorState() && (state == ConnectionState::CONNECTING ||
                         state == ConnectionState::DISCONNECTING)) {
    // Skip attempts to connect/disconnet if we had an error before and keep
    // the button in the error state until we get it clearly fixed.
    return;
  }
  UpdateButtonState();
  UpdateColorsAndInsets();
}

void BraveVPNButton::UpdateButtonState() {
  is_error_state_ = IsConnectError();
  is_connected_ = IsConnected();
}

void BraveVPNButton::OnPurchasedStateChanged(
    brave_vpn::mojom::PurchasedState state,
    const std::optional<std::string>& description) {
  UpdateButtonState();
  UpdateColorsAndInsets();
}

std::unique_ptr<views::Border> BraveVPNButton::GetBorder(
    SkColor border_color) const {
  constexpr auto kTargetInsets = gfx::Insets::VH(6, 8);
  constexpr auto kBorderThickness = 1;
  const int radius = ChromeLayoutProvider::Get()->GetCornerRadiusMetric(
      views::Emphasis::kMaximum, {});
  std::unique_ptr<views::Border> border = views::CreateRoundedRectBorder(
      kBorderThickness, radius, gfx::Insets(), border_color);
  const gfx::Insets extra_insets = kTargetInsets - border->GetInsets();
  return views::CreatePaddedBorder(std::move(border), extra_insets);
}

void BraveVPNButton::UpdateColorsAndInsets() {
  ui::ColorProvider* cp = GetColorProvider();
  if (!cp) {
    return;
  }

  const auto bg_color =
      cp->GetColor(is_error_state_ ? kColorBraveVpnButtonErrorBackgroundNormal
                                   : kColorBraveVpnButtonBackgroundNormal);
  const int radius = ChromeLayoutProvider::Get()->GetCornerRadiusMetric(
      views::Emphasis::kMaximum, {});
  SetBackground(views::CreateRoundedRectBackground(bg_color, radius));

  SetEnabledTextColors(cp->GetColor(is_error_state_
                                        ? kColorBraveVpnButtonTextError
                                        : kColorBraveVpnButtonText));
  if (is_error_state_) {
    SetImageModel(views::Button::STATE_NORMAL,
                  ui::ImageModel::FromVectorIcon(
                      kVpnIndicatorErrorIcon,
                      cp->GetColor(kColorBraveVpnButtonIconError)));

    // Use background for inner color of error button image.
    image_container_view()->SetBackground(
        std::make_unique<ConnectErrorIconBackground>(
            cp->GetColor(kColorBraveVpnButtonIconErrorInner)));
  } else {
    SetImageModel(
        views::Button::STATE_NORMAL,
        ui::ImageModel::FromVectorIcon(
            is_connected_ ? kVpnIndicatorOnIcon : kVpnIndicatorOffIcon,
            cp->GetColor(is_connected_
                             ? kColorBraveVpnButtonIconConnected
                             : kColorBraveVpnButtonIconDisconnected)));

    // Use background for inner color of button image.
    // Adjusted border thickness to make invisible to the outside of the icon.
    image_container_view()->SetBackground(views::CreateRoundedRectBackground(
        cp->GetColor(kColorBraveVpnButtonIconInner), 5 /*radi*/, 2 /*thick*/));
  }

  // Compute highlight color and border in advance. If not, highlight color and
  // border color are mixed as both have alpha value.
  // Draw border only for error state.
  SetBorder(GetBorder(color_utils::GetResultingPaintColor(
      cp->GetColor(is_error_state_ ? kColorBraveVpnButtonErrorBorder
                                   : kColorBraveVpnButtonBorder),
      bg_color)));

  auto* ink_drop_host = views::InkDrop::Get(this);

  // Use different ink drop hover color for each themes.
  auto target_base_color = color_utils::GetResultingPaintColor(
      cp->GetColor(is_error_state_ ? kColorBraveVpnButtonErrorBackgroundHover
                                   : kColorBraveVpnButtonBackgroundHover),
      bg_color);
  bool need_ink_drop_color_update =
      target_base_color != ink_drop_host->GetBaseColor();

  // Update ink drop color if needed because we toggle ink drop mode below after
  // set base color. Toggling could cause subtle flicking.
  if (!need_ink_drop_color_update) {
    return;
  }

  views::InkDrop::Get(this)->SetBaseColor(target_base_color);

  // Hack to update inkdrop color immediately.
  // W/o this, background color and image are changed but inkdrop color is still
  // using previous one till button state is changed after changing base color.
  const auto previous_ink_drop_state =
      views::InkDrop::Get(this)->GetInkDrop()->GetTargetInkDropState();
  views::InkDrop::Get(this)->SetMode(views::InkDropHost::InkDropMode::OFF);
  views::InkDrop::Get(this)->SetMode(views::InkDropHost::InkDropMode::ON);
  // After toggling, ink drop state is reset. So need to re-apply previous
  // state.
  if (previous_ink_drop_state == views::InkDropState::ACTIVATED) {
    views::InkDrop::Get(this)->GetInkDrop()->SnapToActivated();
  }
}

std::u16string BraveVPNButton::GetTooltipText(const gfx::Point& p) const {
  if (!IsPurchased())
    return l10n_util::GetStringUTF16(IDS_BRAVE_VPN);

  return l10n_util::GetStringUTF16(IsConnected()
                                       ? IDS_BRAVE_VPN_CONNECTED_TOOLTIP
                                       : IDS_BRAVE_VPN_DISCONNECTED_TOOLTIP);
}

void BraveVPNButton::OnThemeChanged() {
  ToolbarButton::OnThemeChanged();

  // Configure vpn button specific ink drop config as ink drop is reset
  // whenever theme changes.

  // Set 0.0f to use same color for activated state.
  views::InkDrop::Get(this)->SetVisibleOpacity(0.00f);

  // Different base color is set per themes and it has alpha.
  views::InkDrop::Get(this)->SetHighlightOpacity(1.0f);

  UpdateColorsAndInsets();
}

bool BraveVPNButton::IsConnected() const {
  return service_->IsConnected();
}

ConnectionState BraveVPNButton::GetVpnConnectionState() const {
  if (connection_state_for_testing_) {
    return connection_state_for_testing_.value();
  }
  return service_->GetConnectionState();
}

bool BraveVPNButton::IsConnectError() const {
  const auto state = GetVpnConnectionState();
  return (state == ConnectionState::CONNECT_NOT_ALLOWED ||
          state == ConnectionState::CONNECT_FAILED);
}

bool BraveVPNButton::IsPurchased() const {
  return service_->is_purchased_user();
}
void BraveVPNButton::OnButtonPressed(const ui::Event& event) {
  chrome::ExecuteCommand(browser_, IDC_SHOW_BRAVE_VPN_PANEL);
}

BEGIN_METADATA(BraveVPNButton)
END_METADATA
