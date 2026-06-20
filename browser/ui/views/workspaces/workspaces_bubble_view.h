/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_WORKSPACES_WORKSPACES_BUBBLE_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_WORKSPACES_WORKSPACES_BUBBLE_VIEW_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"

class Browser;

// Bubble that lists the saved workspaces ("spaces") for the active profile and
// exposes actions to restore, delete, or save a new one. The bubble is the
// primary entry point for the workspaces UI and is anchored to the workspaces
// button in the tab strip. The widget is owned by the views framework.
class WorkspacesBubbleView : public views::BubbleDialogDelegateView {
  METADATA_HEADER(WorkspacesBubbleView, views::BubbleDialogDelegateView)

 public:
  // Creates and shows the bubble anchored to |anchor_view|. |browser| supplies
  // the profile used for service lookups and is the modal target for the save
  // and delete confirmation dialogs; it must outlive this call. The resulting
  // widget is owned by the views framework and not returned.
  static void Show(views::View* anchor_view, Browser* browser);

  WorkspacesBubbleView(views::View* anchor_view, Browser* browser);
  ~WorkspacesBubbleView() override;

 private:
  void OnSaveClicked();
  void OnWorkspaceSelected(const std::string& name);
  void OnDeleteClicked(const std::string& name);

  const raw_ref<Browser> browser_;
  base::WeakPtrFactory<WorkspacesBubbleView> weak_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_WORKSPACES_WORKSPACES_BUBBLE_VIEW_H_
