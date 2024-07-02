/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/brave_ads/text_notification_ad_view.h"

#include <memory>
#include <utility>

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

namespace brave_ads {

namespace {

#if BUILDFLAG(IS_WIN)
constexpr int kNotificationWidth = 336;
#elif BUILDFLAG(IS_MAC)
constexpr int kNotificationWidth = 344;
#elif BUILDFLAG(IS_LINUX)
constexpr int kNotificationWidth = 370;
#endif

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
constexpr int kBodyLineSpacing = 5;
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

  DeprecatedLayoutImmediately();
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
  auto header_container_view = std::make_unique<views::View>();
  views::BoxLayout* header_layout = header_container_view->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal, gfx::Insets()));
  NotificationAdHeaderView* header_view =
      header_container_view->AddChildView(CreateHeaderView(notification_ad));
  header_container_view->AddChildView(
      std::make_unique<NotificationAdControlButtonsView>(*this));

  header_layout->SetFlexForView(header_view, 1);
  header_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kEnd);

  AddChildView(std::move(header_container_view));

  // Body
  auto body_container_view = std::make_unique<views::View>();
  views::BoxLayout* body_layout =
      body_container_view->SetLayoutManager(std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal,
          kContainerViewInsideBorderInsets));

  body_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStart);

  views::View* body_view =
      body_container_view->AddChildView(CreateBodyView(notification_ad));
  body_layout->SetFlexForView(body_view, 1);

  AddChildView(std::move(body_container_view));
}

std::unique_ptr<NotificationAdHeaderView>
TextNotificationAdView::CreateHeaderView(
    const NotificationAd& notification_ad) {
  std::unique_ptr<NotificationAdHeaderView> view =
      std::make_unique<NotificationAdHeaderView>();

  view->SetTitle(notification_ad.title());
  view->SetTitleElideBehavior(kTitleElideBehavior);

  return view;
}

std::unique_ptr<views::View> TextNotificationAdView::CreateBodyView(
    const NotificationAd& notification_ad) {
  auto view = std::make_unique<views::View>();

  view->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical, gfx::Insets()));

  view->SetBorder(views::CreateEmptyBorder(kBodyViewBorderInsets));

  CHECK(!body_label_);
  body_label_ = view->AddChildView(CreateBodyLabel(notification_ad));

  return view;
}

std::unique_ptr<views::Label> TextNotificationAdView::CreateBodyLabel(
    const NotificationAd& notification_ad) {
  const std::u16string body = notification_ad.body();

  auto label = std::make_unique<views::Label>(body);

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
