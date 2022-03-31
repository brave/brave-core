// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "ui/views/controls/button/md_text_button.h"

#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/views/controls/highlight_path_generator.h"
#include "ui/views/view_class_properties.h"

// To be called from MdTextButtonBase::UpdateColors().
#define BRAVE_MD_TEXT_BUTTON_UPDATE_COLORS UpdateColorsForBrave();

#define MdTextButton MdTextButtonBase
#include "src/ui/views/controls/button/md_text_button.cc"
#undef MdTextButton

namespace {

constexpr SkColor kBraveBrandColor = SkColorSetRGB(0xff, 0x76, 0x54);

class BraveTextButtonHighlightPathGenerator
    : public views::HighlightPathGenerator {
 public:
  BraveTextButtonHighlightPathGenerator() = default;
  BraveTextButtonHighlightPathGenerator(
      const BraveTextButtonHighlightPathGenerator&) = delete;
  BraveTextButtonHighlightPathGenerator& operator=(
      const BraveTextButtonHighlightPathGenerator&) = delete;

  // HighlightPathGenerator
  SkPath GetHighlightPath(const views::View* view) override;
};

}  // namespace

namespace views {

// To be called from MdTextButtonBase::UpdateColors().
void MdTextButtonBase::UpdateColorsForBrave() {
  if (GetProminent()) {
    return;
  }
  const ui::NativeTheme* theme = GetNativeTheme();
  // Override different text hover color
  if (theme->GetPlatformHighContrastColorScheme() !=
      ui::NativeTheme::PlatformHighContrastColorScheme::kDark) {
    SetTextColor(ButtonState::STATE_HOVERED, kBraveBrandColor);
    SetTextColor(ButtonState::STATE_PRESSED, kBraveBrandColor);
  }
  // Override border color for hover on non-prominent
  if (GetState() == ButtonState::STATE_PRESSED ||
      GetState() == ButtonState::STATE_HOVERED) {
    // First, get the same background fill color that MdTextButtonBase does.
    // It is undfortunate to copy these lines almost as-is. Consider otherwise
    // patching it in via a #define.
    SkColor bg_color = GetColorProvider()->GetColor(ui::kColorDialogBackground);
    if (GetBgColorOverride()) {
      bg_color = *GetBgColorOverride();
    }
    if (GetState() == STATE_PRESSED) {
      bg_color = GetNativeTheme()->GetSystemButtonPressedColor(bg_color);
    }
    // The only thing that differs for Brave is the stroke color
    SkColor stroke_color = kBraveBrandColor;
    SetBackground(CreateBackgroundFromPainter(
        Painter::CreateRoundRectWith1PxBorderPainter(bg_color, stroke_color,
                                                     GetCornerRadius())));
  }
}

MdTextButton::MdTextButton(PressedCallback callback,
                           const std::u16string& text,
                           int button_context)
    : MdTextButtonBase(std::move(callback), text, button_context) {
  SetCornerRadius(100);
  views::HighlightPathGenerator::Install(
      this, std::make_unique<BraveTextButtonHighlightPathGenerator>());
  auto* ink_drop = views::InkDrop::Get(this);
  views::InkDrop::UseInkDropForFloodFillRipple(ink_drop,
                                               /*highlight_on_hover=*/false,
                                               /*highlight_on_focus=*/true);
  ink_drop->SetCreateHighlightCallback(base::BindRepeating(
      [](Button* host) {
        const SkColor fill_color = SK_ColorTRANSPARENT;
        gfx::RectF boundsF(host->GetLocalBounds());
        return std::make_unique<InkDropHighlight>(
            boundsF.size(), static_cast<MdTextButton*>(host)->GetCornerRadius(),
            boundsF.CenterPoint(), fill_color);
      },
      this));
}

MdTextButton::~MdTextButton() = default;

SkPath MdTextButton::GetHighlightPath() const {
  SkPath path;
  int radius = GetCornerRadius();
  path.addRRect(
      SkRRect::MakeRectXY(RectToSkRect(GetLocalBounds()), radius, radius));
  return path;
}

void MdTextButton::OnPaintBackground(gfx::Canvas* canvas) {
  // Set brave-style hover colors
  MdTextButtonBase::OnPaintBackground(canvas);
  if (GetProminent() &&
      (hover_animation().is_animating() || GetState() == STATE_HOVERED)) {
    constexpr SkColor normal_color = kBraveBrandColor;
    constexpr SkColor hover_color = SkColorSetRGB(0xff, 0x97, 0x7d);
    const SkAlpha alpha = hover_animation().CurrentValueBetween(0x00, 0xff);
    const SkColor current_color =
        color_utils::AlphaBlend(hover_color, normal_color, alpha);
    cc::PaintFlags flags;
    flags.setColor(current_color);
    flags.setStyle(cc::PaintFlags::kFill_Style);
    flags.setAntiAlias(true);
    canvas->DrawRoundRect(gfx::RectF(GetLocalBounds()), GetCornerRadius(),
                          flags);
  }
}

}  // namespace views

namespace {

SkPath BraveTextButtonHighlightPathGenerator::GetHighlightPath(
    const views::View* view) {
  return static_cast<const views::MdTextButton*>(view)->GetHighlightPath();
}

}  // namespace
