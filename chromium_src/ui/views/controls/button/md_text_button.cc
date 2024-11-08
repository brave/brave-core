// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "ui/views/controls/button/md_text_button.h"

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
#include "ui/gfx/color_palette.h"
#include "ui/gfx/color_utils.h"
#include "ui/gfx/geometry/rect_f.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/gfx/vector_icon_types.h"
#include "ui/native_theme/native_theme.h"
#include "ui/views/background.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/highlight_path_generator.h"
#include "ui/views/view_class_properties.h"

#define MdTextButton MdTextButtonBase
#include "src/ui/views/controls/button/md_text_button.cc"
#undef MdTextButton

namespace {

SkColor AddOpacity(SkColor color, float opacity) {
  DCHECK(opacity >= 0 && opacity <= 1);
  auto current_alpha = SkColorGetA(color);
  return SkColorSetA(color, current_alpha * opacity);
}

using ColorScheme = ui::NativeTheme::PreferredColorScheme;
using ButtonState = views::Button::ButtonState;

struct ButtonStyle {
  std::optional<SkColor> background_color;
  std::optional<SkColor> border_color;
  SkColor text_color;
};

struct MdTextButtonStyleKey {
  ui::ButtonStyle style;
  ColorScheme color_scheme;
  ButtonState state;

  bool operator<(const MdTextButtonStyleKey& other) const {
    return std::tie(style, color_scheme, state) <
           std::tie(other.style, other.color_scheme, other.state);
  }
};

constexpr float kLoadingOpacity = 0.75f;

// We map our button styles to upstream style.
// Prominent, Default, Tonal, Text styles are mapped
// sequentially to our Filled, Outline, Plain and Plain-Faint.
const base::flat_map<MdTextButtonStyleKey, ButtonStyle>& GetButtonThemes() {
  static base::NoDestructor<base::flat_map<MdTextButtonStyleKey, ButtonStyle>>
      button_themes(
          {{{ui::ButtonStyle::kProminent, ColorScheme::kLight,
             ButtonState::STATE_NORMAL},
            {.background_color = gfx::kColorButtonBackground,
             .border_color = std::nullopt,
             .text_color = SK_ColorWHITE}},
           {{ui::ButtonStyle::kProminent, ColorScheme::kDark,
             ButtonState::STATE_NORMAL},
            {.background_color = gfx::kColorButtonBackground,
             .border_color = std::nullopt,
             .text_color = SK_ColorWHITE}},
           {{ui::ButtonStyle::kProminent, ColorScheme::kLight,
             ButtonState::STATE_HOVERED},
            {.background_color = color_utils::AlphaBlend(
                 SK_ColorBLACK, gfx::kColorButtonBackground, 0.2f),
             .border_color = std::nullopt,
             .text_color = SK_ColorWHITE}},
           {{ui::ButtonStyle::kProminent, ColorScheme::kDark,
             ButtonState::STATE_HOVERED},
            {.background_color = color_utils::AlphaBlend(
                 SK_ColorWHITE, gfx::kColorButtonBackground, 0.2f),
             .border_color = std::nullopt,
             .text_color = SK_ColorWHITE}},
           {{ui::ButtonStyle::kProminent, ColorScheme::kLight,
             ButtonState::STATE_DISABLED},
            {.background_color = gfx::kColorButtonDisabled,
             .border_color = std::nullopt,
             .text_color = gfx::kColorTextDisabled}},
           {{ui::ButtonStyle::kProminent, ColorScheme::kDark,
             ButtonState::STATE_DISABLED},
            {.background_color = gfx::kColorButtonDisabled,
             .border_color = std::nullopt,
             .text_color = gfx::kColorTextDisabledDark}},

           {{ui::ButtonStyle::kDefault, ColorScheme::kLight,
             ButtonState::STATE_NORMAL},
            {.background_color = std::nullopt,
             .border_color = gfx::kColorDividerInteractive,
             .text_color = gfx::kColorTextInteractive}},
           {{ui::ButtonStyle::kDefault, ColorScheme::kDark,
             ButtonState::STATE_NORMAL},
            {.background_color = std::nullopt,
             .border_color = gfx::kColorDividerInteractive,
             .text_color = gfx::kColorTextInteractiveDark}},
           {{ui::ButtonStyle::kDefault, ColorScheme::kLight,
             ButtonState::STATE_HOVERED},
            {
                .background_color = std::nullopt,
                .border_color = color_utils::AlphaBlend(
                    SK_ColorBLACK, gfx::kColorDividerInteractive, 0.2f),
                .text_color = color_utils::AlphaBlend(
                    SK_ColorBLACK, gfx::kColorTextInteractive, 0.2f),
            }},
           {{ui::ButtonStyle::kDefault, ColorScheme::kDark,
             ButtonState::STATE_HOVERED},
            {
                .background_color = std::nullopt,
                .border_color = color_utils::AlphaBlend(
                    SK_ColorWHITE, gfx::kColorDividerInteractive, 0.2f),
                .text_color = color_utils::AlphaBlend(
                    SK_ColorWHITE, gfx::kColorTextInteractiveDark, 0.2f),
            }},
           {{ui::ButtonStyle::kDefault, ColorScheme::kLight,
             ButtonState::STATE_DISABLED},
            {.background_color = std::nullopt,
             .border_color = gfx::kColorButtonDisabled,
             .text_color = gfx::kColorTextDisabled}},
           {{ui::ButtonStyle::kDefault, ColorScheme::kDark,
             ButtonState::STATE_DISABLED},
            {.background_color = std::nullopt,
             .border_color = gfx::kColorButtonDisabled,
             .text_color = gfx::kColorTextDisabledDark}},

           {{ui::ButtonStyle::kTonal, ColorScheme::kLight,
             ButtonState::STATE_NORMAL},
            {.background_color = std::nullopt,
             .border_color = std::nullopt,
             .text_color = gfx::kColorTextInteractive}},
           {{ui::ButtonStyle::kTonal, ColorScheme::kDark,
             ButtonState::STATE_NORMAL},
            {.background_color = std::nullopt,
             .border_color = std::nullopt,
             .text_color = gfx::kColorTextInteractiveDark}},
           {{ui::ButtonStyle::kTonal, ColorScheme::kLight,
             ButtonState::STATE_HOVERED},
            {.background_color =
                 SkColorSetA(gfx::kColorButtonBackground, 0xFF * 0.05),
             .border_color = std::nullopt,
             .text_color = color_utils::AlphaBlend(
                 SK_ColorBLACK, gfx::kColorTextInteractive, 0.2f)}},
           {{ui::ButtonStyle::kTonal, ColorScheme::kDark,
             ButtonState::STATE_HOVERED},
            {.background_color =
                 SkColorSetA(gfx::kColorTextInteractiveDark, 0xFF * 0.1),
             .border_color = std::nullopt,
             .text_color = color_utils::AlphaBlend(
                 SK_ColorWHITE, gfx::kColorTextInteractiveDark, 0.2f)}},
           {{ui::ButtonStyle::kTonal, ColorScheme::kLight,
             ButtonState::STATE_DISABLED},
            {.background_color = std::nullopt,
             .border_color = std::nullopt,
             .text_color = gfx::kColorTextDisabled}},
           {{ui::ButtonStyle::kTonal, ColorScheme::kDark,
             ButtonState::STATE_DISABLED},
            {.background_color = std::nullopt,
             .border_color = std::nullopt,
             .text_color = gfx::kColorTextDisabledDark}},

           {{ui::ButtonStyle::kText, ColorScheme::kLight,
             ButtonState::STATE_NORMAL},
            {.background_color = std::nullopt,
             .border_color = std::nullopt,
             .text_color = gfx::kColorTextSecondary}},
           {{ui::ButtonStyle::kText, ColorScheme::kDark,
             ButtonState::STATE_NORMAL},
            {.background_color = std::nullopt,
             .border_color = std::nullopt,
             .text_color = gfx::kColorTextSecondaryDark}},
           {{ui::ButtonStyle::kText, ColorScheme::kLight,
             ButtonState::STATE_HOVERED},
            {.background_color = SkColorSetA(gfx::kColorGray60, 0xFF * 0.05),
             .border_color = std::nullopt,
             .text_color = color_utils::AlphaBlend(
                 SK_ColorBLACK, gfx::kColorTextSecondary, 0.2f)}},
           {{ui::ButtonStyle::kText, ColorScheme::kDark,
             ButtonState::STATE_HOVERED},
            {.background_color = SkColorSetA(gfx::kColorGray20, 0xFF * 0.1),
             .border_color = std::nullopt,
             .text_color = color_utils::AlphaBlend(
                 SK_ColorWHITE, gfx::kColorTextSecondaryDark, 0.2f)}},
           {{ui::ButtonStyle::kText, ColorScheme::kLight,
             ButtonState::STATE_DISABLED},
            {.background_color = std::nullopt,
             .border_color = std::nullopt,
             .text_color = gfx::kColorTextDisabled}},
           {{ui::ButtonStyle::kText, ColorScheme::kDark,
             ButtonState::STATE_DISABLED},
            {.background_color = std::nullopt,
             .border_color = std::nullopt,
             .text_color = gfx::kColorTextDisabledDark}}});

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

MdTextButton::MdTextButton(
    PressedCallback callback,
    const std::u16string& text,
    int button_context,
    bool use_text_color_for_icon,
    std::unique_ptr<LabelButtonImageContainer> image_container)
    : MdTextButtonBase(std::move(callback),
                       text,
                       button_context,
                       use_text_color_for_icon,
                       std::move(image_container)) {
  // TODO(simonhong): Use --leo-radius-l when it's available in ui layer.
  constexpr int kLeoRadiusL = 12;
  SetCornerRadius(kLeoRadiusL);
  views::HighlightPathGenerator::Install(
      this, std::make_unique<BraveTextButtonHighlightPathGenerator>());

  // Disabled upstream's ink-drop as we have specific color for hover state.
  InkDrop::Get(this)->SetMode(views::InkDropHost::InkDropMode::OFF);
  SetImageLabelSpacing(6);
}

MdTextButton::~MdTextButton() = default;

SkPath MdTextButton::GetHighlightPath() const {
  SkPath path;
  float radius = GetCornerRadiusValue();
  path.addRRect(
      SkRRect::MakeRectXY(RectToSkRect(GetLocalBounds()), radius, radius));
  return path;
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

  // Use explicitely set color instead of our default colors except for
  // prominent style. As we have specific bg color for prominent, need to use
  // our text color for this style.
  if (style_ != ui::ButtonStyle::kProminent && explicitly_set_normal_color()) {
    return;
  }

  // Don't set MdTextButton's color as explicitly_set_color.
  // As below LabelButton::SetTextColor() sets its color args as
  // expliclity_set_color, we cache current explicitly_set_colors here
  // back to original after calling SetTextColor().
  // We(also upstream) uses it to check whether the client of MdTextButton
  // sets another color.
  const auto colors = explicitly_set_colors();
  auto button_colors = GetButtonColors();
  SetTextColor(GetVisualState(), button_colors.text_color);
  set_explicitly_set_colors(colors);
}

void MdTextButton::UpdateBackgroundColor() {
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
    // Usually, only set for normal state if we want to use same image for all
    // state. However, upstream MdTextButton updates left-padding when it has
    // image. As it uses HasImage(GetVisualState()) for checking image,
    // different padding could be used if we don't set image for all state.
    SetImageModel(ButtonState::STATE_NORMAL,
                  ui::ImageModel::FromVectorIcon(*icon_, GetCurrentTextColor(),
                                                 icon_size_));
    SetImageModel(ButtonState::STATE_HOVERED,
                  ui::ImageModel::FromVectorIcon(*icon_, GetCurrentTextColor(),
                                                 icon_size_));
    SetImageModel(ButtonState::STATE_PRESSED,
                  ui::ImageModel::FromVectorIcon(*icon_, GetCurrentTextColor(),
                                                 icon_size_));
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

  // Leo buttons don't have a pressed state, so use the normal state instead.
  if (state == ButtonState::STATE_PRESSED) {
    state = ButtonState::STATE_NORMAL;
  }

  // The loading style is the normal button style, with some opacity.
  if (loading_) {
    state = ButtonState::STATE_NORMAL;
    opacity = kLoadingOpacity;
  }

  MdTextButtonStyleKey style_lookup{GetBraveStyle(), color_scheme, state};
  auto it = GetButtonThemes().find(style_lookup);
  if (it == GetButtonThemes().end()) {
    NOTREACHED() << "No button theme found for : "
                 << static_cast<int>(GetBraveStyle()) << ", ColorScheme: "
                 << (color_scheme == ColorScheme::kDark ? "dark" : "light")
                 << ", ButtonState: " << state;
  }
  const auto& style = it->second;
  return {.background_color = AddOpacity(
              GetBgColorOverrideDeprecated().value_or(
                  style.background_color.value_or(SK_ColorTRANSPARENT)),
              opacity),
          .stroke_color = AddOpacity(
              style.border_color.value_or(SK_ColorTRANSPARENT), opacity),
          .text_color = AddOpacity(style.text_color, opacity)};
}

ui::ButtonStyle MdTextButton::GetBraveStyle() const {
  const auto style = GetStyle();
  if (style == ui::ButtonStyle::kTonal && use_default_for_tonal_) {
    return ui::ButtonStyle::kDefault;
  }
  return style;
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
