// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include <optional>
#include <tuple>

#include "base/containers/flat_map.h"
#include "base/no_destructor.h"
#include "base/notreached.h"
#include "build/build_config.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/base/models/image_model.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/geometry/rect_f.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/gfx/vector_icon_types.h"
#include "ui/native_theme/native_theme.h"
#include "ui/views/background.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/highlight_path_generator.h"
#include "ui/views/view_class_properties.h"

#define MdTextButton MdTextButtonBase
#include "src/ui/views/controls/button/md_text_button.cc"
#undef MdTextButton

namespace {

constexpr SkColor kBraveBrandColor = SkColorSetRGB(0xff, 0x76, 0x54);
constexpr SkColor kBravePrimaryColor = SkColorSetRGB(32, 74, 227);
SkColor AddOpacity(SkColor color, float opacity) {
  DCHECK(opacity >= 0 && opacity <= 1);
  auto current_alpha = SkColorGetA(color);
  return SkColorSetA(color, current_alpha * opacity);
}

using Kind = views::MdTextButton::Kind;
using ColorScheme = ui::NativeTheme::PreferredColorScheme;
using ButtonState = views::Button::ButtonState;

struct ButtonStyle {
  std::optional<SkColor> background_color;
  std::optional<SkColor> border_color;
  SkColor text_color;
};

struct MdTextButtonStyleKey {
  Kind kind;
  ColorScheme color_scheme;
  ButtonState state;

  bool operator<(const MdTextButtonStyleKey& other) const {
    return std::tie(kind, color_scheme, state) <
           std::tie(other.kind, other.color_scheme, other.state);
  }
};

constexpr float kDisabledOpacity = 0.5f;
constexpr float kLoadingOpacity = 0.8f;

const base::flat_map<MdTextButtonStyleKey, ButtonStyle>& GetButtonThemes() {
  static base::NoDestructor<base::flat_map<MdTextButtonStyleKey, ButtonStyle>>
      button_themes(
          {{{Kind::kPrimary, ColorScheme::kLight, ButtonState::STATE_NORMAL},
            {.background_color = kBravePrimaryColor,
             .border_color = std::nullopt,
             .text_color = SK_ColorWHITE}},
           {{Kind::kPrimary, ColorScheme::kDark, ButtonState::STATE_NORMAL},
            {.background_color = kBravePrimaryColor,
             .border_color = std::nullopt,
             .text_color = SK_ColorWHITE}},
           {{Kind::kPrimary, ColorScheme::kLight, ButtonState::STATE_HOVERED},
            {.background_color = SkColorSetRGB(24, 56, 172),
             .border_color = std::nullopt,
             .text_color = SK_ColorWHITE}},
           {{Kind::kPrimary, ColorScheme::kDark, ButtonState::STATE_HOVERED},
            {.background_color = SkColorSetRGB(77, 92, 253),
             .border_color = std::nullopt,
             .text_color = SK_ColorWHITE}},

           {{Kind::kSecondary, ColorScheme::kLight, ButtonState::STATE_NORMAL},
            {.background_color = std::nullopt,
             .border_color = SK_ColorBLACK,
             .text_color = SK_ColorBLACK}},
           {{Kind::kSecondary, ColorScheme::kDark, ButtonState::STATE_NORMAL},
            {.background_color = std::nullopt,
             .border_color = SK_ColorWHITE,
             .text_color = SK_ColorWHITE}},
           {{Kind::kSecondary, ColorScheme::kLight, ButtonState::STATE_HOVERED},
            {.background_color = std::nullopt,
             .border_color = kBravePrimaryColor,
             .text_color = kBravePrimaryColor}},
           {{Kind::kSecondary, ColorScheme::kDark, ButtonState::STATE_HOVERED},
            {.background_color = std::nullopt,
             .border_color = kBravePrimaryColor,
             .text_color = kBravePrimaryColor}},

           {{Kind::kTertiary, ColorScheme::kLight, ButtonState::STATE_NORMAL},
            {.background_color = std::nullopt,
             .border_color = std::nullopt,
             .text_color = SkColorSetRGB(32, 74, 227)}},
           {{Kind::kTertiary, ColorScheme::kDark, ButtonState::STATE_NORMAL},
            {.background_color = std::nullopt,
             .border_color = std::nullopt,
             .text_color = SkColorSetRGB(153, 173, 243)}},
           {{Kind::kTertiary, ColorScheme::kLight, ButtonState::STATE_HOVERED},
            {.background_color = std::nullopt,
             .border_color = std::nullopt,
             .text_color = SkColorSetRGB(24, 56, 172)}},
           {{Kind::kTertiary, ColorScheme::kDark, ButtonState::STATE_HOVERED},
            {.background_color = std::nullopt,
             .border_color = std::nullopt,
             .text_color = SkColorSetRGB(186, 199, 247)}},

           {{Kind::kQuaternary, ColorScheme::kLight, ButtonState::STATE_NORMAL},
            {.background_color = std::nullopt,
             .border_color = std::nullopt,
             .text_color = SkColorSetRGB(84, 96, 113)}},
           {{Kind::kQuaternary, ColorScheme::kDark, ButtonState::STATE_NORMAL},
            {.background_color = std::nullopt,
             .border_color = std::nullopt,
             .text_color = SkColorSetRGB(195, 201, 211)}},
           {{Kind::kQuaternary, ColorScheme::kLight,
             ButtonState::STATE_HOVERED},
            {.background_color = std::nullopt,
             .border_color = std::nullopt,
             .text_color = SkColorSetRGB(63, 72, 85)}},
           {{Kind::kQuaternary, ColorScheme::kDark, ButtonState::STATE_HOVERED},
            {.background_color = std::nullopt,
             .border_color = std::nullopt,
             .text_color = SkColorSetRGB(195, 201, 211)}}});

  return *button_themes;
}

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

MdTextButton::MdTextButton(PressedCallback callback,
                           const std::u16string& text,
                           int button_context,
                           bool use_text_color_for_icon)
    : MdTextButtonBase(std::move(callback),
                       text,
                       button_context,
                       use_text_color_for_icon) {
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
            boundsF.size(),
            static_cast<MdTextButton*>(host)->GetCornerRadiusValue(),
            boundsF.CenterPoint(), fill_color);
      },
      this));
}

MdTextButton::~MdTextButton() = default;

SkPath MdTextButton::GetHighlightPath() const {
  SkPath path;
  float radius = GetCornerRadiusValue();
  path.addRRect(
      SkRRect::MakeRectXY(RectToSkRect(GetLocalBounds()), radius, radius));
  return path;
}

MdTextButton::Kind MdTextButton::GetKind() const {
  return kind_;
}

void MdTextButton::SetKind(Kind kind) {
  if (kind == kind_) {
    return;
  }

  kind_ = kind;

  // We don't want to affect the OLD style buttons, and we want them to be the
  // default (for now), so don't change the image-label spacing unless we set
  // the button kind to something that isn't OLD.
  if (kind != Kind::kOld) {
    SetImageLabelSpacing(6);
  }

  UpdateColors();
}

void MdTextButton::SetIcon(const gfx::VectorIcon* icon, int icon_size) {
  icon_ = icon;
  icon_size_ = icon_size;
  UpdateColors();
}

bool MdTextButton::GetLoading() const {
  return loading_;
}

void MdTextButton::SetLoading(bool loading) {
  loading_ = loading;
  UpdateColors();
}

void MdTextButton::UpdateTextColor() {
  MdTextButtonBase::UpdateTextColor();

  // Once we update the buttons across Brave to use the new style, we can remove
  // this branch.
  if (kind_ == kOld) {
    if (GetStyle() == ui::ButtonStyle::kProminent) {
      return;
    }
    const ui::NativeTheme* theme = GetNativeTheme();
    // Override different text hover color
    if (theme->GetPlatformHighContrastColorScheme() !=
        ui::NativeTheme::PlatformHighContrastColorScheme::kDark) {
      SetTextColor(ButtonState::STATE_HOVERED, kBraveBrandColor);
      SetTextColor(ButtonState::STATE_PRESSED, kBraveBrandColor);
    }
    return;
  }

  auto colors = GetButtonColors();
  SetTextColor(GetVisualState(), colors.text_color);
}

void MdTextButton::UpdateBackgroundColor() {
  // Once we update the buttons across Brave to use the new style, we can remove
  // this branch.
  if (kind_ == kOld) {
    MdTextButtonBase::UpdateBackgroundColor();

    // We don't modify the Prominent button at all.
    if (GetStyle() == ui::ButtonStyle::kProminent) {
      return;
    }

    // Override border color for hover on non-prominent
    if (GetState() == ButtonState::STATE_PRESSED ||
        GetState() == ButtonState::STATE_HOVERED) {
      // First, get the same background fill color that MdTextButtonBase does.
      // It is unfortunate to copy these lines almost as-is. Consider otherwise
      // patching it in via a #define.
      SkColor bg_color =
          GetColorProvider()->GetColor(ui::kColorDialogBackground);
      if (GetBgColorOverride()) {
        bg_color = *GetBgColorOverride();
      }
      if (GetState() == STATE_PRESSED) {
        bg_color = GetNativeTheme()->GetSystemButtonPressedColor(bg_color);
      }
      // The only thing that differs for Brave is the stroke color
      SkColor stroke_color = kBraveBrandColor;
      SetBackground(CreateBackgroundFromPainter(
          Painter::CreateRoundRectWith1PxBorderPainter(
              bg_color, stroke_color, GetCornerRadiusValue())));
    }
    return;
  }

  auto colors = GetButtonColors();

  // SubPixelRendering doesn't work if we have any background opacity.
  SetTextSubpixelRenderingEnabled(SkColorGetA(colors.background_color) == 0xFF);

  SetBackground(
      CreateBackgroundFromPainter(Painter::CreateRoundRectWith1PxBorderPainter(
          colors.background_color, colors.stroke_color,
          GetCornerRadiusValue())));
}

void MdTextButton::UpdateColors() {
  MdTextButtonBase::UpdateColors();

  // Update the icon color.
  if (icon_) {
    SetImageModel(ButtonState::STATE_NORMAL,
                  ui::ImageModel::FromVectorIcon(*icon_, GetCurrentTextColor(),
                                                 icon_size_));
  }
}

void MdTextButton::OnPaintBackground(gfx::Canvas* canvas) {
  // Set brave-style hover colors
  MdTextButtonBase::OnPaintBackground(canvas);
  if (GetStyle() == ui::ButtonStyle::kProminent &&
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
    canvas->DrawRoundRect(gfx::RectF(GetLocalBounds()), GetCornerRadiusValue(),
                          flags);
  }
}

MdTextButton::ButtonColors MdTextButton::GetButtonColors() {
  // Leo buttons only have a light and dark mode.
  auto color_scheme =
      GetNativeTheme()->GetPreferredColorScheme() == ColorScheme::kDark
          ? ColorScheme::kDark
          : ColorScheme::kLight;
  auto state = GetVisualState();
  float opacity = 1;

  // Leo buttons don't have a pressed state, so use the hover state instead.
  if (state == ButtonState::STATE_PRESSED) {
    state = ButtonState::STATE_HOVERED;
  }

  // The loading style is the normal button style, with some opacity.
  if (loading_) {
    state = ButtonState::STATE_NORMAL;
    opacity = kLoadingOpacity;
  }

  // The enabled style is the normal button style with more opacity.
  if (!GetEnabled() || state == STATE_DISABLED) {
    state = ButtonState::STATE_NORMAL;
    opacity = kDisabledOpacity;
  }

  MdTextButtonStyleKey style_lookup{GetKind(), color_scheme, state};
  auto it = GetButtonThemes().find(style_lookup);
  if (it == GetButtonThemes().end()) {
    NOTREACHED() << "No style found for ButtonKind: " << kind_
                 << ", ColorScheme: "
                 << (color_scheme == ColorScheme::kDark ? "dark" : "light")
                 << ", ButtonState: " << state;
  }
  const auto& style = it->second;
  return {.background_color = AddOpacity(
              GetBgColorOverride().value_or(
                  style.background_color.value_or(SK_ColorTRANSPARENT)),
              opacity),
          .stroke_color = AddOpacity(
              style.border_color.value_or(SK_ColorTRANSPARENT), opacity),
          .text_color = AddOpacity(style.text_color, opacity)};
}

BEGIN_METADATA(MdTextButton)
END_METADATA

}  // namespace views

namespace {

SkPath BraveTextButtonHighlightPathGenerator::GetHighlightPath(
    const views::View* view) {
  return static_cast<const views::MdTextButton*>(view)->GetHighlightPath();
}

}  // namespace
