/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/brave_ads/text_ad_notification_view.h"

#include <memory>
#include <string>

#include "brave/browser/ui/brave_ads/ad_notification.h"
#include "brave/browser/ui/views/brave_ads/ad_notification_control_buttons_view.h"
#include "brave/browser/ui/views/brave_ads/ad_notification_header_view.h"
#include "brave/browser/ui/views/brave_ads/insets_util.h"
#include "build/build_config.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/font.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/text_constants.h"
#include "ui/native_theme/native_theme.h"
#include "ui/views/border.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"

namespace brave_ads {

namespace {

const int kNotificationWidth = 350;
const int kNotificationHeight = 100;

constexpr gfx::Insets kContainerViewInsideBorderInsets(
    /* top */ 0,
    /* left */ 20,
    /* bottom */ 10,
    /* right */ 10);

constexpr gfx::Insets kBodyViewBorderInsets(0);

const gfx::ElideBehavior kTitleElideBehavior = gfx::ELIDE_TAIL;

constexpr char kBodyFontName[] = "Roboto";
const gfx::Font::FontStyle kBodyFontStyle = gfx::Font::NORMAL;
const int kBodyFontSize = 13;
const gfx::Font::Weight kBodyFontWeight = gfx::Font::Weight::LIGHT;
constexpr SkColor kLightModeBodyColor = SkColorSetRGB(0x45, 0x49, 0x55);
constexpr SkColor kDarkModeBodyColor = SkColorSetRGB(0xd7, 0xdb, 0xe2);

const int kBodyMaximumLines = 2;

#if defined(OS_WIN)
const int kBodyLineSpacing = 0;
#elif defined(OS_MAC)
const int kBodyLineSpacing = 2;
#elif defined(OS_LINUX)
const int kBodyLineSpacing = 2;
#endif

const gfx::HorizontalAlignment kBodyHorizontalAlignment = gfx::ALIGN_LEFT;
const gfx::VerticalAlignment kBodyVerticalAlignment = gfx::ALIGN_TOP;

const gfx::ElideBehavior kBodyElideBehavior = gfx::ELIDE_TAIL;

constexpr gfx::Insets kBodyBorderInsets(0);

}  // namespace

TextAdNotificationView::TextAdNotificationView(
    const AdNotification& ad_notification)
    : AdNotificationView(ad_notification), ad_notification_(ad_notification) {
  SetSize(gfx::Size(kNotificationWidth, kNotificationHeight));

  CreateView(ad_notification_);
}

TextAdNotificationView::~TextAdNotificationView() = default;

void TextAdNotificationView::UpdateContents(
    const AdNotification& ad_notification) {
  AdNotificationView::UpdateContents(ad_notification);

  UpdateBodyLabel();

  Layout();
  SchedulePaint();
}

void TextAdNotificationView::OnThemeChanged() {
  AdNotificationView::OnThemeChanged();

  UpdateContents(ad_notification_);
}

///////////////////////////////////////////////////////////////////////////////

void TextAdNotificationView::CreateView(const AdNotification& ad_notification) {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical, gfx::Insets()));

  // Header
  AdNotificationHeaderView* header_view = CreateHeaderView(ad_notification);
  AdNotificationControlButtonsView* control_buttons_view =
      new AdNotificationControlButtonsView(this);
  header_view->AddChildView(control_buttons_view);
  AddChildView(header_view);

  // Container
  views::View* container_view = new views::View();
  views::BoxLayout* box_layout =
      container_view->SetLayoutManager(std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal,
          kContainerViewInsideBorderInsets));

  box_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStart);

  AddChildView(container_view);

  // Body
  views::View* body_view = CreateBodyView(ad_notification);
  container_view->AddChildView(body_view);
  box_layout->SetFlexForView(body_view, 1);
}

AdNotificationHeaderView* TextAdNotificationView::CreateHeaderView(
    const AdNotification& ad_notification) {
  const int width = View::width();
  AdNotificationHeaderView* view = new AdNotificationHeaderView(width);

  view->SetTitle(ad_notification.title());
  view->SetTitleElideBehavior(kTitleElideBehavior);

  return view;
}

views::View* TextAdNotificationView::CreateBodyView(
    const AdNotification& ad_notification) {
  views::View* view = new views::View();

  view->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical, gfx::Insets()));

  view->SetBorder(views::CreateEmptyBorder(kBodyViewBorderInsets));

  DCHECK(!body_label_);
  body_label_ = CreateBodyLabel(ad_notification);
  view->AddChildView(body_label_);

  return view;
}

views::Label* TextAdNotificationView::CreateBodyLabel(
    const AdNotification& ad_notification) {
  const bool should_use_dark_colors = GetNativeTheme()->ShouldUseDarkColors();

  const std::u16string body = ad_notification.body();

  views::Label* label = new views::Label(body);

  const gfx::FontList font_list({kBodyFontName}, kBodyFontStyle, kBodyFontSize,
                                kBodyFontWeight);
  label->SetFontList(font_list);

  label->SetEnabledColor(should_use_dark_colors ? kDarkModeBodyColor
                                                : kLightModeBodyColor);
  label->SetBackgroundColor(SK_ColorTRANSPARENT);

  label->SetHorizontalAlignment(kBodyHorizontalAlignment);
  label->SetVerticalAlignment(kBodyVerticalAlignment);

  label->SetElideBehavior(kBodyElideBehavior);

  const int line_height = font_list.GetHeight() + kBodyLineSpacing;
  label->SetLineHeight(line_height);
  label->SetMaxLines(kBodyMaximumLines);
  label->SetMultiLine(true);
  label->SetAllowCharacterBreak(true);

  gfx::Insets border_insets = kBodyBorderInsets;
  AdjustInsetsForFontList(&border_insets, font_list);
  label->SetBorder(views::CreateEmptyBorder(border_insets));

  const int width = View::width() - kContainerViewInsideBorderInsets.width() -
                    border_insets.width();
  label->SizeToFit(width);

  label->SetHandlesTooltips(false);

  return label;
}

void TextAdNotificationView::UpdateBodyLabel() {
  const bool should_use_dark_colors = GetNativeTheme()->ShouldUseDarkColors();

  DCHECK(body_label_);
  body_label_->SetEnabledColor(should_use_dark_colors ? kDarkModeBodyColor
                                                      : kLightModeBodyColor);
}

}  // namespace brave_ads
