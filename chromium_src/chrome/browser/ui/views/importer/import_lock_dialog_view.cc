/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/importer/import_lock_dialog_view.h"

#include "chrome/browser/importer/importer_lock_dialog.h"

#include "src/chrome/browser/ui/views/importer/import_lock_dialog_view.cc"

namespace importer {

void ShowImportLockDialog(gfx::NativeView parent_view,
                          gfx::NativeWindow parent,
                          base::OnceCallback<void(bool)> callback,
                          int importer_lock_title_id,
                          int importer_lock_text_id) {
  ImportLockDialogView::Show(parent_view, parent, std::move(callback),
                             importer_lock_title_id, importer_lock_text_id);
}

}  // namespace importer

// static
void ImportLockDialogView::Show(gfx::NativeView parent_view,
                                gfx::NativeWindow parent,
                                base::OnceCallback<void(bool)> callback,
                                int importer_lock_title_id,
                                int importer_lock_text_id) {
  views::DialogDelegate::CreateDialogWidget(
      new ImportLockDialogView(std::move(callback), importer_lock_title_id,
                               importer_lock_text_id),
      parent, parent_view)
      ->Show();
  base::RecordAction(UserMetricsAction("ImportLockDialogView_Shown"));
}

ui::mojom::ModalType ImportLockDialogView::GetModalType() const {
  return ui::mojom::ModalType::kChild;
}
