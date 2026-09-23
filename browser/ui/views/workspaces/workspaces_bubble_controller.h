/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_WORKSPACES_WORKSPACES_BUBBLE_CONTROLLER_H_
#define BRAVE_BROWSER_UI_VIEWS_WORKSPACES_WORKSPACES_BUBBLE_CONTROLLER_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "ui/gfx/native_ui_types.h"
#include "ui/views/widget/widget.h"

class Profile;
class SaveWorkspaceDialog;
class WorkspacesBubbleView;

namespace views {
class View;
}  // namespace views

// Owns the workspaces bubble and the "save workspace" dialog. Both use the
// CLIENT_OWNS_WIDGET ownership model, and because their delegates are not also
// Views, the Widget does not delete them. The controller therefore owns both
// the delegate and its Widget, and tears them down (Widget first, then
// delegate, since the delegate must outlive the Widget) when the widget closes.
// The controller instance lives in the browser window features and gets called
// by the tab strip region view that hosts the workspaces button.
class WorkspacesBubbleController {
 public:
  WorkspacesBubbleController();
  ~WorkspacesBubbleController();

  WorkspacesBubbleController(const WorkspacesBubbleController&) = delete;
  WorkspacesBubbleController& operator=(const WorkspacesBubbleController&) =
      delete;

  // Shows the workspaces bubble anchored to |anchor_view|. |profile| is used
  // for workspace service lookups. Does nothing if the bubble is already
  // showing.
  void ShowBubble(views::View* anchor_view, Profile* profile);

 private:
  // Shows the modal "save workspace" dialog. Invoked by the bubble's Save
  // action; uses the profile and parent window captured by ShowBubble().
  void ShowSaveDialog();

  // Close handlers for the two widgets. With CLIENT_OWNS_WIDGET the close path
  // is routed here synchronously and we are responsible for destroying the
  // Widget and its delegate. Destruction is deferred because the delegate
  // (e.g. a button handler that called Widget::Close()) may still be on the
  // stack when this runs.
  void OnBubbleClosed(views::Widget::ClosedReason reason);
  void OnSaveDialogClosed(views::Widget::ClosedReason reason);

  // Captured when the bubble is shown and used to launch the save dialog.
  raw_ptr<Profile> profile_ = nullptr;
  gfx::NativeWindow parent_window_ = gfx::NativeWindow();

  // Each delegate is declared before its Widget so that, if the controller is
  // destroyed while a widget is open, the Widget is destroyed first (members
  // are destroyed in reverse declaration order).
  std::unique_ptr<WorkspacesBubbleView> bubble_;
  std::unique_ptr<views::Widget> bubble_widget_;
  std::unique_ptr<SaveWorkspaceDialog> save_dialog_;
  std::unique_ptr<views::Widget> save_dialog_widget_;

  base::WeakPtrFactory<WorkspacesBubbleController> weak_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_WORKSPACES_WORKSPACES_BUBBLE_CONTROLLER_H_
