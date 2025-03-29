/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/shared_pinned_tab_dummy_view_views.h"

#include <memory>

#include "base/functional/bind.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/tabs/shared_pinned_tab_dummy_view.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/thumbnails/thumbnail_tab_helper.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "third_party/skia/include/core/SkPath.h"
#include "third_party/skia/include/core/SkRect.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/models/image_model.h"
#include "ui/gfx/font_list.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/view_class_properties.h"

// static
std::unique_ptr<SharedPinnedTabDummyView> SharedPinnedTabDummyView::Create(
    content::WebContents* shared_contents,
    content::WebContents* dummy_contents) {
  auto view = base::WrapUnique(
      new SharedPinnedTabDummyViewViews(shared_contents, dummy_contents));

  // Note that SharedPinnedTabDummyView is owned by the client, not by a view
  // tree. This is invariant of WebView::SetCrashedOverlayView, which we're
  // using to attach this view to web view.
  view->set_owned_by_client();

  return view;
}

SharedPinnedTabDummyViewViews::SharedPinnedTabDummyViewViews(
    content::WebContents* shared_contents,
    content::WebContents* dummy_contents)
    : shared_contents_(shared_contents),
      dummy_contents_(dummy_contents),
      thumbnail_(
          ThumbnailTabHelper::FromWebContents(shared_contents)->thumbnail()),
      subscription_(thumbnail_->Subscribe()) {
  SetPaintToLayer();
  SetLayoutManager(std::make_unique<views::FlexLayout>())
      ->SetOrientation(views::LayoutOrientation::kVertical)
      .SetCrossAxisAlignment(views::LayoutAlignment::kCenter)
      .SetMainAxisAlignment(views::LayoutAlignment::kCenter);
  SetBackground(views::CreateSolidBackground(
      kColorBraveSharedPinnedTabDummyViewBackground));

  constexpr auto kTitleFontSize = 22;
  constexpr auto kDescriptionFontSize = 14;
  constexpr auto kThumbnailSize = gfx::Size(360, 240);
  constexpr auto kThumbnailRadius = 5;
  constexpr auto kThumbnailBorderThickness = 1;
  constexpr auto kThumbnailImageSize =
      gfx::Size(kThumbnailSize.width() - kThumbnailBorderThickness * 2,
                kThumbnailSize.height() - kThumbnailBorderThickness * 2);

  views::Builder<SharedPinnedTabDummyViewViews>(this)
      .AddChild(views::Builder<views::View>()
                    .SetBorder(views::CreateRoundedRectBorder(
                        kThumbnailBorderThickness, kThumbnailRadius,
                        kColorBraveSharedPinnedTabDummyViewThumbnailBorder))
                    .SetPreferredSize(kThumbnailSize)
                    .SetLayoutManager(std::make_unique<views::FillLayout>())
                    .AddChild(views::Builder<views::ImageView>()
                                  .CopyAddressTo(&thumbnail_view_)
                                  .SetImageSize(kThumbnailImageSize)))
      .AddChild(views::Builder<views::Label>()
                    .CopyAddressTo(&title_label_)
                    .SetText(l10n_util::GetStringUTF16(
                        IDS_SHARED_PINNED_TABS_DUMMY_TAB_VIEW_TITLE))
                    .SetFontList(
                        gfx::FontList()
                            .DeriveWithSizeDelta(kTitleFontSize -
                                                 gfx::FontList().GetFontSize())
                            .DeriveWithWeight(gfx::Font::Weight::SEMIBOLD))
                    .SetEnabledColor(kColorBraveSharedPinnedTabDummyViewTitle)
                    .SetProperty(views::kMarginsKey, gfx::Insets().set_top(40)))
      .AddChild(
          views::Builder<views::Label>()
              .CopyAddressTo(&description_label_)
              .SetText(l10n_util::GetStringUTF16(
                  IDS_SHARED_PINNED_TABS_DUMMY_TAB_VIEW_DESCRIPTION))
              .SetFontList(gfx::FontList().DeriveWithSizeDelta(
                  kDescriptionFontSize - gfx::FontList().GetFontSize()))
              .SetEnabledColor(kColorBraveSharedPinnedTabDummyViewDescription)
              .SetProperty(views::kMarginsKey, gfx::Insets().set_top(8)))
      .BuildChildren();

  SkPath path;
  path.addRoundRect(
      SkRect::MakeWH(kThumbnailImageSize.width(), kThumbnailImageSize.height()),
      kThumbnailRadius - kThumbnailBorderThickness,
      kThumbnailRadius - kThumbnailBorderThickness);
  thumbnail_view_->SetClipPath(path);

  subscription_->SetSizeHint(kThumbnailImageSize);
  subscription_->SetUncompressedImageCallback(base::BindRepeating(
      [](views::ImageView* thumbnail_view, gfx::ImageSkia image) {
        thumbnail_view->SetImage(ui::ImageModel::FromImageSkia(image));
      },
      thumbnail_view_));
  thumbnail_->RequestThumbnailImage();
}

SharedPinnedTabDummyViewViews::~SharedPinnedTabDummyViewViews() = default;

void SharedPinnedTabDummyViewViews::Install() {
  auto* browser = chrome::FindBrowserWithTab(dummy_contents_);
  CHECK(browser);

  // Borrows WebView::SetCrashedOverlayView() which is used to attach SadTabView
  // over views::WebView.
  // TODO(sko) We should take split view into account. This is the same problem
  // as with SadTabView.
  static_cast<BrowserView*>(browser->window())
      ->contents_web_view()
      ->SetCrashedOverlayView(this);

  // views::WebView hides the overlay unless web contents is crashed. We should
  // forcibly show this view.
  SetVisible(true);
}

BEGIN_METADATA(SharedPinnedTabDummyViewViews)
END_METADATA
