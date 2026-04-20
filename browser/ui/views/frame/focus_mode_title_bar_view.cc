/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/focus_mode_title_bar_view.h"

#include <utility>

#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/tab_ui_helper.h"
#include "components/tabs/public/tab_interface.h"
#include "components/url_formatter/url_formatter.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/models/image_model.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_skia_operations.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "url/gurl.h"

namespace {

constexpr int kTitleBarHeight = 20;
constexpr int kFaviconSize = 12;
constexpr int kLabelFontSize = 11;
constexpr int kFaviconLabelSpacing = 4;

tabs::TabInterface* GetTabInterface(content::WebContents* web_contents) {
  return web_contents ? tabs::TabInterface::MaybeGetFromContents(web_contents)
                      : nullptr;
}

}  // namespace

FocusModeTitleBarView::FocusModeTitleBarView() {
  auto* layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal, gfx::Insets(),
      kFaviconLabelSpacing));
  layout->set_main_axis_alignment(views::BoxLayout::MainAxisAlignment::kCenter);
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  SetPreferredSize(gfx::Size(0, kTitleBarHeight));

  favicon_ = AddChildView(std::make_unique<views::ImageView>());
  favicon_->SetImageSize(gfx::Size(kFaviconSize, kFaviconSize));
  favicon_->SetVisible(false);

  domain_label_ = AddChildView(std::make_unique<views::Label>());
  const gfx::FontList base_font;
  domain_label_->SetFontList(
      base_font.DeriveWithSizeDelta(kLabelFontSize - base_font.GetFontSize()));
  domain_label_->SetAutoColorReadabilityEnabled(false);
}

FocusModeTitleBarView::~FocusModeTitleBarView() = default;

void FocusModeTitleBarView::SetWebContents(content::WebContents* web_contents) {
  tab_ui_updated_subscription_.reset();
  web_contents_ = web_contents;

  if (auto* interface = GetTabInterface(web_contents_)) {
    tab_ui_updated_subscription_ =
        TabUIHelper::From(interface)->AddTabUIChangeCallback(
            base::BindRepeating(&FocusModeTitleBarView::Update,
                                base::Unretained(this)));
  }

  Update();
}

void FocusModeTitleBarView::OnThemeChanged() {
  views::View::OnThemeChanged();
  if (const auto* color_provider = GetColorProvider()) {
    domain_label_->SetEnabledColor(color_provider->GetColor(kColorToolbarText));
  }
}

void FocusModeTitleBarView::Update() {
  auto* interface = GetTabInterface(web_contents_);
  if (!interface) {
    return;
  }

  auto* tab_ui_helper = TabUIHelper::From(interface);
  CHECK(tab_ui_helper);

  GURL domain_url = tab_ui_helper->GetVisibleURL();
  if (tab_ui_helper->GetLastCommittedURL().is_valid()) {
    domain_url = tab_ui_helper->GetLastCommittedURL();
  }

  std::u16string domain;
  if (tab_ui_helper->ShouldDisplayURL()) {
    domain = url_formatter::FormatUrl(
        domain_url,
        url_formatter::kFormatUrlOmitDefaults |
            url_formatter::kFormatUrlOmitTrivialSubdomains |
            url_formatter::kFormatUrlOmitHTTPS |
            url_formatter::kFormatUrlTrimAfterHost,
        base::UnescapeRule::SPACES, nullptr, nullptr, nullptr);
  }

  domain_label_->SetText(domain);
  domain_label_->SetElideBehavior(domain_url.IsStandard() &&
                                          !domain_url.SchemeIsFile() &&
                                          !domain_url.SchemeIsFileSystem()
                                      ? gfx::ELIDE_HEAD
                                      : gfx::ELIDE_TAIL);

  ui::ImageModel favicon;

  if (!domain.empty()) {
    favicon = tab_ui_helper->GetFavicon();
  }

  if (favicon.IsEmpty()) {
    favicon_->SetImage(ui::ImageModel());
    favicon_->SetVisible(false);
  } else {
    bool themify_favicon = tab_ui_helper->ShouldThemifyFavicon();
    if (auto* provider = GetColorProvider(); provider && themify_favicon) {
      SkColor favicon_color = provider->GetColor(kColorBookmarkFavicon);
      if (favicon_color != SK_ColorTRANSPARENT) {
        favicon = ui::ImageModel::FromImageSkia(
            gfx::ImageSkiaOperations::CreateColorMask(
                favicon.Rasterize(provider), favicon_color));
      }
    }
    favicon_->SetImage(favicon);
    favicon_->SetVisible(true);
  }
}

BEGIN_METADATA(FocusModeTitleBarView)
END_METADATA
