// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "ui/views/controls/button/md_text_button.h"

#include <optional>
#include <tuple>

#include "base/check.h"
#include "base/containers/fixed_flat_map.h"
#include "base/containers/map_util.h"
#include "base/no_destructor.h"
#include "base/notreached.h"
#include "brave/ui/color/nala/nala_color_id.h"
#include "build/build_config.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/base/models/image_model.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/color/color_transform.h"
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

using ButtonState = views::Button::ButtonState;

struct ButtonStyle {
  std::optional<ui::ColorId> background_color;
  std::optional<ui::ColorId> border_color;
  std::optional<ui::ColorId> text_color;
};

struct MdTextButtonStyleKey {
  constexpr MdTextButtonStyleKey(ui::ButtonStyle style, ButtonState state)
      : style(style), state(state) {}

  ui::ButtonStyle style;
  ButtonState state;

  constexpr bool operator<(const MdTextButtonStyleKey& other) const {
    return std::tie(style, state) < std::tie(other.style, other.state);
  }
};

constexpr float kLoadingOpacity = 0.75f;

// We map our button styles to upstream style.
// Prominent, Default, Tonal, Text styles are mapped
// sequentially to our Filled, Outline, Plain and Plain-Faint.
static constexpr auto kButtonThemes =
    base::MakeFixedFlatMap<MdTextButtonStyleKey, ButtonStyle>({
        // Kind=Filled
        {{ui::ButtonStyle::kProminent, ButtonState::STATE_NORMAL},
         {.background_color = nala::kColorButtonBackground,
          .border_color = std::nullopt,
          .text_color = std::nullopt}},
        {{ui::ButtonStyle::kProminent, ButtonState::STATE_HOVERED},
         {.background_color = nala::kColorPrimary60,
          .border_color = std::nullopt,
          .text_color = std::nullopt}},

        // Kind=Outline
        {{ui::ButtonStyle::kDefault, ButtonState::STATE_NORMAL},
         {.background_color = std::nullopt,
          .border_color = nala::kColorDividerInteractive,
          .text_color = nala::kColorTextInteractive}},
        {{ui::ButtonStyle::kDefault, ButtonState::STATE_HOVERED},
         {.background_color = nala::kColorNeutral20,
          .border_color = nala::kColorPrimary30,
          .text_color = nala::kColorTextInteractive}},

        // Kind=Plain
        {{ui::ButtonStyle::kTonal, ButtonState::STATE_NORMAL},
         {.background_color = std::nullopt,
          .border_color = std::nullopt,
          .text_color = nala::kColorTextInteractive}},
        {{ui::ButtonStyle::kTonal, ButtonState::STATE_HOVERED},
         {.background_color = nala::kColorNeutral10,
          .border_color = std::nullopt,
          .text_color = nala::kColorTextInteractive}},

        // Kind=Plain-Faint
        {{ui::ButtonStyle::kText, ButtonState::STATE_NORMAL},
         {.background_color = std::nullopt,
          .border_color = std::nullopt,
          .text_color = nala::kColorTextPrimary}},
        {{ui::ButtonStyle::kText, ButtonState::STATE_HOVERED},
         {.background_color = std::nullopt,
          .border_color = std::nullopt,
          .text_color = nala::kColorTextSecondary}},
    });

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
    std::u16string_view text,
    int button_context,
    bool use_text_color_for_icon,
    std::unique_ptr<LabelButtonImageContainer> image_container)
    : MdTextButtonBase(std::move(callback),
                       text,
                       button_context,
                       use_text_color_for_icon,
                       std::move(image_container)) {
  views::HighlightPathGenerator::Install(
      this, std::make_unique<BraveTextButtonHighlightPathGenerator>());

  // Disabled upstream's ink-drop as we have specific color for hover state.
  InkDrop::Get(this)->SetMode(views::InkDropHost::InkDropMode::OFF);
  SetImageLabelSpacing(6);
}

MdTextButton::~MdTextButton() = default;

SkPath MdTextButton::GetHighlightPath() const {
  SkPath path;
  gfx::RoundedCornersF radii = GetCornerRadii();
  path.addRRect(SkRRect::MakeRectXY(RectToSkRect(GetLocalBounds()),
                                    radii.upper_left(), radii.lower_right()));
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
          colors.background_color, colors.stroke_color, GetCornerRadii())));
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

  if (state == ButtonState::STATE_DISABLED) {
    state = ButtonState::STATE_NORMAL;
    opacity = 0.5f;
  }

  MdTextButtonStyleKey style_lookup{GetBraveStyle(), state};
  const auto* style = base::FindOrNull(kButtonThemes, style_lookup);
  if (!style) {
    NOTREACHED() << "No button theme found for : "
                 << static_cast<int>(GetBraveStyle()) << ", ColorScheme: "
                 << ", ButtonState: " << state;
  }

  SkColor bg_color = GetBgColorOverrideDeprecated().value_or(
      style->background_color.has_value()
          ? GetColorProvider()->GetColor(style->background_color.value())
          : SK_ColorTRANSPARENT);
  SkColor border_color =
      style->border_color.has_value()
          ? GetColorProvider()->GetColor(style->border_color.value())
          : SK_ColorTRANSPARENT;
  SkColor text_color =
      style->text_color.has_value()
          ? GetColorProvider()->GetColor(style->text_color.value())
          : GetColorProvider()->GetColor(color_utils::IsDark(bg_color)
                                             ? nala::kColorPrimitiveNeutral90
                                             : nala::kColorPrimitiveNeutral10);
  return {.background_color = AddOpacity(bg_color, opacity),
          .stroke_color = AddOpacity(border_color, opacity),
          .text_color = AddOpacity(text_color, opacity)};
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
