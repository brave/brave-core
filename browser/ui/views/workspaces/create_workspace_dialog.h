/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_WORKSPACES_CREATE_WORKSPACE_DIALOG_H_
#define BRAVE_BROWSER_UI_VIEWS_WORKSPACES_CREATE_WORKSPACE_DIALOG_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/controls/textfield/textfield_controller.h"
#include "ui/views/view.h"
#include "ui/views/window/dialog_delegate.h"

class BrowserWindowInterface;

namespace views {
class Textfield;
class Widget;
}  // namespace views

// A modal dialog that prompts the user to name a new workspace. On accept, the
// tabs currently open in the browser window are stashed into whatever that
// window is showing, and the window switches to the new, empty workspace.
//
// Nothing happens until the user accepts, so cancelling leaves the window
// exactly as it was.
//
// Rather than inherit from the deprecated views::DialogDelegateView (a combined
// delegate and View), this inherits from views::DialogDelegate and installs a
// separate contents view via SetContentsView(). It also serves as the name
// field's controller so the OK button is enabled only while a name has been
// entered.
class CreateWorkspaceDialog : public views::DialogDelegate,
                              public views::TextfieldController {
 public:
  // |browser| is the window the new workspace opens in, and supplies the
  // profile whose workspace service is written to. It must outlive this
  // dialog. The
  // dialog configures its widget as CLIENT_OWNS_WIDGET; the caller owns the
  // resulting Widget (see WorkspacesBubbleController).
  explicit CreateWorkspaceDialog(BrowserWindowInterface* browser);
  ~CreateWorkspaceDialog() override;

  CreateWorkspaceDialog(const CreateWorkspaceDialog&) = delete;
  CreateWorkspaceDialog& operator=(const CreateWorkspaceDialog&) = delete;

 private:
  void OnAccept();

  // views::TextfieldController:
  void ContentsChanged(views::Textfield* sender,
                       const std::u16string& new_contents) override;

  raw_ptr<BrowserWindowInterface> browser_;
  // Cached text value updated by ContentsChanged(). Storing a string rather
  // than a raw_ptr<Textfield> avoids a dangling pointer: the Textfield is
  // owned by the widget's view hierarchy, which is destroyed before the
  // delegate (CreateWorkspaceDialog) is deleted.
  std::string workspace_name_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_WORKSPACES_CREATE_WORKSPACE_DIALOG_H_
