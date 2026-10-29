/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_WORKSPACES_WORKSPACES_BUBBLE_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_WORKSPACES_WORKSPACES_BUBBLE_VIEW_H_

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/uuid.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/view.h"

class BrowserWindowInterface;

// Bubble that lists the saved workspaces ("spaces") for the active profile and
// exposes actions to switch to, delete, or create one. The window's own
// default leads the list, separated from the workspaces shared across the
// profile. The bubble is the primary entry point for the workspaces UI and is
// anchored to the workspaces button in the tab strip.
//
// Rather than inherit from the deprecated views::BubbleDialogDelegateView
// (a combined delegate and View), this inherits from
// views::BubbleDialogDelegate and installs a separate contents view via
// SetContentsView(). Its widget uses the CLIENT_OWNS_WIDGET model and is owned
// by WorkspacesBubbleController.
class WorkspacesBubbleView : public views::BubbleDialogDelegate {
 public:
  // Internal name set on the bubble's widget; lets tests identify it.
  static constexpr char kWidgetName[] = "WorkspacesBubbleView";

  // |browser| is the window the bubble acts on: workspaces load into it, and
  // it supplies the profile whose workspace service is used for lookups. It
  // must outlive this bubble. |on_new_space| is run when the user activates
  // the New Space action.
  WorkspacesBubbleView(views::View* anchor_view,
                       BrowserWindowInterface* browser,
                       base::RepeatingClosure on_new_space);
  ~WorkspacesBubbleView() override;

 private:
  void OnNewSpaceClicked();
  void OnUnmanagedSelected();
  void OnWorkspaceSelected(const base::Uuid& id);
  void OnDeleteClicked(const base::Uuid& id);

  const raw_ptr<BrowserWindowInterface> browser_;
  base::RepeatingClosure on_new_space_;
  base::WeakPtrFactory<WorkspacesBubbleView> weak_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_WORKSPACES_WORKSPACES_BUBBLE_VIEW_H_
