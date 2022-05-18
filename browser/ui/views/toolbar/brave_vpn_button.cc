/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/toolbar/brave_vpn_button.h"

#include <memory>
#include <utility>

#include "base/memory/raw_ptr.h"
#include "base/notreached.h"
#include "brave/app/brave_command_ids.h"
#include "brave/app/vector_icons/vector_icons.h"
#include "brave/browser/brave_vpn/brave_vpn_service_factory.h"
#include "brave/browser/themes/theme_properties.h"
#include "brave/components/brave_vpn/brave_vpn_service.h"
#include "brave/components/l10n/common/locale_util.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/views/toolbar/toolbar_ink_drop_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/models/simple_menu_model.h"
#include "ui/gfx/geometry/rect_f.h"
#include "ui/gfx/geometry/rrect_f.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/gfx/skia_util.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/controls/highlight_path_generator.h"

using ConnectionState = brave_vpn::mojom::ConnectionState;
using PurchasedState = brave_vpn::mojom::PurchasedState;

namespace {

constexpr int kButtonRadius = 47;

class BraveVPNButtonHighlightPathGenerator
    : public views::HighlightPathGenerator {
 public:
  explicit BraveVPNButtonHighlightPathGenerator(const gfx::Insets& insets)
      : HighlightPathGenerator(insets) {}

  BraveVPNButtonHighlightPathGenerator(
      const BraveVPNButtonHighlightPathGenerator&) = delete;
  BraveVPNButtonHighlightPathGenerator& operator=(
      const BraveVPNButtonHighlightPathGenerator&) = delete;

  // views::HighlightPathGenerator overrides:
  absl::optional<gfx::RRectF> GetRoundRect(const gfx::RectF& rect) override {
    return gfx::RRectF(rect, kButtonRadius);
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
    DCHECK(service_);
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
      brave_vpn::mojom::PurchasedState state) override {
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
  DCHECK(service_);
  Observe(service_);

  // Replace ToolbarButton's highlight path generator.
  views::HighlightPathGenerator::Install(
      this, std::make_unique<BraveVPNButtonHighlightPathGenerator>(
                GetToolbarInkDropInsets(this)));

  // The MenuButtonController makes sure the panel closes when clicked if the
  // panel is already open.
  auto menu_button_controller = std::make_unique<views::MenuButtonController>(
      this,
      base::BindRepeating(&BraveVPNButton::OnButtonPressed,
                          base::Unretained(this)),
      std::make_unique<views::Button::DefaultButtonControllerDelegate>(this));
  menu_button_controller_ = menu_button_controller.get();
  SetButtonController(std::move(menu_button_controller));

  label()->SetText(brave_l10n::GetLocalizedResourceUTF16String(
      IDS_BRAVE_VPN_TOOLBAR_BUTTON_TEXT));
  gfx::FontList font_list = views::Label::GetDefaultFontList();
  constexpr int kFontSize = 12;
  label()->SetFontList(
      font_list.DeriveWithSizeDelta(kFontSize - font_list.GetFontSize()));

  // Set image positions first. then label.
  SetHorizontalAlignment(gfx::ALIGN_LEFT);

  UpdateButtonState();

  // Views resulting in focusable nodes later on in the accessibility tree need
  // to have an accessible name for screen readers to see what they are about.
  // TODO(simonhong): Re-visit this name.
  SetAccessibleName(brave_l10n::GetLocalizedResourceUTF16String(
      IDS_BRAVE_VPN_TOOLBAR_BUTTON_TEXT));
}

BraveVPNButton::~BraveVPNButton() = default;

void BraveVPNButton::OnConnectionStateChanged(ConnectionState state) {
  UpdateButtonState();
}

void BraveVPNButton::UpdateColorsAndInsets() {
  if (const auto* tp = GetThemeProvider()) {
    const gfx::Insets paint_insets =
        gfx::Insets((height() - GetLayoutConstant(LOCATION_BAR_HEIGHT)) / 2);
    SetBackground(views::CreateBackgroundFromPainter(
        views::Painter::CreateSolidRoundRectPainter(
            tp->GetColor(ThemeProperties::COLOR_TOOLBAR), kButtonRadius,
            paint_insets)));

    SetEnabledTextColors(tp->GetColor(
        IsConnected()
            ? BraveThemeProperties::COLOR_BRAVE_VPN_BUTTON_TEXT_CONNECTED
            : BraveThemeProperties::COLOR_BRAVE_VPN_BUTTON_TEXT_DISCONNECTED));

    std::unique_ptr<views::Border> border = views::CreateRoundedRectBorder(
        1, kButtonRadius, gfx::Insets(),
        tp->GetColor(BraveThemeProperties::COLOR_BRAVE_VPN_BUTTON_BORDER));
    constexpr auto kTargetInsets = gfx::Insets::VH(4, 6);
    const gfx::Insets extra_insets = kTargetInsets - border->GetInsets();
    SetBorder(views::CreatePaddedBorder(std::move(border), extra_insets));
  }

  constexpr int kBraveAvatarImageLabelSpacing = 4;
  SetImageLabelSpacing(kBraveAvatarImageLabelSpacing);
}

void BraveVPNButton::UpdateButtonState() {
  constexpr SkColor kColorConnected = SkColorSetRGB(0x51, 0xCF, 0x66);
  constexpr SkColor kColorDisconnected = SkColorSetRGB(0xAE, 0xB1, 0xC2);
  SetImage(views::Button::STATE_NORMAL,
           gfx::CreateVectorIcon(kVpnIndicatorIcon, IsConnected()
                                                        ? kColorConnected
                                                        : kColorDisconnected));
}

bool BraveVPNButton::IsConnected() {
  return service_->is_connected();
}

void BraveVPNButton::OnButtonPressed(const ui::Event& event) {
  chrome::ExecuteCommand(browser_, IDC_SHOW_BRAVE_VPN_PANEL);
}

BEGIN_METADATA(BraveVPNButton, LabelButton)
END_METADATA
