/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_tooltips/brave_tooltip_view.h"

#include <memory>
#include <string>

#include "brave/app/vector_icons/vector_icons.h"
#include "brave/browser/ui/brave_tooltips/bounds_util.h"
#include "brave/browser/ui/brave_tooltips/brave_tooltip_popup.h"
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
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/view.h"

namespace {

const int kBodyMaximumLines = 3;

const gfx::Size kTooltipSize(424, 94);
const gfx::Size kTitleSize(200, 20);
const gfx::Size kBodySize(279, 48);
const gfx::Size kButtonSize(72, 24);

constexpr char kFontName[] = "Roboto";
const gfx::Font::FontStyle kFontStyle = gfx::Font::NORMAL;

const int kBodyFontSize = 12;
const int kTitleFontSize = 14;

const int kBodyLineHeight = 16;
const int kTitleLineHeight = 20;

const float kButtonCornerRadius = 48.0;

const int kTooltipBorderThickness = 6;

const gfx::Font::Weight kBodyFontWeight = gfx::Font::Weight::NORMAL;
const gfx::Font::Weight kTitleFontWeight = gfx::Font::Weight::SEMIBOLD;

constexpr SkColor kTooltipBorderColor = SkColorSetRGB(0xF7, 0x3A, 0x1C);
constexpr SkColor kDefaultButtonColor = SkColorSetRGB(0x4C, 0x54, 0xD2);
constexpr SkColor kDefaultButtonTextColor = SkColorSetRGB(0xFF, 0xFF, 0xFF);
constexpr SkColor kButtonColor = SkColorSetRGB(0xFF, 0xFF, 0xFF);
constexpr SkColor kButtonTextColor = SkColorSetRGB(0x49, 0x50, 0x57);
constexpr SkColor kBodyTextColor = SkColorSetRGB(0x49, 0x50, 0x57);
constexpr SkColor kTitleTextColor = SkColorSetRGB(0x21, 0x25, 0x29);
constexpr SkColor kIconColor = SkColorSetRGB(0xE3, 0x24, 0x44);

const gfx::HorizontalAlignment kTitleHorizontalAlignment = gfx::ALIGN_LEFT;
const gfx::HorizontalAlignment kBodyHorizontalAlignment = gfx::ALIGN_LEFT;

const gfx::VerticalAlignment kTitleVerticalAlignment = gfx::ALIGN_TOP;
const gfx::VerticalAlignment kBodyVerticalAlignment = gfx::ALIGN_TOP;

const gfx::ElideBehavior kTitleElideBehavior = gfx::ELIDE_TAIL;
const gfx::ElideBehavior kBodyElideBehavior = gfx::ELIDE_TAIL;

constexpr gfx::Insets kTooltipViewInsets(10, 20, 10, 10);

}  // namespace

namespace brave_tooltips {

BraveTooltipView::BraveTooltipView(const BraveTooltip& tooltip)
    : tooltip_(tooltip) {
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

  gfx::Rect bounds =
      GetWidget()->GetContentsView()->GetBoundsInScreen() + movement;
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

void BraveTooltipView::OnThemeChanged() {
  views::View::OnThemeChanged();
  SchedulePaint();
}

///////////////////////////////////////////////////////////////////////////////

void BraveTooltipView::CreateView() {
  SetFocusBehavior(FocusBehavior::ALWAYS);

  SetBorder(views::CreateSolidSidedBorder(0, kTooltipBorderThickness, 0, 0,
                                          kTooltipBorderColor));

  // Paint to a dedicated layer to make the layer non-opaque
  SetPaintToLayer();
  layer()->SetFillsBoundsOpaquely(false);

  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal, kTooltipViewInsets, 39));

  // Container
  views::View* container_view = new views::View();
  container_view->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical, gfx::Insets(), 4));
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

  const std::string id = tooltip_.id();
  BraveTooltipPopup::Close(id, /* by_user */ true);
}

views::View* BraveTooltipView::CreateHeaderView() {
  views::View* view = new views::View();

  view->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal, gfx::Insets(), 5));

  views::ImageView* icon_view = CreateIconView();
  view->AddChildView(icon_view);

  DCHECK(!title_label_);
  title_label_ = CreateTitleLabel();
  view->AddChildView(title_label_);

  return view;
}

views::ImageView* BraveTooltipView::CreateIconView() {
  views::ImageView* view = new views::ImageView();

  view->SetImage(
      gfx::CreateVectorIcon(kBraveTooltipsStopwatchIcon, kIconColor));

  return view;
}

views::Label* BraveTooltipView::CreateTitleLabel() {
  views::Label* label = new views::Label(tooltip_.attributes().title());

  const gfx::FontList font_list({kFontName}, kFontStyle, kTitleFontSize,
                                kTitleFontWeight);
  label->SetFontList(font_list);

  label->SetEnabledColor(kTitleTextColor);
  label->SetBackgroundColor(SK_ColorTRANSPARENT);

  label->SetHorizontalAlignment(kTitleHorizontalAlignment);
  label->SetVerticalAlignment(kTitleVerticalAlignment);

  label->SetElideBehavior(kTitleElideBehavior);

  label->SetLineHeight(kTitleLineHeight);
  label->SetMaxLines(1);
  label->SetMultiLine(false);
  label->SetAllowCharacterBreak(false);

  label->SetSize(kTitleSize);

  label->SetHandlesTooltips(false);

  return label;
}

views::View* BraveTooltipView::CreateButtonView() {
  views::View* view = new views::View();

  view->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical, gfx::Insets(), 7));

  DCHECK(!ok_button_);
  ok_button_ = CreateOkButton();
  view->AddChildView(ok_button_);

  if (tooltip_.attributes().cancel_button_text() != u"") {
    DCHECK(!cancel_button_);
    cancel_button_ = CreateCancelButton();
    view->AddChildView(cancel_button_);
  }

  return view;
}

views::View* BraveTooltipView::CreateBodyView() {
  views::View* view = new views::View();

  view->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal, gfx::Insets()));

  DCHECK(!body_label_);
  body_label_ = CreateBodyLabel();
  view->AddChildView(body_label_);

  return view;
}

views::Label* BraveTooltipView::CreateBodyLabel() {
  views::Label* label = new views::Label(tooltip_.attributes().body());

  const gfx::FontList font_list({kFontName}, kFontStyle, kBodyFontSize,
                                kBodyFontWeight);
  label->SetFontList(font_list);

  label->SetEnabledColor(kBodyTextColor);
  label->SetBackgroundColor(SK_ColorTRANSPARENT);

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

  return label;
}

views::LabelButton* BraveTooltipView::CreateOkButton() {
  views::LabelButton* button = new views::LabelButton(
      base::BindRepeating(&BraveTooltipView::OnOkButtonPressed,
                          base::Unretained(this)),
      tooltip_.attributes().ok_button_text());

  button->SetBackground(views::CreateRoundedRectBackground(
      kDefaultButtonColor, kButtonCornerRadius));
  button->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  button->SetEnabledTextColors(kDefaultButtonTextColor);
  button->SetMinSize(kButtonSize);
  button->SetMaxSize(kButtonSize);
  button->SetIsDefault(true);

  // Make button focusable for keyboard navigation.
  button->SetFocusBehavior(views::View::FocusBehavior::ALWAYS);

  return button;
}

void BraveTooltipView::OnOkButtonPressed() {
  const std::string id = tooltip_.id();
  BraveTooltipPopup::OnOkButtonPressed(id);
}

views::LabelButton* BraveTooltipView::CreateCancelButton() {
  views::LabelButton* button = new views::LabelButton(
      base::BindRepeating(&BraveTooltipView::OnCancelButtonPressed,
                          base::Unretained(this)),
      tooltip_.attributes().cancel_button_text());

  button->SetBackground(
      views::CreateRoundedRectBackground(kButtonColor, kButtonCornerRadius));
  button->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  button->SetEnabledTextColors(kButtonTextColor);
  button->SetMinSize(kButtonSize);
  button->SetMaxSize(kButtonSize);
  button->SetEnabled(tooltip_.attributes().cancel_button_enabled());

  // Make button focusable for keyboard navigation.
  button->SetFocusBehavior(views::View::FocusBehavior::ALWAYS);

  return button;
}

void BraveTooltipView::OnCancelButtonPressed() {
  const std::string id = tooltip_.id();
  BraveTooltipPopup::OnCancelButtonPressed(id);
}

BEGIN_METADATA(BraveTooltipView, views::View)
END_METADATA

}  // namespace brave_tooltips
