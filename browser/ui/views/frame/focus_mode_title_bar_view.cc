/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/focus_mode_title_bar_view.h"

#include <string>

#include "base/functional/bind.h"
#include "brave/browser/ui/brave_scheme_utils.h"
#include "brave/ui/color/nala/nala_color_id.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/tab_ui_helper.h"
#include "components/url_formatter/url_formatter.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/models/image_model.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/image/image_skia_operations.h"
#include "ui/views/background.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "url/gurl.h"

namespace {

constexpr int kTitleBarHeight = 20;
constexpr int kFaviconSize = 12;
constexpr int kFaviconLabelSpacing = 4;
constexpr int kLabelFontSize = 11;

gfx::FontList GetLabelFont() {
  const gfx::FontList base;
  return base.DeriveWithSizeDelta(kLabelFontSize - base.GetFontSize());
}

}  // namespace

FocusModeTitleBarView::FocusModeTitleBarView() {
  auto* layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal, gfx::Insets(),
      kFaviconLabelSpacing));
  SetBackground(views::CreateSolidBackground(kColorToolbar));

  layout->set_main_axis_alignment(views::BoxLayout::MainAxisAlignment::kCenter);
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  SetPreferredSize(gfx::Size(0, kTitleBarHeight));

  favicon_image_ = AddChildView(std::make_unique<views::ImageView>());
  favicon_image_->SetImageSize(gfx::Size(kFaviconSize, kFaviconSize));
  favicon_image_->SetVisible(false);

  domain_label_ = AddChildView(std::make_unique<views::Label>(
      u"", views::Label::CustomFont(GetLabelFont())));
  domain_label_->SetEnabledColor(nala::kColorTextTertiary);
  domain_label_->SetAutoColorReadabilityEnabled(false);
}

FocusModeTitleBarView::~FocusModeTitleBarView() = default;

void FocusModeTitleBarView::SetTab(tabs::TabInterface* tab) {
  tab_ui_updated_subscription_ = {};
  tab_will_detach_subscription_ = {};
  tab_ = tab;

  if (tab_) {
    tab_will_detach_subscription_ =
        tab_->RegisterWillDetach(base::BindRepeating(
            &FocusModeTitleBarView::OnTabWillDetach, base::Unretained(this)));
    if (auto* helper = TabUIHelper::From(tab_)) {
      tab_ui_updated_subscription_ =
          helper->AddTabUIChangeCallback(base::BindRepeating(
              &FocusModeTitleBarView::Update, base::Unretained(this)));
    }
  }

  Update();
}

bool FocusModeTitleBarView::IsFaviconVisibleForTesting() const {
  return favicon_image_->GetVisible();
}

void FocusModeTitleBarView::Update() {
  auto* tab_ui_helper = tab_ ? TabUIHelper::From(tab_) : nullptr;
  if (!tab_ui_helper) {
    favicon_image_->SetImage(ui::ImageModel());
    favicon_image_->SetVisible(false);
    domain_label_->SetText(u"");
    return;
  }

  GURL domain_url = tab_ui_helper->GetVisibleURL();
  std::u16string domain;
  if (tab_ui_helper->ShouldDisplayURL()) {
    domain = url_formatter::FormatUrl(
        domain_url,
        (url_formatter::kFormatUrlOmitDefaults &
         ~url_formatter::kFormatUrlOmitHTTP) |
            url_formatter::kFormatUrlOmitTrivialSubdomains |
            url_formatter::kFormatUrlOmitHTTPS |
            url_formatter::kFormatUrlTrimAfterHost,
        base::UnescapeRule::SPACES, nullptr, nullptr, nullptr);
    brave_utils::ReplaceChromeToBraveScheme(domain);
  }

  domain_label_->SetText(domain);
  domain_label_->SetElideBehavior(domain_url.IsStandard() &&
                                          !domain_url.SchemeIsFile() &&
                                          !domain_url.SchemeIsFileSystem()
                                      ? gfx::ELIDE_HEAD
                                      : gfx::ELIDE_TAIL);

  if (ui::ImageModel favicon = tab_ui_helper->GetFavicon();
      !favicon.IsEmpty() && !domain.empty()) {
    bool themify_favicon = tab_ui_helper->ShouldThemifyFavicon();
    if (auto* provider = GetColorProvider(); provider && themify_favicon) {
      SkColor favicon_color = provider->GetColor(kColorBookmarkFavicon);
      if (favicon_color != SK_ColorTRANSPARENT) {
        favicon = ui::ImageModel::FromImageSkia(
            gfx::ImageSkiaOperations::CreateColorMask(
                favicon.Rasterize(provider), favicon_color));
      }
    }
    favicon_image_->SetImage(favicon);
    favicon_image_->SetVisible(true);
  } else {
    favicon_image_->SetImage(ui::ImageModel());
    favicon_image_->SetVisible(false);
  }
}

void FocusModeTitleBarView::OnTabWillDetach(
    tabs::TabInterface* tab,
    tabs::TabInterface::DetachReason reason) {
  SetTab(nullptr);
}

BEGIN_METADATA(FocusModeTitleBarView)
END_METADATA
