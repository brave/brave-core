/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/profiles/brave_avatar_toolbar_button.h"

#include <memory>
#include <string>
#include <utility>

#include "base/strings/string_number_conversions.h"
#include "brave/app/vector_icons/vector_icons.h"
#include "brave/browser/ui/color/color_palette.h"
#include "brave/browser/ui/views/profiles/brave_avatar_toolbar_button_delegate.h"
#include "brave/components/l10n/common/localization_util.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/views/chrome_layout_provider.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/profiles/avatar_toolbar_button.h"
#include "chrome/browser/ui/views/toolbar/toolbar_ink_drop_util.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/pointer/touch_ui_controller.h"
#include "ui/base/theme_provider.h"
#include "ui/gfx/geometry/rrect_f.h"
#include "ui/gfx/geometry/skia_conversions.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/controls/highlight_path_generator.h"
#include "ui/views/view_class_properties.h"

namespace {

constexpr int kHighlightRadius = 36;
constexpr int kBraveAvatarButtonHorizontalSpacing = 10;

class BraveAvatarButtonHighlightPathGenerator
    : public views::HighlightPathGenerator {
 public:
  explicit BraveAvatarButtonHighlightPathGenerator(
      const base::WeakPtr<BraveAvatarToolbarButton>& avatar_button)
      : avatar_button_(avatar_button) {}
  BraveAvatarButtonHighlightPathGenerator(
      const BraveAvatarButtonHighlightPathGenerator&) = delete;
  BraveAvatarButtonHighlightPathGenerator& operator=(
      const BraveAvatarButtonHighlightPathGenerator&) = delete;
  ~BraveAvatarButtonHighlightPathGenerator() override = default;

  absl::optional<gfx::RRectF> GetRoundRect(const gfx::RectF& bounds) override {
    gfx::Rect rect(avatar_button_->size());
    rect.Inset(GetToolbarInkDropInsets(avatar_button_.get()));
    DCHECK(avatar_button_);
    if (avatar_button_->GetAvatarButtonState() ==
        AvatarToolbarButton::State::kAnimatedUserIdentity) {
      // In this case, our radius wouldn't be used. We should keep using
      // upstream's radius for the highlight too.
      return gfx::RRectF(gfx::RectF(rect),
                         ChromeLayoutProvider::Get()->GetCornerRadiusMetric(
                             views::Emphasis::kMaximum, {}));
    }

    // This make the highlight drawn as circle.
    return gfx::RRectF(gfx::RectF(rect), kHighlightRadius);
  }

 private:
  base::WeakPtr<BraveAvatarToolbarButton> avatar_button_;
};

}  // namespace

BraveAvatarToolbarButton::BraveAvatarToolbarButton(BrowserView* browser_view)
    : AvatarToolbarButton(browser_view) {}

BraveAvatarToolbarButton::~BraveAvatarToolbarButton() = default;

AvatarToolbarButton::State BraveAvatarToolbarButton::GetAvatarButtonState()
    const {
  return delegate_->GetState();
}

void BraveAvatarToolbarButton::SetHighlight(
    const std::u16string& highlight_text,
    absl::optional<SkColor> highlight_color) {
  std::u16string revised_highlight_text;
  if (browser_->profile()->IsTor()) {
    revised_highlight_text = brave_l10n::GetLocalizedResourceUTF16String(
        IDS_TOR_AVATAR_BUTTON_LABEL);

    if (GetWindowCount() > 1) {
      revised_highlight_text =
          l10n_util::GetStringFUTF16(IDS_TOR_AVATAR_BUTTON_LABEL_COUNT,
                                     base::NumberToString16(GetWindowCount()));
    }
  } else if (browser_->profile()->IsIncognitoProfile()) {
    revised_highlight_text = brave_l10n::GetLocalizedResourceUTF16String(
        IDS_PRIVATE_AVATAR_BUTTON_LABEL);

    if (GetWindowCount() > 1) {
      revised_highlight_text =
          l10n_util::GetStringFUTF16(IDS_PRIVATE_AVATAR_BUTTON_LABEL_COUNT,
                                     base::NumberToString16(GetWindowCount()));
    }
  } else if (browser_->profile()->IsGuestSession()) {
    // We only want the icon for Guest profiles.
    revised_highlight_text = std::u16string();
  } else {
    revised_highlight_text = highlight_text;
  }

  AvatarToolbarButton::SetHighlight(revised_highlight_text, highlight_color);
}

void BraveAvatarToolbarButton::OnThemeChanged() {
  AvatarToolbarButton::OnThemeChanged();

  // Replace ToolbarButton's highlight path generator.
  views::HighlightPathGenerator::Install(
      this, std::make_unique<BraveAvatarButtonHighlightPathGenerator>(
                weak_ptr_factory_.GetWeakPtr()));
}

int BraveAvatarToolbarButton::GetWindowCount() const {
  return delegate_->GetWindowCount();
}

void BraveAvatarToolbarButton::UpdateColorsAndInsets() {
  // Use custom bg/border for private/tor window.
  if (delegate_->GetState() == State::kIncognitoProfile) {
    const bool is_tor = browser_->profile()->IsTor();
    const auto text_color = is_tor ? SkColorSetRGB(0xE3, 0xB3, 0xFF)
                                   : SkColorSetRGB(0xcc, 0xBE, 0xFE);
    SetEnabledTextColors(text_color);
    SetTextColor(views::Button::STATE_DISABLED, text_color);

    // We give more margins to horizontally.
    gfx::Insets target_insets =
        ::GetLayoutInsets(TOOLBAR_BUTTON) +
        *GetProperty(views::kInternalPaddingKey) +
        gfx::Insets::VH(0, kBraveAvatarButtonHorizontalSpacing);

    const auto border_color = is_tor ? SkColorSetARGB(0x66, 0x91, 0x5E, 0xAE)
                                     : SkColorSetARGB(0x66, 0x7B, 0x63, 0xBF);
    const auto final_border_color = color_utils::GetResultingPaintColor(
        border_color, (is_tor ? kPrivateTorToolbar : kPrivateToolbar));
    std::unique_ptr<views::Border> border = views::CreateRoundedRectBorder(
        1, kHighlightRadius, gfx::Insets(), final_border_color);
    const gfx::Insets extra_insets = target_insets - border->GetInsets();
    SetBorder(views::CreatePaddedBorder(std::move(border), extra_insets));

    constexpr int kBraveAvatarImageLabelSpacing = 8;
    SetImageLabelSpacing(kBraveAvatarImageLabelSpacing);
    return;
  }

  AvatarToolbarButton::UpdateColorsAndInsets();
}

ui::ImageModel BraveAvatarToolbarButton::GetAvatarIcon(
    ButtonState state,
    const gfx::Image& gaia_account_image) const {
  const auto icon_size = GetLayoutConstant(LOCATION_BAR_ICON_SIZE);
  if (browser_->profile()->IsTor()) {
    return ui::ImageModel::FromVectorIcon(
        kLeoProductTorIcon, SkColorSetRGB(0x3C, 0x82, 0x3C), icon_size);
  }

  if (browser_->profile()->IsIncognitoProfile()) {
    return ui::ImageModel::FromVectorIcon(
        kIncognitoIcon, SkColorSetRGB(0xFF, 0xFF, 0xFF), icon_size);
  }

  if (browser_->profile()->IsGuestSession()) {
    return ui::ImageModel::FromVectorIcon(kUserMenuGuestIcon,
                                          GetForegroundColor(state), icon_size);
  }

  return AvatarToolbarButton::GetAvatarIcon(state, gaia_account_image);
}

std::u16string BraveAvatarToolbarButton::GetAvatarTooltipText() const {
  if (browser_->profile()->IsTor())
    return brave_l10n::GetLocalizedResourceUTF16String(
        IDS_TOR_AVATAR_BUTTON_TOOLTIP_TEXT);

  return AvatarToolbarButton::GetAvatarTooltipText();
}
