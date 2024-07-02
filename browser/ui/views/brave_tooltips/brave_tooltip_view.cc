/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/brave_tooltips/brave_tooltip_view.h"

#include <memory>
#include <string>

#include "brave/app/vector_icons/vector_icons.h"
#include "brave/browser/ui/brave_tooltips/bounds_util.h"
#include "brave/browser/ui/views/brave_tooltips/brave_tooltip_popup.h"
#include "brave/grit/brave_generated_resources.h"
#include "ui/accessibility/ax_enums.mojom.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/compositor/layer.h"
#include "ui/gfx/font.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/vector2d.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/native_theme/native_theme.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/view.h"

namespace {

constexpr int kBodyMaximumLines = 4;

constexpr gfx::Size kTooltipSize(434 + 15, 104 + 15);
constexpr gfx::Size kTitleSize(200, 20);
constexpr gfx::Size kBodySize(279, 72);
constexpr gfx::Size kButtonSize(82, 24);

constexpr char kFontName[] = "Roboto";
constexpr gfx::Font::FontStyle kFontStyle = gfx::Font::NORMAL;

constexpr int kBodyFontSize = 12;
constexpr int kTitleFontSize = 14;

constexpr int kBodyLineHeight = 16;
constexpr int kTitleLineHeight = 20;

constexpr float kButtonCornerRadius = 48.0;

constexpr gfx::Font::Weight kBodyFontWeight = gfx::Font::Weight::NORMAL;
constexpr gfx::Font::Weight kTitleFontWeight = gfx::Font::Weight::SEMIBOLD;

constexpr SkColor kDefaultButtonColor = SkColorSetRGB(0x4C, 0x54, 0xD2);
constexpr SkColor kLightModeButtonColor = SkColorSetRGB(0xFF, 0xFF, 0xFF);
constexpr SkColor kDarkModeButtonColor = SkColorSetRGB(0x3B, 0x3E, 0x4F);
constexpr SkColor kLightModeDefaultButtonTextColor =
    SkColorSetRGB(0xFF, 0xFF, 0xFF);
constexpr SkColor kDarkModeDefaultButtonTextColor =
    SkColorSetRGB(0xFF, 0xFF, 0xFF);
constexpr SkColor kLightModeButtonTextColor = SkColorSetRGB(0x49, 0x50, 0x57);
constexpr SkColor kDarkModeButtonTextColor = SkColorSetRGB(0xF0, 0xF2, 0xFF);
constexpr SkColor kLightModeBodyTextColor = SkColorSetRGB(0x49, 0x50, 0x57);
constexpr SkColor kDarkModeBodyTextColor = SkColorSetRGB(0xC2, 0xC4, 0xCF);
constexpr SkColor kLightModeTitleTextColor = SkColorSetRGB(0x21, 0x25, 0x29);
constexpr SkColor kDarkModeTitleTextColor = SkColorSetRGB(0xFF, 0xFF, 0xFF);
constexpr SkColor kIconColor = SkColorSetRGB(0xE3, 0x24, 0x44);

constexpr gfx::HorizontalAlignment kTitleHorizontalAlignment = gfx::ALIGN_LEFT;
constexpr gfx::HorizontalAlignment kBodyHorizontalAlignment = gfx::ALIGN_LEFT;

constexpr gfx::VerticalAlignment kTitleVerticalAlignment = gfx::ALIGN_TOP;
constexpr gfx::VerticalAlignment kBodyVerticalAlignment = gfx::ALIGN_TOP;

constexpr gfx::ElideBehavior kTitleElideBehavior = gfx::ELIDE_TAIL;
constexpr gfx::ElideBehavior kBodyElideBehavior = gfx::ELIDE_TAIL;

constexpr auto kTooltipViewInsets = gfx::Insets::TLBR(10, 20, 10, 10);

}  // namespace

namespace brave_tooltips {

BraveTooltipView::BraveTooltipView(
    BraveTooltipPopup* tooltip_popup,
    const BraveTooltipAttributes& tooltip_attributes)
    : tooltip_popup_(tooltip_popup), tooltip_attributes_(tooltip_attributes) {
  SetSize(kTooltipSize);
  CreateView();
}

BraveTooltipView::~BraveTooltipView() = default;

void BraveTooltipView::GetAccessibleNodeData(ui::AXNodeData* node_data) {
  node_data->role = ax::mojom::Role::kGenericContainer;
  node_data->AddStringAttribute(
      ax::mojom::StringAttribute::kRoleDescription,
      l10n_util::GetStringUTF8(
          IDS_BRAVE_TOOLTIPS_BRAVE_TOOLTIP_ACCESSIBLE_NAME));

  if (accessible_name_.empty()) {
    node_data->SetNameFrom(ax::mojom::NameFrom::kAttributeExplicitlyEmpty);
  }

  node_data->SetName(accessible_name_);
}

bool BraveTooltipView::OnMousePressed(const ui::MouseEvent& event) {
  initial_mouse_pressed_location_ = event.location();

  return true;
}

bool BraveTooltipView::OnMouseDragged(const ui::MouseEvent& event) {
  const gfx::Vector2d movement =
      event.location() - initial_mouse_pressed_location_;

  if (!is_dragging_ && ExceededDragThreshold(movement)) {
    is_dragging_ = true;
  }

  if (!is_dragging_) {
    return false;
  }

  gfx::Rect bounds = tooltip_popup_->CalculateBounds(false) + movement;
  const gfx::NativeView native_view = GetWidget()->GetNativeView();
  AdjustBoundsToFitWorkAreaForNativeView(&bounds, native_view);
  GetWidget()->SetBounds(bounds);

  return true;
}

void BraveTooltipView::OnMouseReleased(const ui::MouseEvent& event) {
  if (is_dragging_) {
    is_dragging_ = false;
    return;
  }

  if (!event.IsOnlyLeftMouseButton()) {
    return;
  }

  View::OnMouseReleased(event);
}

void BraveTooltipView::OnDeviceScaleFactorChanged(
    float old_device_scale_factor,
    float new_device_scale_factor) {
  GetWidget()->DeviceScaleFactorChanged(old_device_scale_factor,
                                        new_device_scale_factor);
}

void BraveTooltipView::OnThemeChanged() {
  views::View::OnThemeChanged();

  UpdateTitleLabelColors();
  UpdateBodyLabelColors();
  UpdateOkButtonColors();
  UpdateCancelButtonColors();

  SchedulePaint();
}

///////////////////////////////////////////////////////////////////////////////

void BraveTooltipView::CreateView() {
  SetFocusBehavior(FocusBehavior::ALWAYS);

  // Paint to a dedicated layer to make the layer non-opaque
  SetPaintToLayer();
  layer()->SetFillsBoundsOpaquely(false);

  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal, kTooltipViewInsets, 39));

  // Container
  views::View* container_view = new views::View();
  container_view->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical, gfx::Insets(), 4));
  container_view->SetPreferredSize(kBodySize);
  AddChildView(container_view);

  // Header
  views::View* header_view = CreateHeaderView();
  container_view->AddChildView(header_view);

  // Body
  views::View* body_view = CreateBodyView();
  container_view->AddChildView(body_view);

  // Buttons
  views::View* button_view = CreateButtonView();
  AddChildView(button_view);
}

void BraveTooltipView::Close() {
  if (is_closing_) {
    return;
  }

  is_closing_ = true;

  if (tooltip_popup_) {
    tooltip_popup_->Close(/* by_user */ true);
  }
}

views::View* BraveTooltipView::CreateHeaderView() {
  views::View* view = new views::View();

  view->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal, gfx::Insets(), 5));

  views::ImageView* icon_view = CreateIconView();
  view->AddChildView(icon_view);

  DCHECK(!title_label_);
  title_label_ = CreateTitleLabel();
  view->AddChildView(title_label_.get());

  return view;
}

views::ImageView* BraveTooltipView::CreateIconView() {
  views::ImageView* view = new views::ImageView();

  view->SetImage(
      gfx::CreateVectorIcon(kBraveTooltipsStopwatchIcon, kIconColor));

  return view;
}

views::Label* BraveTooltipView::CreateTitleLabel() {
  views::Label* label = new views::Label(tooltip_attributes_.title());

  const gfx::FontList font_list({kFontName}, kFontStyle, kTitleFontSize,
                                kTitleFontWeight);
  label->SetFontList(font_list);

  label->SetHorizontalAlignment(kTitleHorizontalAlignment);
  label->SetVerticalAlignment(kTitleVerticalAlignment);

  label->SetElideBehavior(kTitleElideBehavior);

  label->SetLineHeight(kTitleLineHeight);
  label->SetMaxLines(1);
  label->SetMultiLine(false);
  label->SetAllowCharacterBreak(false);

  label->SetSize(kTitleSize);

  label->SetHandlesTooltips(false);

  UpdateTitleLabelColors();

  return label;
}

void BraveTooltipView::UpdateTitleLabelColors() {
  if (!title_label_) {
    return;
  }

  const bool should_use_dark_colors = GetNativeTheme()->ShouldUseDarkColors();
  title_label_->SetEnabledColor(should_use_dark_colors
                                    ? kDarkModeTitleTextColor
                                    : kLightModeTitleTextColor);
  title_label_->SetBackgroundColor(SK_ColorTRANSPARENT);
}

views::View* BraveTooltipView::CreateButtonView() {
  views::View* view = new views::View();
  view->SetPreferredSize(kButtonSize);

  view->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical, gfx::Insets(), 7));

  DCHECK(!ok_button_);
  ok_button_ = CreateOkButton();
  view->AddChildView(ok_button_.get());

  if (tooltip_attributes_.cancel_button_text() != u"") {
    DCHECK(!cancel_button_);
    cancel_button_ = CreateCancelButton();
    view->AddChildView(cancel_button_.get());
  }

  return view;
}

views::View* BraveTooltipView::CreateBodyView() {
  views::View* view = new views::View();

  view->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal, gfx::Insets()));

  DCHECK(!body_label_);
  body_label_ = CreateBodyLabel();
  view->AddChildView(body_label_.get());

  return view;
}

views::Label* BraveTooltipView::CreateBodyLabel() {
  views::Label* label = new views::Label(tooltip_attributes_.body());

  const gfx::FontList font_list({kFontName}, kFontStyle, kBodyFontSize,
                                kBodyFontWeight);
  label->SetFontList(font_list);

  label->SetHorizontalAlignment(kBodyHorizontalAlignment);
  label->SetVerticalAlignment(kBodyVerticalAlignment);

  label->SetElideBehavior(kBodyElideBehavior);

  label->SetLineHeight(kBodyLineHeight);
  label->SetMaxLines(kBodyMaximumLines);
  label->SetMultiLine(true);
  label->SetAllowCharacterBreak(true);

  label->SetSize(kBodySize);

  label->SetHandlesTooltips(false);

  label->SizeToFit(kBodySize.width());

  UpdateBodyLabelColors();

  return label;
}

void BraveTooltipView::UpdateBodyLabelColors() {
  if (!body_label_) {
    return;
  }

  const bool should_use_dark_colors = GetNativeTheme()->ShouldUseDarkColors();
  body_label_->SetEnabledColor(should_use_dark_colors
                                   ? kDarkModeBodyTextColor
                                   : kLightModeBodyTextColor);
  body_label_->SetBackgroundColor(SK_ColorTRANSPARENT);
}

BraveTooltipLabelButton* BraveTooltipView::CreateOkButton() {
  auto* button = new BraveTooltipLabelButton(
      base::BindRepeating(&BraveTooltipView::OnOkButtonPressed,
                          base::Unretained(this)),
      tooltip_attributes_.ok_button_text());

  button->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  button->SetMinSize(kButtonSize);
  button->SetMaxSize(kButtonSize);
  button->SetIsDefault(true);

  // Make button focusable for keyboard navigation.
  button->SetFocusBehavior(views::View::FocusBehavior::ALWAYS);

  UpdateOkButtonColors();

  return button;
}

void BraveTooltipView::UpdateOkButtonColors() {
  if (!ok_button_) {
    return;
  }

  const bool should_use_dark_colors = GetNativeTheme()->ShouldUseDarkColors();
  ok_button_->SetBackground(views::CreateRoundedRectBackground(
      kDefaultButtonColor, kButtonCornerRadius));
  ok_button_->SetTextColor(views::Button::ButtonState::STATE_DISABLED,
                           should_use_dark_colors
                               ? kDarkModeDefaultButtonTextColor
                               : kLightModeDefaultButtonTextColor);
  ok_button_->SetEnabledTextColors(should_use_dark_colors
                                       ? kDarkModeDefaultButtonTextColor
                                       : kLightModeDefaultButtonTextColor);
}

void BraveTooltipView::OnOkButtonPressed() {
  if (tooltip_popup_) {
    tooltip_popup_->OnOkButtonPressed();
  }
}

BraveTooltipLabelButton* BraveTooltipView::CreateCancelButton() {
  auto* button = new BraveTooltipLabelButton(
      base::BindRepeating(&BraveTooltipView::OnCancelButtonPressed,
                          base::Unretained(this)),
      tooltip_attributes_.cancel_button_text());

  button->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  button->SetMinSize(kButtonSize);
  button->SetMaxSize(kButtonSize);
  button->SetEnabled(tooltip_attributes_.cancel_button_enabled());

  // Make button focusable for keyboard navigation.
  button->SetFocusBehavior(views::View::FocusBehavior::ALWAYS);

  UpdateCancelButtonColors();

  return button;
}

void BraveTooltipView::UpdateCancelButtonColors() {
  if (!cancel_button_) {
    return;
  }

  const bool should_use_dark_colors = GetNativeTheme()->ShouldUseDarkColors();
  cancel_button_->SetBackground(views::CreateRoundedRectBackground(
      should_use_dark_colors ? kDarkModeButtonColor : kLightModeButtonColor,
      kButtonCornerRadius));
  cancel_button_->SetTextColor(views::Button::ButtonState::STATE_DISABLED,
                               should_use_dark_colors
                                   ? kDarkModeButtonTextColor
                                   : kLightModeButtonTextColor);
  cancel_button_->SetEnabledTextColors(should_use_dark_colors
                                           ? kDarkModeButtonTextColor
                                           : kLightModeButtonTextColor);
}

void BraveTooltipView::OnCancelButtonPressed() {
  if (tooltip_popup_) {
    tooltip_popup_->OnCancelButtonPressed();
  }
}

BEGIN_METADATA(BraveTooltipView)
END_METADATA

}  // namespace brave_tooltips
