/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_IMPORTER_BRAVE_IMPORT_LOCK_DIALOG_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_IMPORTER_BRAVE_IMPORT_LOCK_DIALOG_VIEW_H_

#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/macros.h"
#include "chrome/common/importer/importer_data_types.h"
#include "ui/views/window/dialog_delegate.h"

namespace brave {

// ImportLockDialogView asks the user to shut down Chrome before starting the
// profile import.
class ImportLockDialogView : public views::DialogDelegateView {
 public:
  static void Show(gfx::NativeWindow parent,
                   ::importer::SourceProfile source_profile,
                   const base::Callback<void(bool)>& callback);

 private:
  explicit ImportLockDialogView(::importer::SourceProfile source_profile,
                                const base::Callback<void(bool)>& callback);
  ~ImportLockDialogView() override;

  // views::View:
  gfx::Size CalculatePreferredSize() const override;

  // views::DialogDelegate:
  base::string16 GetWindowTitle() const override;
  bool Accept() override;
  bool Cancel() override;

  // views::WidgetDelegate:
  bool ShouldShowCloseButton() const override;

 private:
  ::importer::SourceProfile source_profile_;

  // Called with the result of the dialog.
  base::Callback<void(bool)> callback_;

  DISALLOW_COPY_AND_ASSIGN(ImportLockDialogView);
};

}  // namespace brave

#endif  // BRAVE_BROWSER_UI_VIEWS_IMPORTER_BRAVE_IMPORT_LOCK_DIALOG_VIEW_H_
