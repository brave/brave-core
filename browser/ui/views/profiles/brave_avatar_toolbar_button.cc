/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/profiles/brave_avatar_toolbar_button.h"

#include <memory>

#include "brave/app/vector_icons/vector_icons.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/views/profiles/avatar_toolbar_button.h"
#include "chrome/browser/ui/views/profiles/avatar_toolbar_button_delegate.h"
#include "chrome/browser/ui/views/toolbar/toolbar_ink_drop_util.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/pointer/touch_ui_controller.h"
#include "ui/base/theme_provider.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/controls/highlight_path_generator.h"

namespace {

constexpr int kHighlightRadius = 36;
constexpr int kBraveAvatarButtonHorizontalSpacing = 10;

class BraveToolbarButtonHighlightPathGenerator
    : public views::HighlightPathGenerator {
 public:
  // HighlightPathGenerator:
  SkPath GetHighlightPath(const views::View* view) override {
    gfx::Rect rect(view->size());
    rect.Inset(GetToolbarInkDropInsets(view));
    SkPath path;
    path.addRoundRect(gfx::RectToSkRect(rect),
                      kHighlightRadius, kHighlightRadius);
    return path;
  }
};
}  // namespace

BraveAvatarToolbarButton::BraveAvatarToolbarButton(BrowserView* browser_view)
    : AvatarToolbarButton(browser_view) {
  // Replace ToolbarButton's highlight path generator.
  views::HighlightPathGenerator::Install(
      this, std::make_unique<BraveToolbarButtonHighlightPathGenerator>());
}

void BraveAvatarToolbarButton::SetHighlight(
    const std::u16string& highlight_text,
    base::Optional<SkColor> highlight_color) {
  std::u16string revised_highlight_text;
  if (browser_->profile()->IsTor()) {
    revised_highlight_text =
        l10n_util::GetStringUTF16(IDS_TOR_AVATAR_BUTTON_LABEL);
  } else if (browser_->profile()->IsGuestSession()) {
    // We only want the icon for Guest profiles.
    revised_highlight_text = std::u16string();
  } else {
    revised_highlight_text = highlight_text;
  }

  AvatarToolbarButton::SetHighlight(revised_highlight_text, highlight_color);
}

void BraveAvatarToolbarButton::UpdateColorsAndInsets() {
  // Use custom bg/border for private/tor window.
  const bool is_tor = browser_->profile()->IsTor();
  if (delegate_->GetState() == State::kIncognitoProfile || is_tor) {
    constexpr SkColor kPrivateAvatarBGColor = SkColorSetRGB(0x1D, 0x19, 0x35);
    constexpr SkColor kPrivateTorAvatarBGColor =
        SkColorSetRGB(0x1A, 0x0F, 0x2A);

    const gfx::Insets paint_insets =
        gfx::Insets((height() - GetLayoutConstant(LOCATION_BAR_HEIGHT)) / 2) +
        *GetProperty(views::kInternalPaddingKey);
    SetEnabledTextColors(SK_ColorWHITE);
    SetBackground(views::CreateBackgroundFromPainter(
        views::Painter::CreateSolidRoundRectPainter(
            is_tor ? kPrivateTorAvatarBGColor : kPrivateAvatarBGColor,
            kHighlightRadius,
            paint_insets)));

    // We give more margins to horizontally.
    gfx::Insets target_insets =
        ::GetLayoutInsets(TOOLBAR_BUTTON) +
        *GetProperty(views::kInternalPaddingKey) +
        gfx::Insets(0, kBraveAvatarButtonHorizontalSpacing);
    SetBorder(views::CreateEmptyBorder(target_insets));

    constexpr int kBraveAvatarImageLabelSpacing = 8;
    SetImageLabelSpacing(kBraveAvatarImageLabelSpacing);
    return;
  }

  AvatarToolbarButton::UpdateColorsAndInsets();
}

ui::ImageModel BraveAvatarToolbarButton::GetAvatarIcon(
    ButtonState state,
    const gfx::Image& gaia_account_image) const {
  // We don't use icon for tor avatar button.
  if (browser_->profile()->IsTor())
    return ui::ImageModel();

  if (browser_->profile()->IsGuestSession()) {
    return ui::ImageModel::FromVectorIcon(
        kUserMenuGuestIcon,
        GetForegroundColor(state),
        ui::TouchUiController::Get()->touch_ui() ? 24 : 20);
  }

  return AvatarToolbarButton::GetAvatarIcon(state, gaia_account_image);
}

std::u16string BraveAvatarToolbarButton::GetAvatarTooltipText() const {
  if (browser_->profile()->IsTor())
    return l10n_util::GetStringUTF16(IDS_TOR_AVATAR_BUTTON_TOOLTIP_TEXT);

  return AvatarToolbarButton::GetAvatarTooltipText();
}
