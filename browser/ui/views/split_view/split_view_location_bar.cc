/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/split_view/split_view_location_bar.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "base/strings/utf_string_conversions.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "cc/paint/paint_flags.h"
#include "chrome/browser/ssl/security_state_tab_helper.h"
#include "components/url_formatter/url_formatter.h"
#include "content/public/browser/web_contents.h"
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
#include "ui/views/widget/widget_delegate.h"
#include "url/url_constants.h"

SplitViewLocationBar::SplitViewLocationBar() {
  views::Builder<SplitViewLocationBar>(this)
      .SetBorder(
          views::CreateEmptyBorder(gfx::Insets().set_bottom(5).set_right(5)))
      .SetLayoutManager(std::make_unique<views::FillLayout>())
      .AddChild(
          views::Builder<views::BoxLayoutView>()
              .SetBetweenChildSpacing(8)
              .SetBorder(views::CreateEmptyBorder(
                  gfx::Insets().set_top(1).set_bottom(4).set_left_right(12, 8)))
              .AddChild(views::Builder<views::ImageView>()
                            .CopyAddressTo(&safety_icon_)
                            .SetImage(ui::ImageModel::FromVectorIcon(
                                kLeoWarningTriangleOutlineIcon,
                                kColorBraveSplitViewMenuItemIcon, 14)))
              .AddChild(views::Builder<views::Label>()
                            .CopyAddressTo(&url_)
                            .SetEnabledColorId(kColorBraveSplitViewUrl)))

      .BuildChildren();

  url_->SetFontList(url_->font_list().DeriveWithSizeDelta(
      12 - url_->font_list().GetFontSize()));
}

SplitViewLocationBar::~SplitViewLocationBar() = default;

// static
SplitViewLocationBar* SplitViewLocationBar::Create(views::View* web_view) {
  CHECK(web_view->GetWidget());
  auto* location_bar = new SplitViewLocationBar();

  views::Widget::InitParams params;
  params.type = views::Widget::InitParams::TYPE_CONTROL;
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
    auto url = contents->GetLastCommittedURL();
    auto origin = url::Origin::Create(url);
    auto formatted_origin = url_formatter::FormatUrl(
        origin.opaque() ? url : origin.GetURL(),
        url_formatter::kFormatUrlOmitDefaults |
            url_formatter::kFormatUrlOmitHTTPS,
        base::UnescapeRule::SPACES, nullptr, nullptr, nullptr);
    url_->SetText(formatted_origin);
    UpdateIcon();
  } else {
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

  if (SecurityStateTabHelper::FromWebContents(web_contents())
          ->GetSecurityLevel() == security_state::SecurityLevel::SECURE) {
    return true;
  }

  auto url = web_contents()->GetLastCommittedURL();
  return url.is_empty() || url.SchemeIs(url::kAboutScheme) ||
         url.SchemeIs("chrome");
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
