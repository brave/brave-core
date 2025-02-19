/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/psst/psst_consent_dialog.h"

#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/functional/callback_forward.h"
#include "base/strings/strcat.h"
#include "base/values.h"
#include "brave/grit/brave_generated_resources.h"
#include "brave/grit/brave_theme_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/models/image_model.h"
#include "ui/views/accessibility/view_accessibility.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/progress_bar.h"
#include "ui/views/layout/box_layout_view.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/metadata/view_factory.h"
#include "ui/views/view_class_properties.h"

namespace {

void CallBackWithClose(base::WeakPtr<PsstConsentDialog> dialog,
                       base::OnceClosure callback) {
  std::move(callback).Run();
  if (dialog) {
    dialog->GetWidget()->CloseWithReason(
        views::Widget::ClosedReason::kCancelButtonClicked);
  }
}
}  // namespace

PsstConsentDialog::PsstConsentDialog(bool prompt_for_new_version,
                                     const std::string& list_of_changes,
                                     base::OnceClosure yes_callback,
                                     base::OnceClosure no_callback) {
  set_margins(gfx::Insets(20));
  SetModalType(ui::mojom::ModalType::kChild);
  SetShowCloseButton(false);
  SetButtons(static_cast<int>(ui::mojom::DialogButton::kNone));

  std::u16string full_text = base::StrCat(
      {l10n_util::GetStringUTF16(IDS_PSST_CONSENT_DIALOG_BODY), u"\n\n",
       l10n_util::GetStringUTF16(IDS_PSST_CONSENT_DIALOG_BODY_LIST_OF_CHANGES),
       u"\n", base::ASCIIToUTF16(list_of_changes)});

  SetLayoutManager(std::make_unique<views::FillLayout>());

  views::Label* header = nullptr;
  views::Label* body = nullptr;

  auto box =
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
                  .SetText(full_text)
                  .SetMultiLine(true)
                  .SetHorizontalAlignment(gfx::HorizontalAlignment::ALIGN_LEFT))
          .AddChild(views::Builder<views::ProgressBar>()
                        .SetPreferredSize(gfx::Size(50, 10))
                        .CopyAddressTo(&progress_bar_)
                        .SetValue(0)
                        .SetProperty(views::kMarginsKey,
                                     gfx::Insets().set_bottom(16)));

  if (prompt_for_new_version) {
    box.AddChild(
        views::Builder<views::Label>()
            .SetText(l10n_util::GetStringUTF16(
                IDS_PSST_CONSENT_DIALOG_BODY_NEW_VERSION))
            .SetMultiLine(true)
            .SetHorizontalAlignment(gfx::HorizontalAlignment::ALIGN_LEFT));
  }

  auto button_box =
      views::Builder<views::BoxLayoutView>()
          .SetOrientation(views::BoxLayout::Orientation::kHorizontal)
          .SetMainAxisAlignment(views::LayoutAlignment::kEnd)
          .SetCrossAxisAlignment(views::BoxLayout::CrossAxisAlignment::kEnd)
          .AddChild(views::Builder<views::MdTextButton>()
                        .SetText(l10n_util::GetStringUTF16(
                            IDS_PSST_CONSENT_DIALOG_CANCEL))
                        .SetAccessibleName(u"SetAccessibleName")
                        .SetStyle(ui::ButtonStyle::kText)
                        .SetCallback(base::BindOnce(&CallBackWithClose,
                                                    weak_factory_.GetWeakPtr(),
                                                    std::move(no_callback)))
                        .SetID(1515152)
                        .SetHorizontalAlignment(
                            gfx::HorizontalAlignment::ALIGN_RIGHT))
          .AddChild(views::Builder<views::MdTextButton>()
                        .SetText(l10n_util::GetStringUTF16(
                            IDS_PSST_CONSENT_DIALOG_OK))
                        .SetAccessibleName(u"SetAccessibleName")
                        .SetStyle(ui::ButtonStyle::kDefault)
                        .SetCallback(std::move(yes_callback))
                        .SetID(1515151)
                        .SetHorizontalAlignment(
                            gfx::HorizontalAlignment::ALIGN_RIGHT));
  box.AddChild(std::move(button_box));
  AddChildView(std::move(box).Build());

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
}

PsstConsentDialog::~PsstConsentDialog() = default;

gfx::Size PsstConsentDialog::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  auto bounded_size =
      DialogDelegateView::CalculatePreferredSize(available_size);
  //   bounded_size.SetToMin(gfx::Size(500, std::numeric_limits<int>::max()));
  return bounded_size;
}

void PsstConsentDialog::WindowClosing() {
  DialogDelegateView::WindowClosing();
}

void PsstConsentDialog::SetProgressValue(const double value) {
  LOG(INFO) << "[PSST] PsstConsentDialog::SetProgressValue #100";
  if (!progress_bar_) {
    return;
  }

  LOG(INFO) << "[PSST] PsstConsentDialog::SetProgressValue value:" << value;
  progress_bar_->SetValue(std::move(value));
}
