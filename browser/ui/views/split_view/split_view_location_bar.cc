/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/split_view/split_view_location_bar.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "base/functional/bind.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "cc/paint/paint_flags.h"
#include "chrome/browser/ssl/security_state_tab_helper.h"
#include "components/omnibox/browser/omnibox_prefs.h"
#include "components/prefs/pref_service.h"
#include "components/url_formatter/url_formatter.h"
#include "content/public/browser/web_contents.h"
#include "net/cert/cert_status_flags.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/models/image_model.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/point.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout_view.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/view_class_properties.h"
#include "ui/views/widget/widget_delegate.h"
#include "url/url_constants.h"

SplitViewLocationBar::SplitViewLocationBar(PrefService* prefs) : prefs_(prefs) {
  constexpr auto kChildSpacing = 8;
  views::Builder<SplitViewLocationBar>(this)
      .SetBorder(
          views::CreateEmptyBorder(gfx::Insets().set_bottom(5).set_right(5)))
      .SetLayoutManager(std::make_unique<views::FillLayout>())
      .AddChild(
          views::Builder<views::BoxLayoutView>()
              .SetBetweenChildSpacing(kChildSpacing)
              .SetBorder(views::CreateEmptyBorder(
                  gfx::Insets().set_top(1).set_bottom(4).set_left_right(12, 8)))
              .AddChild(views::Builder<views::ImageView>()
                            .CopyAddressTo(&safety_icon_)
                            .SetImage(ui::ImageModel::FromVectorIcon(
                                kLeoWarningTriangleOutlineIcon,
                                kColorBraveSplitViewMenuItemIcon, 14)))
              .AddChild(
                  views::Builder<views::Label>()
                      .SetText(u"https")
                      .CopyAddressTo(&https_with_strike_)
                      .SetEnabledColorId(kColorOmniboxSecurityChipDangerous))
              .AddChild(views::Builder<views::Label>()
                            .SetText(u"://")
                            .CopyAddressTo(&scheme_separator_)
                            .SetEnabledColorId(kColorBraveSplitViewUrl))
              .AddChild(views::Builder<views::Label>()
                            .CopyAddressTo(&url_)
                            .SetEnabledColorId(kColorBraveSplitViewUrl)
                            .SetElideBehavior(gfx::ElideBehavior::ELIDE_HEAD)))
      .BuildChildren();

  for (auto url_part : {https_with_strike_, scheme_separator_, url_}) {
    // Adjust font size.
    constexpr auto kURLSize = 12;
    url_part->SetFontList(url_part->font_list().DeriveWithSizeDelta(
        kURLSize - url_part->font_list().GetFontSize()));
  }

  for (auto scheme_part : {https_with_strike_, scheme_separator_}) {
    // Remove child spacing
    scheme_part->SetProperty(views::kMarginsKey,
                             gfx::Insets().set_right(-kChildSpacing));
  }

  // Strike through the "https"
  https_with_strike_->SetFontList(
      https_with_strike_->font_list().DeriveWithStyle(
          gfx::Font::FontStyle::STRIKE_THROUGH));

  prevent_url_elision_.Init(
      omnibox::kPreventUrlElisionsInOmnibox, prefs_,
      base::BindRepeating(&SplitViewLocationBar::UpdateURLAndIcon,
                          base::Unretained(this)));
}

SplitViewLocationBar::~SplitViewLocationBar() = default;

// static
SplitViewLocationBar* SplitViewLocationBar::Create(PrefService* service,
                                                   views::View* web_view) {
  CHECK(web_view->GetWidget());
  auto* location_bar = new SplitViewLocationBar(service);

  views::Widget::InitParams params(views::Widget::InitParams::TYPE_CONTROL);
  params.parent = web_view->GetWidget()->GetNativeView();
  params.activatable = views::Widget::InitParams::Activatable::kNo;
  params.delegate = location_bar;
  auto* widget = new views::Widget();
  widget->Init(std::move(params));

  location_bar->view_observation_.Observe(web_view);
  location_bar->UpdateVisibility();
  location_bar->UpdateBounds();

  widget->Show();

  return location_bar;
}

void SplitViewLocationBar::SetWebContents(content::WebContents* web_contents) {
  if (this->web_contents() == web_contents) {
    return;
  }

  Observe(web_contents);
  UpdateURLAndIcon();
}

void SplitViewLocationBar::OnPaintBackground(gfx::Canvas* canvas) {
  auto* cp = GetColorProvider();
  CHECK(cp);

  auto path = GetBorderPath(/*close*/ true);
  cc::PaintFlags flags;
  flags.setColor(cp->GetColor(kColorToolbar));
  flags.setAntiAlias(true);
  canvas->DrawPath(path, flags);
}

void SplitViewLocationBar::OnPaintBorder(gfx::Canvas* canvas) {
  auto* cp = GetColorProvider();
  CHECK(cp);

  auto path = GetBorderPath(/*close*/ false);
  cc::PaintFlags flags;
  flags.setColor(cp->GetColor(kColorBraveSplitViewInactiveWebViewBorder));
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setStrokeWidth(2);
  canvas->DrawPath(path, flags);
}

gfx::Size SplitViewLocationBar::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  auto size = views::WidgetDelegateView::CalculatePreferredSize(available_size);
  if (auto* view = view_observation_.GetSource(); view && view->width()) {
    size.set_width(std::min(view->width(), size.width()));
  }
  return size;
}

void SplitViewLocationBar::PrimaryPageChanged(content::Page& page) {
  UpdateURLAndIcon();
}

void SplitViewLocationBar::DidChangeVisibleSecurityState() {
  UpdateURLAndIcon();
}

void SplitViewLocationBar::WebContentsDestroyed() {
  Observe(nullptr);
  UpdateURLAndIcon();
}

void SplitViewLocationBar::OnViewVisibilityChanged(views::View* observed_view,
                                                   views::View* starting_view) {
  UpdateVisibility();
}

void SplitViewLocationBar::OnViewBoundsChanged(views::View* observed_view) {
  UpdateBounds();
}

void SplitViewLocationBar::OnViewIsDeleting(views::View* observed_view) {
  view_observation_.Reset();
}

void SplitViewLocationBar::UpdateVisibility() {
  auto* view = view_observation_.GetSource();
  if (view && view->GetVisible()) {
    GetWidget()->ShowInactive();
  } else {
    GetWidget()->Hide();
  }
}

void SplitViewLocationBar::UpdateBounds() {
  auto* view = view_observation_.GetSource();
  if (!view) {
    return;
  }

  gfx::Point point;
  views::View::ConvertPointToWidget(view, &point);

  auto* widget = GetWidget();
  CHECK(widget);
  GetWidget()->SetBounds(gfx::Rect(point, GetPreferredSize()));
}

void SplitViewLocationBar::UpdateURLAndIcon() {
  if (auto* contents = web_contents()) {
    auto format = GetURLFormatType();
    if (HasCertError()) {
      // In case of cert error, we show https scheme with another view so that
      // we can strike the scheme. So we omit HTTPS here.
      format |= url_formatter::kFormatUrlOmitHTTPS;
      https_with_strike_->SetVisible(true);
      scheme_separator_->SetVisible(true);
    } else {
      https_with_strike_->SetVisible(false);
      scheme_separator_->SetVisible(false);
    }

    auto url = contents->GetLastCommittedURL();
    auto origin = url::Origin::Create(url);
    auto formatted_origin = url_formatter::FormatUrl(
        origin.opaque() ? url : origin.GetURL(), format,
        base::UnescapeRule::SPACES, nullptr, nullptr, nullptr);
    url_->SetText(formatted_origin);
    UpdateIcon();
  } else {
    https_with_strike_->SetVisible(false);
    scheme_separator_->SetVisible(false);
    url_->SetText({});
    UpdateIcon();
  }

  UpdateBounds();
}

void SplitViewLocationBar::UpdateIcon() {
  // At the moment, we show only warning icon.
  safety_icon_->SetVisible(!IsContentsSafe());
}

bool SplitViewLocationBar::IsContentsSafe() const {
  if (!web_contents()) {
    return true;
  }

  auto security_level = SecurityStateTabHelper::FromWebContents(web_contents())
                            ->GetSecurityLevel();
  return security_level == security_state::SecurityLevel::SECURE ||
         security_level == security_state::SecurityLevel::NONE;
}

bool SplitViewLocationBar::HasCertError() const {
  if (!web_contents()) {
    return false;
  }

  return net::IsCertStatusError(
      SecurityStateTabHelper::FromWebContents(web_contents())
          ->GetVisibleSecurityState()
          ->cert_status);
}

url_formatter::FormatUrlType SplitViewLocationBar::GetURLFormatType() const {
  const auto full_url_format = url_formatter::kFormatUrlOmitDefaults &
                               ~url_formatter::kFormatUrlOmitHTTP;
  if (*prevent_url_elision_) {
    return full_url_format;
  }

  if (IsContentsSafe()) {
    return url_formatter::kFormatUrlOmitDefaults |
           url_formatter::kFormatUrlOmitHTTPS;
  }

  return full_url_format;
}

SkPath SplitViewLocationBar::GetBorderPath(bool close) {
  /*
                                         //==
                                        //  <- small arc(2)
                                       ||   ^
                                      /|    |
                   Large arc  ->     //     |
                                    //      |
                                  //    ^   |
                                //      |   |
       =========================   <---- contents bounds
     //    <- small arc(1)                  |
   ||  <------------------------------------ bounds
  */

  constexpr auto kSmallArcRadius = 5;
  constexpr auto kLargeArcRadius = 12;

  auto bounds = GetLocalBounds();
  auto contents_bounds = GetContentsBounds();
  SkPath path;

  // small arc(1)
  path.moveTo(bounds.x(), bounds.bottom());
  path.rArcTo(kSmallArcRadius, -kSmallArcRadius, 0, SkPath::kSmall_ArcSize,
              SkPathDirection::kCW, kSmallArcRadius, -kSmallArcRadius);

  // proceed to large arc
  path.rLineTo(
      std::max(0, contents_bounds.width() - kSmallArcRadius - kLargeArcRadius),
      0);

  // large arc
  path.rArcTo(kLargeArcRadius, -kLargeArcRadius, 0, SkPath::kSmall_ArcSize,
              SkPathDirection::kCCW, kLargeArcRadius, -kLargeArcRadius);

  // proceed to small arc(2)
  path.lineTo(contents_bounds.right(), std::max(0, kSmallArcRadius));

  // small arc(2)
  path.rArcTo(kSmallArcRadius, -kSmallArcRadius, 0, SkPath::kSmall_ArcSize,
              SkPathDirection::kCW, kSmallArcRadius, -kSmallArcRadius);

  if (close) {
    path.lineTo(0, 0);
    path.close();
  }

  return path;
}

BEGIN_METADATA(SplitViewLocationBar)
END_METADATA
