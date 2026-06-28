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

namespace views {
class View;
}  // namespace views

// Owns the workspaces bubble and the "save workspace" dialog widgets. Both use
// the CLIENT_OWNS_WIDGET ownership model, so the controller holds each
// Widget's unique_ptr and tears it down when the widget closes. The controller
// is owned by the tab strip region view that hosts the workspaces button, so
// it reliably outlives the widgets it manages.
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
  // Widget. Destruction is deferred because the Widget owns the delegate/view
  // whose code is still on the stack when this runs.
  void OnBubbleClosed(views::Widget::ClosedReason reason);
  void OnSaveDialogClosed(views::Widget::ClosedReason reason);

  // Captured when the bubble is shown and used to launch the save dialog.
  raw_ptr<Profile> profile_ = nullptr;
  gfx::NativeWindow parent_window_ = gfx::NativeWindow();

  std::unique_ptr<views::Widget> bubble_widget_;
  std::unique_ptr<views::Widget> save_dialog_widget_;

  base::WeakPtrFactory<WorkspacesBubbleController> weak_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_WORKSPACES_WORKSPACES_BUBBLE_CONTROLLER_H_
