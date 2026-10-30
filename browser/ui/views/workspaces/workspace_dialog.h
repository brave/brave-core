/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_WORKSPACES_WORKSPACE_DIALOG_H_
#define BRAVE_BROWSER_UI_VIEWS_WORKSPACES_WORKSPACE_DIALOG_H_

#include <optional>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/uuid.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/controls/textfield/textfield_controller.h"
#include "ui/views/view.h"
#include "ui/views/window/dialog_delegate.h"

class BrowserWindowInterface;

namespace views {
class Textfield;
class Widget;
}  // namespace views

// A modal dialog for naming a workspace, in one of two modes.
//
// Creating: the tabs currently open in the browser window stay where they are,
// and the window switches to a new, empty workspace.
//
// Renaming: the given workspace takes the new name. Storage is keyed by UUID,
// so nothing moves on disk.
//
// Nothing happens until the user accepts, so cancelling leaves the window
// exactly as it was.
//
// Rather than inherit from the deprecated views::DialogDelegateView (a combined
// delegate and View), this inherits from views::DialogDelegate and installs a
// separate contents view via SetContentsView(). It also serves as the name
// field's controller so the OK button is enabled only while a name has been
// entered.
class WorkspaceDialog : public views::DialogDelegate,
                              public views::TextfieldController {
 public:
  // |browser| is the window the dialog acts on, and supplies the profile whose
  // workspace service is written to. It must outlive this dialog. Passing a
  // |workspace_to_rename| puts the dialog in renaming mode; leaving it empty
  // creates a new workspace instead. The dialog configures its widget as
  // CLIENT_OWNS_WIDGET; the caller owns the resulting Widget (see
  // WorkspacesBubbleController).
  WorkspaceDialog(BrowserWindowInterface* browser,
                  std::optional<base::Uuid> workspace_to_rename);
  ~WorkspaceDialog() override;

  WorkspaceDialog(const WorkspaceDialog&) = delete;
  WorkspaceDialog& operator=(const WorkspaceDialog&) = delete;

 private:
  void OnAccept();

  // views::TextfieldController:
  void ContentsChanged(views::Textfield* sender,
                       const std::u16string& new_contents) override;

  raw_ptr<BrowserWindowInterface> browser_;
  const std::optional<base::Uuid> workspace_to_rename_;
  // Cached text value updated by ContentsChanged(). Storing a string rather
  // than a raw_ptr<Textfield> avoids a dangling pointer: the Textfield is
  // owned by the widget's view hierarchy, which is destroyed before the
  // delegate (WorkspaceDialog) is deleted.
  std::string workspace_name_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_WORKSPACES_WORKSPACE_DIALOG_H_
