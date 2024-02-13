/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/psst/psst_consent_dialog.h"

#include <iostream>
#include <limits>
#include <memory>
#include <utility>

#include "base/functional/callback.h"
#include "base/functional/callback_forward.h"
#include "brave/grit/brave_generated_resources.h"
#include "brave/grit/brave_theme_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/models/image_model.h"
#include "ui/views/layout/box_layout_view.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/metadata/view_factory.h"
#include "ui/views/view_class_properties.h"

PsstConsentDialog::PsstConsentDialog(base::OnceClosure yes_callback,
                                     base::OnceClosure no_callback) {
  set_margins(gfx::Insets(40));
  SetModalType(ui::MODAL_TYPE_CHILD);
  SetShowCloseButton(false);
  SetButtonLabel(ui::DIALOG_BUTTON_OK,
                 l10n_util::GetStringUTF16(IDS_PSST_CONSENT_DIALOG_OK));
  SetButtonLabel(ui::DIALOG_BUTTON_CANCEL,
                 l10n_util::GetStringUTF16(IDS_PSST_CONSENT_DIALOG_CANCEL));

  SetLayoutManager(std::make_unique<views::FillLayout>());

  views::Label* header = nullptr;
  views::Label* body = nullptr;

  AddChildView(
      views::Builder<views::BoxLayoutView>()
          .SetOrientation(views::BoxLayout::Orientation::kVertical)
          .SetCrossAxisAlignment(views::BoxLayout::CrossAxisAlignment::kStretch)
          .AddChild(
              views::Builder<views::Label>()
                  .CopyAddressTo(&header)
                  .SetText(
                      l10n_util::GetStringUTF16(IDS_PSST_CONSENT_DIALOG_HEADER))
                  .SetHorizontalAlignment(gfx::HorizontalAlignment::ALIGN_LEFT))
          .AddChild(
              views::Builder<views::Label>()
                  .CopyAddressTo(&body)
                  .SetText(
                      l10n_util::GetStringUTF16(IDS_PSST_CONSENT_DIALOG_BODY))
                  .SetMultiLine(true)
                  .SetHorizontalAlignment(gfx::HorizontalAlignment::ALIGN_LEFT))
          .Build());

  CHECK(header);
  CHECK(body);

  const auto& header_font_list = header->font_list();
  header->SetFontList(
      header_font_list.DeriveWithSizeDelta(16 - header_font_list.GetFontSize())
          .DeriveWithWeight(gfx::Font::Weight::SEMIBOLD));
  header->SetProperty(views::kMarginsKey, gfx::Insets().set_bottom(16));

  const auto& body_font_list = body->font_list();
  body->SetFontList(
      body_font_list.DeriveWithSizeDelta(14 - body_font_list.GetFontSize()));

  SetAcceptCallback(std::move(yes_callback));
  SetCancelCallback(std::move(no_callback));
  // TODO(shivan): what about SetCloseCallback?

  std::cerr << "done with PsstConsentDialog" << std::endl;
}

PsstConsentDialog::~PsstConsentDialog() = default;

gfx::Size PsstConsentDialog::CalculatePreferredSize() const {
  auto bounded_size = DialogDelegateView::CalculatePreferredSize();
  bounded_size.SetToMin(gfx::Size(500, std::numeric_limits<int>::max()));
  return bounded_size;
}

void PsstConsentDialog::WindowClosing() {
  DialogDelegateView::WindowClosing();
}
