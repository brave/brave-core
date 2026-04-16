/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_WORKSPACE_OPEN_WORKSPACE_DIALOG_H_
#define BRAVE_BROWSER_UI_VIEWS_WORKSPACE_OPEN_WORKSPACE_DIALOG_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/browser/workspace/brave_workspace.h"
#include "ui/views/window/dialog_delegate.h"

class Browser;

namespace views {
class LabelButton;
class ScrollView;
class View;
}  // namespace views

// A modal dialog that lists all saved workspaces and lets the user open or
// delete one.
class OpenWorkspaceDialog : public views::DialogDelegateView {
  METADATA_HEADER(OpenWorkspaceDialog, views::DialogDelegateView)
 public:
  // Shows the dialog as a browser-modal window.
  static void Show(Browser* browser);

 private:
  OpenWorkspaceDialog(Browser* browser, std::vector<WorkspaceInfo> workspaces);
  ~OpenWorkspaceDialog() override;

  OpenWorkspaceDialog(const OpenWorkspaceDialog&) = delete;
  OpenWorkspaceDialog& operator=(const OpenWorkspaceDialog&) = delete;

  // views::DialogDelegateView:
  ui::mojom::ModalType GetModalType() const override;
  std::u16string GetWindowTitle() const override;
  bool IsDialogButtonEnabled(ui::mojom::DialogButton button) const override;

  void BuildWorkspaceList();
  void OnWorkspaceSelected(int index);
  void OnAccept();
  void OnDeleteClicked();
  void OnDeleteCompleted(int index, bool success);

  raw_ptr<Browser> browser_;
  std::vector<WorkspaceInfo> workspaces_;
  int selected_index_ = -1;

  raw_ptr<views::View> list_container_ = nullptr;
  raw_ptr<views::LabelButton> delete_button_ = nullptr;

  // Tracks the currently highlighted row view.
  raw_ptr<views::View> selected_row_ = nullptr;

  base::WeakPtrFactory<OpenWorkspaceDialog> weak_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_WORKSPACE_OPEN_WORKSPACE_DIALOG_H_
