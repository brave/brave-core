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
#include "ui/views/widget/widget_observer.h"

class Profile;
class SaveWorkspaceDialog;
class WorkspacesBubbleView;

namespace views {
class View;
}  // namespace views

// Owns the workspaces bubble and the "save workspace" dialog.
//
// The bubble uses CLIENT_OWNS_WIDGET; its close callback owns cleanup.
// The save dialog uses NATIVE_WIDGET_OWNS_WIDGET (default): when dismissed the
// normal native-window close sequence runs (re-enabling the browser window) and
// the widget auto-deletes. The controller owns the delegate separately and
// defers its deletion via DeleteSoon in OnWidgetDestroyed() so the widget
// finishes its own destruction before the delegate is freed.
//
// The controller instance lives in the browser window features and gets called
// by the tab strip / toolbar view that hosts the workspaces button.
class WorkspacesBubbleController : public views::WidgetObserver {
 public:
  WorkspacesBubbleController();
  ~WorkspacesBubbleController() override;

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

  // Close handler for the bubble widget (CLIENT_OWNS_WIDGET).
  void OnBubbleClosed(views::Widget::ClosedReason reason);

  // views::WidgetObserver: fires once when the save-dialog native window is
  // destroyed, regardless of how the dialog was closed.
  void OnWidgetDestroyed(views::Widget* widget) override;

  // Captured when the bubble is shown and used to launch the save dialog.
  raw_ptr<Profile> profile_ = nullptr;
  gfx::NativeWindow parent_window_ = gfx::NativeWindow();

  // Bubble delegate and its CLIENT_OWNS_WIDGET widget (destroyed in reverse
  // declaration order: widget before delegate).
  std::unique_ptr<WorkspacesBubbleView> bubble_;
  std::unique_ptr<views::Widget> bubble_widget_;

  // Save-dialog delegate, owned here because NATIVE_WIDGET_OWNS_WIDGET does
  // not delete the delegate. Deleted via DeleteSoon in OnWidgetDestroyed().
  std::unique_ptr<SaveWorkspaceDialog> save_dialog_;

  // Non-owning pointer to the save-dialog widget (NATIVE_WIDGET_OWNS_WIDGET:
  // auto-deletes when native window closes). Cleared in OnWidgetDestroyed().
  raw_ptr<views::Widget> save_dialog_widget_ = nullptr;

  base::WeakPtrFactory<WorkspacesBubbleController> weak_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_WORKSPACES_WORKSPACES_BUBBLE_CONTROLLER_H_
