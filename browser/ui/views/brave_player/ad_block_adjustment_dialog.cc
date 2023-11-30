/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/brave_player/ad_block_adjustment_dialog.h"

#include "brave/grit/brave_generated_resources.h"
#include "brave/grit/brave_theme_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/models/image_model.h"
#include "ui/views/layout/box_layout_view.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/metadata/view_factory.h"
#include "ui/views/view_class_properties.h"

AdBlockAdjustmentDialog::AdBlockAdjustmentDialog(const GURL& url) : url_(url) {
  set_margins(gfx::Insets(40));
  SetModalType(ui::MODAL_TYPE_CHILD);
  SetShowCloseButton(false);
  SetButtonLabel(ui::DIALOG_BUTTON_OK,
                 l10n_util::GetStringUTF16(
                     IDS_BRAVE_PLAYER_AD_BLOCK_ADJUSTMENT_DIALOG_OK));
  SetButtonLabel(ui::DIALOG_BUTTON_CANCEL,
                 l10n_util::GetStringUTF16(
                     IDS_BRAVE_PLAYER_AD_BLOCK_ADJUSTMENT_DIALOG_CANCEL));

  SetLayoutManager(std::make_unique<views::FillLayout>());

  views::Label* header = nullptr;
  views::Label* body = nullptr;
  views::Label* footer = nullptr;

  // TODO(sko) We'd like to use different header and body text via flag/griffin
  AddChildView(
      views::Builder<views::BoxLayoutView>()
          .SetOrientation(views::BoxLayout::Orientation::kVertical)
          .SetCrossAxisAlignment(views::BoxLayout::CrossAxisAlignment::kStretch)
          .AddChild(views::Builder<views::ImageView>()
                        .SetImage(ui::ImageModel::FromResourceId(
                            IDR_AD_BLOCK_ADJUSTMENT_DIALOG_HEADER_ICON))
                        .SetHorizontalAlignment(
                            views::ImageViewBase::Alignment::kLeading))
          .AddChild(
              views::Builder<views::Label>()
                  .CopyAddressTo(&header)
                  .SetText(l10n_util::GetStringUTF16(
                      IDS_BRAVE_PLAYER_AD_BLOCK_ADJUSTMENT_DIALOG_HEADER))
                  .SetHorizontalAlignment(gfx::HorizontalAlignment::ALIGN_LEFT))
          .AddChild(
              views::Builder<views::Label>()
                  .CopyAddressTo(&body)
                  .SetText(l10n_util::GetStringUTF16(
                      IDS_BRAVE_PLAYER_AD_BLOCK_ADJUSTMENT_DIALOG_BODY))
                  .SetMultiLine(true)
                  .SetHorizontalAlignment(gfx::HorizontalAlignment::ALIGN_LEFT))
          .AddChild(views::Builder<views::ImageView>().SetImage(
              ui::ImageModel::FromResourceId(
                  IDR_AD_BLOCK_ADJUSTMENT_DIALOG_LOCATION_BAR)))
          .AddChild(views::Builder<views::Label>()
                        .SetText(l10n_util::GetStringUTF16(
                            IDS_BRAVE_PLAYER_AD_BLOCK_ADJUSTMENT_DIALOG_FOOTER))
                        .CopyAddressTo(&footer))
          .Build());

  CHECK(header);
  CHECK(body);
  CHECK(footer);

  const auto& header_font_list = header->font_list();
  header->SetFontList(
      header_font_list.DeriveWithSizeDelta(16 - header_font_list.GetFontSize())
          .DeriveWithWeight(gfx::Font::Weight::SEMIBOLD));
  header->SetProperty(views::kMarginsKey,
                      gfx::Insets().set_top(40).set_bottom(16));

  const auto& body_font_list = body->font_list();
  body->SetFontList(
      body_font_list.DeriveWithSizeDelta(14 - body_font_list.GetFontSize()));
  body->SetProperty(views::kMarginsKey, gfx::Insets().set_bottom(24));

  const auto& footer_font_list = footer->font_list();
  footer->SetFontList(footer_font_list.DeriveWithSizeDelta(
      13 - footer_font_list.GetFontSize()));
  footer->SetProperty(views::kMarginsKey, gfx::Insets().set_top(16));

  // It's okay to bind Unretained(this) here because we're registering callback
  // from the base class.
  SetAcceptCallback(base::BindOnce(
      &AdBlockAdjustmentDialog::DisableAdBlockForSite, base::Unretained(this)));
}

AdBlockAdjustmentDialog::~AdBlockAdjustmentDialog() = default;

gfx::Size AdBlockAdjustmentDialog::CalculatePreferredSize() const {
  auto bounded_size = DialogDelegateView::CalculatePreferredSize();
  bounded_size.SetToMin(gfx::Size(500, std::numeric_limits<int>::max()));
  return bounded_size;
}

void AdBlockAdjustmentDialog::WindowClosing() {
  DialogDelegateView::WindowClosing();

  // TODO: We may want to remember we've showed this dialog for this site.
}

void AdBlockAdjustmentDialog::DisableAdBlockForSite() {
  NOTIMPLEMENTED() << url_.spec();
}
