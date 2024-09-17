/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/location_bar/brave_search_conversion/promotion_button_view.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "brave/grit/brave_generated_resources.h"
#include "brave/grit/brave_theme_resources.h"
#include "chrome/browser/ui/layout_constants.h"
#include "ui/base/cursor/mojom/cursor_type.mojom.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/models/image_model.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/animation/slide_animation.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/image/image_util.h"
#include "ui/views/background.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/painter.h"

using views::BoxLayout;

namespace {
constexpr int kIconSize = 16;

// Subclass for using different cursor type.
class CustomImageView : public views::ImageView {
  METADATA_HEADER(CustomImageView, views::ImageView)

 public:
  using views::ImageView::ImageView;

  // views::ImageView overrides:
  ui::Cursor GetCursor(const ui::MouseEvent& event) override {
    return ui::mojom::CursorType::kHand;
  }
};

BEGIN_METADATA(CustomImageView)
END_METADATA

class CustomImageButton : public views::ImageButton {
  METADATA_HEADER(CustomImageButton, views::ImageButton)

 public:
  using views::ImageButton::ImageButton;

  // views::ImageButton overrides:
  ui::Cursor GetCursor(const ui::MouseEvent& event) override {
    return ui::mojom::CursorType::kHand;
  }
};

BEGIN_METADATA(CustomImageButton)
END_METADATA

base::TimeDelta GetAnimationDuration(base::TimeDelta duration) {
  return gfx::Animation::ShouldRenderRichAnimation() ? duration
                                                     : base::TimeDelta();
}

}  // namespace

PromotionButtonView::PromotionButtonView() {
  // Hovering on close button should not make this as normal state.
  SetNotifyEnterExitOnChild(true);

  // Unretained here is safe as this callback is used by itself.
  SetCallback(base::BindOnce(&PromotionButtonView::OnButtonPressed,
                             base::Unretained(this)));
  SetLayoutManager(
      std::make_unique<BoxLayout>(BoxLayout::Orientation::kHorizontal,
                                  gfx::Insets::VH(0, 6),
                                  /*between_child_spacing*/ 4))
      ->set_cross_axis_alignment(BoxLayout::CrossAxisAlignment::kCenter);
  SetTooltipText(
      l10n_util::GetStringUTF16(IDS_BRAVE_SEARCH_CONVERSION_BUTTON_LABEL));
  AddChildViews();
  animation_ = std::make_unique<gfx::SlideAnimation>(this);
  animation_->SetSlideDuration(GetAnimationDuration(base::Milliseconds(250)));
  Update();
}

PromotionButtonView::~PromotionButtonView() = default;

void PromotionButtonView::UpdateTargetProviderImage(const gfx::Image& image) {
  target_provider_image_->SetImage(
      gfx::ResizedImage(image, {kIconSize, kIconSize}).AsImageSkia());
}

void PromotionButtonView::AnimateExpand() {
  animation_->Reset();
  animation_->Show();
}

void PromotionButtonView::SetDismissedCallback(base::OnceClosure callback) {
  dismissed_callback_ = std::move(callback);
}

void PromotionButtonView::SetMakeDefaultCallback(base::OnceClosure callback) {
  make_default_callback_ = std::move(callback);
}

void PromotionButtonView::StateChanged(views::Button::ButtonState old_state) {
  Update();
}

void PromotionButtonView::OnThemeChanged() {
  Button::OnThemeChanged();

  SetupShadow();
  Update();
}

gfx::Size PromotionButtonView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  if (!animation_->is_animating()) {
    return Button::CalculatePreferredSize(available_size);
  }

  const auto size = GetLayoutManager()->GetPreferredSize(this);
  const int target_width = size.width() * animation_->GetCurrentValue();
  return {target_width, size.height()};
}

ui::Cursor PromotionButtonView::GetCursor(const ui::MouseEvent& event) {
  return ui::mojom::CursorType::kHand;
}

void PromotionButtonView::AnimationProgressed(const gfx::Animation* animation) {
  if (animation != animation_.get()) {
    Button::AnimationProgressed(animation);
    return;
  }

  PreferredSizeChanged();
}

float PromotionButtonView::GetCornerRadius() const {
  return GetLayoutConstant(LOCATION_BAR_CHILD_CORNER_RADIUS);
}

void PromotionButtonView::UpdateBackgroundAndBorders() {
  auto* cp = GetColorProvider();
  if (!cp) {
    return;
  }

  const auto bg_color =
      cp->GetColor(GetState() == views::Button::STATE_NORMAL
                       ? kColorSearchConversionButtonBackground
                       : kColorSearchConversionButtonBackgroundHovered);
  const auto stroke_color = cp->GetColor(kColorSearchConversionButtonBorder);
  SetBackground(views::CreateBackgroundFromPainter(
      views::Painter::CreateRoundRectWith1PxBorderPainter(
          bg_color, stroke_color, GetCornerRadius(), SkBlendMode::kSrcOver,
          true /* antialias */, true /* should_border_scale */)));
}

void PromotionButtonView::AddChildViews() {
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  target_provider_image_ = AddChildView(std::make_unique<CustomImageView>());
  target_provider_image_->SetPreferredSize(gfx::Size{kIconSize, kIconSize});

  AddChildView(std::make_unique<CustomImageView>(ui::ImageModel::FromVectorIcon(
      kLeoCaratLastIcon, kColorSearchConversionButtonCaratRight, 14)));
  AddChildView(std::make_unique<CustomImageView>(
      ui::ImageModel::FromImageSkia(*rb.GetImageSkiaNamed(
          IDR_BRAVE_SEARCH_CONVERSION_BUTTON_BRAVE_SEARCH_ICON))));

  auto title_font_list =
      views::Label::GetDefaultFontList()
          .DeriveWithWeight(gfx::Font::Weight::SEMIBOLD)
          .DeriveWithStyle(gfx::Font::NORMAL)
          .DeriveWithHeightUpperBound(18)
          .DeriveWithSizeDelta(
              12 - views::Label::GetDefaultFontList().GetFontSize());
  views::Label::CustomFont custom_font{title_font_list};
  auto* button_label = AddChildView(std::make_unique<views::Label>(
      l10n_util::GetStringUTF16(IDS_BRAVE_SEARCH_CONVERSION_BUTTON_LABEL),
      custom_font));
  button_label->SetHorizontalAlignment(gfx::HorizontalAlignment::ALIGN_LEFT);
  button_label->SetVerticalAlignment(gfx::VerticalAlignment::ALIGN_MIDDLE);
  button_label->SetEnabledColorId(kColorSearchConversionButtonText);
  button_label->SetBackgroundColor(SK_ColorTRANSPARENT);

  auto set_image = [](views::ImageButton* close_button,
                      views::Button::ButtonState state, int color_id) {
    close_button->SetImageModel(state, ui::ImageModel::FromVectorIcon(
                                           kLeoCloseCircleIcon, color_id, 16));
  };
  auto* close_button = AddChildView(std::make_unique<CustomImageButton>());

  // Unretained here is safe as |close_button| is the child of this class.
  close_button->SetCallback(base::BindOnce(&PromotionButtonView::OnClosePressed,
                                           base::Unretained(this)));
  set_image(close_button, views::Button::STATE_NORMAL,
            kColorSearchConversionButtonCloseButton);
  set_image(close_button, views::Button::STATE_HOVERED,
            kColorSearchConversionButtonCloseButtonHovered);
  close_button->SetTooltipText(l10n_util::GetStringUTF16(
      IDS_BRAVE_SEARCH_CONVERSION_CLOSE_BUTTON_TOOLTIP));
}

void PromotionButtonView::Update() {
  UpdateBackgroundAndBorders();
  UpdateShadow();
}

void PromotionButtonView::SetupShadow() {
  auto* cp = GetColorProvider();
  if (!cp) {
    return;
  }

  ViewShadow::ShadowParameters shadow_config1{
      .offset_x = 0,
      .offset_y = 1,
      .blur_radius = 0,
      .shadow_color = cp->GetColor(kColorSearchConversionButtonShadow1)};

  const int radius = GetCornerRadius();
  ViewShadow::ShadowParameters shadow_config2{
      .offset_x = 0,
      .offset_y = 1,
      .blur_radius = radius,
      .shadow_color = cp->GetColor(kColorSearchConversionButtonShadow2)};

  shadow1_ = std::make_unique<ViewShadow>(this, radius, shadow_config1);
  shadow2_ = std::make_unique<ViewShadow>(this, radius, shadow_config2);
}

void PromotionButtonView::UpdateShadow() {
  if (!shadow1_ || !shadow2_) {
    return;
  }

  const bool is_hovered = GetState() == views::Button::STATE_HOVERED;
  shadow1_->SetVisible(is_hovered);
  shadow2_->SetVisible(is_hovered);
}

void PromotionButtonView::OnButtonPressed() {
  if (make_default_callback_) {
    std::move(make_default_callback_).Run();
  }
}

void PromotionButtonView::OnClosePressed() {
  if (dismissed_callback_) {
    std::move(dismissed_callback_).Run();
  }
}

BEGIN_METADATA(PromotionButtonView)
END_METADATA
