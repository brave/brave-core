/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/obsolete_system_confirm_dialog_view.h"

#include <memory>
#include <utility>

#include "base/functional/callback.h"
#include "base/notreached.h"
#include "brave/components/l10n/common/localization_util.h"
#include "brave/grit/brave_generated_resources.h"
#include "build/build_config.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window.h"
#include "components/constrained_window/constrained_window_views.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/views/controls/label.h"

namespace brave {

void ShowObsoleteSystemConfirmDialog(base::OnceCallback<void(bool)> callback) {
  if (auto* browser = chrome::FindLastActive()) {
    constrained_window::CreateBrowserModalDialogViews(
        new ObsoleteSystemConfirmDialogView(std::move(callback)),
        browser->window()->GetNativeWindow())
        ->Show();
  }
}

}  // namespace brave

ObsoleteSystemConfirmDialogView::ObsoleteSystemConfirmDialogView(
    base::OnceCallback<void(bool)> closing_callback)
    : closing_callback_(std::move(closing_callback)) {
  SetModalType(ui::mojom::ModalType::kWindow);
  SetShowCloseButton(false);
  SetUseDefaultFillLayout(true);
  SetAcceptCallback(
      base::BindOnce(&ObsoleteSystemConfirmDialogView::OnButtonPressed,
                     base::Unretained(this), true));
  SetCancelCallback(
      base::BindOnce(&ObsoleteSystemConfirmDialogView::OnButtonPressed,
                     base::Unretained(this), false));
  set_margins(gfx::Insets::VH(20, 30));

  auto* label = AddChildView(std::make_unique<views::Label>());
  label->SetMultiLine(true);
  label->SetMaximumWidth(330);
  label->SetText(brave_l10n::GetLocalizedResourceUTF16String(
      IDS_OBSOLETE_SYSTEM_CONFIRM_DIALOG_CONTENT));
  const int size_diff = 14 - views::Label::GetDefaultFontList().GetFontSize();
  label->SetFontList(views::Label::GetDefaultFontList()
                         .DeriveWithSizeDelta(size_diff)
                         .DeriveWithWeight(gfx::Font::Weight::SEMIBOLD));
  label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
}

ObsoleteSystemConfirmDialogView::~ObsoleteSystemConfirmDialogView() = default;

void ObsoleteSystemConfirmDialogView::OnButtonPressed(bool accept) {
  std::move(closing_callback_).Run(accept);
}

BEGIN_METADATA(ObsoleteSystemConfirmDialogView)
END_METADATA
