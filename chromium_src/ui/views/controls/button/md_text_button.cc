// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "ui/views/controls/button/md_text_button.h"

#include "absl/types/optional.h"
#include "include/core/SkColor.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/geometry/rect_f.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/gfx/vector_icon_types.h"
#include "ui/native_theme/native_theme.h"
#include "ui/views/background.h"
#include "ui/views/controls/highlight_path_generator.h"
#include "ui/views/view_class_properties.h"

// To be called from MdTextButtonBase::UpdateColors().
#define BRAVE_MD_TEXT_BUTTON_UPDATE_COLORS \
  UpdateColorsForBrave();                  \
  UpdateIconForBrave();

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

SkColor kBravePrimaryColor = SkColorSetRGB(32, 74, 227);
MdTextButton::ButtonTheme g_primary_theme = {
    // Normal Light
    MdTextButton::ButtonStyle{kBravePrimaryColor, absl::nullopt,
                              SK_ColorWHITE},
    // Normal Dark
    MdTextButton::ButtonStyle{kBravePrimaryColor, absl::nullopt,
                              SK_ColorWHITE},
    // Hover Light
    MdTextButton::ButtonStyle{SkColorSetRGB(24, 56, 172), absl::nullopt,
                              SK_ColorWHITE},
    // Hover Dark
    MdTextButton::ButtonStyle{SkColorSetRGB(77, 92, 253), absl::nullopt,
                              SK_ColorWHITE},
    // Disabled Light
    MdTextButton::ButtonStyle{SkColorSetARGB(128, 172, 175, 187), absl::nullopt,
                              SkColorSetA(SK_ColorWHITE, 128)},
    // Disabled Dark
    MdTextButton::ButtonStyle{SkColorSetARGB(128, 88, 92, 109), absl::nullopt,
                              SkColorSetA(SK_ColorWHITE, 128)},
    // Loading Light
    MdTextButton::ButtonStyle{SkColorSetARGB(192, 32, 74, 227), absl::nullopt,
                              SkColorSetA(SK_ColorWHITE, 192)},
    // Loading Dark
    MdTextButton::ButtonStyle{SkColorSetARGB(192, 32, 74, 227), absl::nullopt,
                              SkColorSetA(SK_ColorWHITE, 192)}};

MdTextButton::ButtonTheme g_secondary_theme = {
    // Normal Light
    MdTextButton::ButtonStyle{absl::nullopt, SK_ColorBLACK, SK_ColorBLACK},
    // Normal Dark
    MdTextButton::ButtonStyle{absl::nullopt, SK_ColorWHITE, SK_ColorWHITE},
    // Hover Light
    MdTextButton::ButtonStyle{absl::nullopt, kBravePrimaryColor, kBravePrimaryColor},
    // Hover Dark
    MdTextButton::ButtonStyle{absl::nullopt, kBravePrimaryColor, kBravePrimaryColor},
    // Disabled Light
    MdTextButton::ButtonStyle{absl::nullopt, SkColorSetARGB(128, 226, 227, 231),
                              SkColorSetARGB(128, 107, 112, 132)},
    // Disabled Dark
    MdTextButton::ButtonStyle{absl::nullopt, SkColorSetARGB(128, 46, 48, 57),
                              SkColorSetARGB(128, 140, 144, 161)},
    // Loading Light
    MdTextButton::ButtonStyle{absl::nullopt, SkColorSetARGB(192, 226, 227, 231),
                              SkColorSetARGB(192, 107, 112, 132)},
    // Loading Dark
    MdTextButton::ButtonStyle{absl::nullopt, SkColorSetARGB(192, 46, 48, 57),
                              SkColorSetARGB(192, 140, 144, 161)}};

MdTextButton::ButtonTheme g_tertiary_theme = {
    // Normal Light
    MdTextButton::ButtonStyle{absl::nullopt, absl::nullopt,
                              SkColorSetRGB(32, 74, 227)},
    // Normal Dark
    MdTextButton::ButtonStyle{absl::nullopt, absl::nullopt,
                              SkColorSetRGB(153, 173, 243)},
    // Hover Light
    MdTextButton::ButtonStyle{absl::nullopt, absl::nullopt,
                              SkColorSetRGB(24, 56, 172)},
    // Hover Dark
    MdTextButton::ButtonStyle{absl::nullopt, absl::nullopt,
                              SkColorSetRGB(186, 199, 247)},
};

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
  SetKind(SECONDARY);
  SetImageLabelSpacing(6);
}

MdTextButton::~MdTextButton() = default;

SkPath MdTextButton::GetHighlightPath() const {
  SkPath path;
  int radius = GetCornerRadius();
  path.addRRect(
      SkRRect::MakeRectXY(RectToSkRect(GetLocalBounds()), radius, radius));
  return path;
}

MdTextButton::Kind MdTextButton::GetKind() const {
  return GetProminent() ? CTA : kind_;
}

void MdTextButton::SetKind(Kind kind) {
  kind_ = kind;
  switch (kind_) {
    case PRIMARY:
      theme_ = g_primary_theme;
      break;
    case SECONDARY:
      theme_ = g_secondary_theme;
      break;
    case TERTIARY:
      theme_ = g_tertiary_theme;
      break;
    default:
      theme_ = absl::nullopt;
      break;
  }

  // Unfortunately we also have to deal with |IsProminent| from the
  // MdTextButtonBase. This theme corresponds to the CTA theme in Leo.
  SetProminent(kind == CTA);
  UpdateColors();
}

void MdTextButton::SetIcon(const gfx::VectorIcon* icon) {
  icon_ = icon;
  UpdateColors();
}

// To be called from MdTextButtonBase::UpdateColors().
void MdTextButton::UpdateColorsForBrave() {
  if (GetProminent()) {
    return;
  }

  // Theme should only ever be |absl::nullopt| if the button is Prominent.
  DCHECK(theme_);

  absl::optional<ButtonStyle> style;
  auto is_dark = GetNativeTheme()->GetPreferredColorScheme() ==
                 ui::NativeTheme::PreferredColorScheme::kDark;
  auto state = GetVisualState();
  if (theme_) {
    style = is_dark ? theme_->normal_dark : theme_->normal_light;
    if (state == STATE_HOVERED)
      style = is_dark ? theme_->hover_dark : theme_->hover_light;
    // TODO: Loading
    if (!GetEnabled() || state == STATE_DISABLED)
      style = is_dark ? theme_->disabled_dark : theme_->disabled_light;

    SetTextColor(state, style->text_color);

    // Prefer the BgColorOverride, if there is one. Fallback to what's in our
    // style.
    SkColor bg_color = GetBgColorOverride().value_or(
        style->background_color.value_or(SK_ColorTRANSPARENT));
    SkColor stroke_color = style->border_color.value_or(bg_color);
    SetBackground(CreateBackgroundFromPainter(
        Painter::CreateRoundRectWith1PxBorderPainter(bg_color, stroke_color,
                                                     GetCornerRadius())));
    return;
  }
}

void MdTextButton::UpdateIconForBrave() {
  if (icon_) {
    SetImage(ButtonState::STATE_NORMAL,
             gfx::CreateVectorIcon(*icon_, GetCurrentTextColor()));
  }
}

void MdTextButton::OnPaintBackground(gfx::Canvas* canvas) {
  // Set brave-style hover colors
  MdTextButtonBase::OnPaintBackground(canvas);
  if (GetProminent() &&
      (hover_animation().is_animating() || GetState() == STATE_HOVERED)) {
    constexpr SkColor normal_color = kBraveBrandColor;
    constexpr SkColor hover_color = SkColorSetRGB(0xff, 0x97, 0x7d);
    const SkAlpha alpha =
        static_cast<SkAlpha>(hover_animation().CurrentValueBetween(0x00, 0xff));
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
