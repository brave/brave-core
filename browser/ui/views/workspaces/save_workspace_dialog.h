/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_WORKSPACES_SAVE_WORKSPACE_DIALOG_H_
#define BRAVE_BROWSER_UI_VIEWS_WORKSPACES_SAVE_WORKSPACE_DIALOG_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/controls/textfield/textfield_controller.h"
#include "ui/views/view.h"
#include "ui/views/window/dialog_delegate.h"

class Profile;

namespace views {
class Textfield;
}  // namespace views

// A modal dialog that prompts the user to name a new workspace. On accept, the
// current state of all open windows for the profile is saved to disk.
//
// Rather than inherit from the deprecated views::DialogDelegateView (a combined
// delegate and View), this inherits from views::DialogDelegate and installs a
// separate contents view via SetContentsView(). It also serves as the name
// field's controller so the OK button is enabled only while a name has been
// entered. WorkspacesBubbleController owns the delegate and defers its
// deletion via DeleteSoon once the widget's own destruction is complete.
class SaveWorkspaceDialog : public views::DialogDelegate,
                            public views::TextfieldController {
 public:
  // |profile| supplies the workspace service saved to and must outlive this
  // dialog.
  explicit SaveWorkspaceDialog(Profile* profile);
  ~SaveWorkspaceDialog() override;

  SaveWorkspaceDialog(const SaveWorkspaceDialog&) = delete;
  SaveWorkspaceDialog& operator=(const SaveWorkspaceDialog&) = delete;

 private:
  void OnAccept();

  // views::TextfieldController:
  void ContentsChanged(views::Textfield* sender,
                       const std::u16string& new_contents) override;

  raw_ptr<Profile> profile_;
  // Cached text value updated by ContentsChanged(). Storing a string rather
  // than a raw_ptr<Textfield> avoids a dangling pointer: the Textfield is
  // owned by the widget's view hierarchy, which is destroyed before the
  // delegate (SaveWorkspaceDialog) is deleted.
  std::string workspace_name_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_WORKSPACES_SAVE_WORKSPACE_DIALOG_H_
