/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/brave_ads/text_notification_ad_view.h"

#include <memory>

#include "brave/browser/ui/brave_ads/notification_ad.h"
#include "brave/browser/ui/views/brave_ads/insets_util.h"
#include "brave/browser/ui/views/brave_ads/notification_ad_control_buttons_view.h"
#include "brave/browser/ui/views/brave_ads/notification_ad_header_view.h"
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

constexpr int kNotificationWidth = 350;
constexpr int kNotificationHeight = 100;

constexpr auto kContainerViewInsideBorderInsets =
    gfx::Insets::TLBR(0, 20, 10, 10);

constexpr gfx::Insets kBodyViewBorderInsets(0);

constexpr gfx::ElideBehavior kTitleElideBehavior = gfx::ELIDE_TAIL;

constexpr char kBodyFontName[] = "Roboto";
constexpr gfx::Font::FontStyle kBodyFontStyle = gfx::Font::NORMAL;
constexpr int kBodyFontSize = 13;
constexpr gfx::Font::Weight kBodyFontWeight = gfx::Font::Weight::LIGHT;
constexpr SkColor kLightModeBodyColor = SkColorSetRGB(0x45, 0x49, 0x55);
constexpr SkColor kDarkModeBodyColor = SkColorSetRGB(0xd7, 0xdb, 0xe2);

constexpr int kBodyMaximumLines = 2;

#if BUILDFLAG(IS_WIN)
constexpr int kBodyLineSpacing = 0;
#elif BUILDFLAG(IS_MAC)
constexpr int kBodyLineSpacing = 2;
#elif BUILDFLAG(IS_LINUX)
constexpr int kBodyLineSpacing = 2;
#endif

constexpr gfx::HorizontalAlignment kBodyHorizontalAlignment = gfx::ALIGN_LEFT;
constexpr gfx::VerticalAlignment kBodyVerticalAlignment = gfx::ALIGN_TOP;

constexpr gfx::ElideBehavior kBodyElideBehavior = gfx::ELIDE_TAIL;

constexpr gfx::Insets kBodyBorderInsets(0);

}  // namespace

TextNotificationAdView::TextNotificationAdView(
    const NotificationAd& notification_ad)
    : NotificationAdView(notification_ad), notification_ad_(notification_ad) {
  SetSize(gfx::Size(kNotificationWidth, kNotificationHeight));

  CreateView(notification_ad_);
}

TextNotificationAdView::~TextNotificationAdView() = default;

void TextNotificationAdView::UpdateContents(
    const NotificationAd& notification_ad) {
  NotificationAdView::UpdateContents(notification_ad);

  UpdateBodyLabel();

  Layout();
  SchedulePaint();
}

void TextNotificationAdView::OnThemeChanged() {
  NotificationAdView::OnThemeChanged();

  UpdateContents(notification_ad_);
}

///////////////////////////////////////////////////////////////////////////////

void TextNotificationAdView::CreateView(const NotificationAd& notification_ad) {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical, gfx::Insets()));

  // Header
  NotificationAdHeaderView* header_view = CreateHeaderView(notification_ad);
  NotificationAdControlButtonsView* control_buttons_view =
      new NotificationAdControlButtonsView(*this);
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
  views::View* body_view = CreateBodyView(notification_ad);
  container_view->AddChildView(body_view);
  box_layout->SetFlexForView(body_view, 1);
}

NotificationAdHeaderView* TextNotificationAdView::CreateHeaderView(
    const NotificationAd& notification_ad) {
  const int width = View::width();
  NotificationAdHeaderView* view = new NotificationAdHeaderView(width);

  view->SetTitle(notification_ad.title());
  view->SetTitleElideBehavior(kTitleElideBehavior);

  return view;
}

views::View* TextNotificationAdView::CreateBodyView(
    const NotificationAd& notification_ad) {
  views::View* view = new views::View();

  view->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical, gfx::Insets()));

  view->SetBorder(views::CreateEmptyBorder(kBodyViewBorderInsets));

  CHECK(!body_label_);
  body_label_ = CreateBodyLabel(notification_ad);
  view->AddChildView(body_label_.get());

  return view;
}

views::Label* TextNotificationAdView::CreateBodyLabel(
    const NotificationAd& notification_ad) {
  const std::u16string body = notification_ad.body();

  views::Label* label = new views::Label(body);

  const gfx::FontList font_list({kBodyFontName}, kBodyFontStyle, kBodyFontSize,
                                kBodyFontWeight);
  label->SetFontList(font_list);

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

void TextNotificationAdView::UpdateBodyLabel() {
  const bool should_use_dark_colors = GetNativeTheme()->ShouldUseDarkColors();

  CHECK(body_label_);
  body_label_->SetEnabledColor(should_use_dark_colors ? kDarkModeBodyColor
                                                      : kLightModeBodyColor);
}

}  // namespace brave_ads
