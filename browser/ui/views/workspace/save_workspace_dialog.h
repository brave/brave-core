/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_WORKSPACE_SAVE_WORKSPACE_DIALOG_H_
#define BRAVE_BROWSER_UI_VIEWS_WORKSPACE_SAVE_WORKSPACE_DIALOG_H_

#include "base/memory/raw_ptr.h"
#include "ui/views/window/dialog_delegate.h"

class Browser;

namespace views {
class Textfield;
}  // namespace views

// A modal dialog that prompts the user to name a new workspace.  On accept,
// the current state of all open windows for the profile is saved to disk.
class SaveWorkspaceDialog : public views::DialogDelegateView {
  METADATA_HEADER(SaveWorkspaceDialog, views::DialogDelegateView)
 public:
  // Shows the dialog as a browser-modal window.
  static void Show(Browser* browser);

 private:
  explicit SaveWorkspaceDialog(Browser* browser);
  ~SaveWorkspaceDialog() override;

  SaveWorkspaceDialog(const SaveWorkspaceDialog&) = delete;
  SaveWorkspaceDialog& operator=(const SaveWorkspaceDialog&) = delete;

  // views::DialogDelegateView:
  ui::mojom::ModalType GetModalType() const override;
  std::u16string GetWindowTitle() const override;
  bool IsDialogButtonEnabled(ui::mojom::DialogButton button) const override;

  void OnAccept();

  raw_ptr<Browser> browser_;
  raw_ptr<views::Textfield> name_field_ = nullptr;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_WORKSPACE_SAVE_WORKSPACE_DIALOG_H_
