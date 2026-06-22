/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_WORKSPACES_WORKSPACES_BUBBLE_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_WORKSPACES_WORKSPACES_BUBBLE_VIEW_H_

#include <string>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/view.h"

class Browser;

namespace views {
class Widget;
}  // namespace views

// Bubble that lists the saved workspaces ("spaces") for the active profile and
// exposes actions to restore, delete, or save a new one. The bubble is the
// primary entry point for the workspaces UI and is anchored to the workspaces
// button in the tab strip.
//
// This combines the bubble delegate and contents view by inheriting from both
// views::BubbleDialogDelegate and views::View directly, rather than the
// deprecated views::BubbleDialogDelegateView. Its widget uses the
// CLIENT_OWNS_WIDGET model and is owned by WorkspacesBubbleController.
class WorkspacesBubbleView : public views::BubbleDialogDelegate,
                             public views::View {
  METADATA_HEADER(WorkspacesBubbleView, views::View)

 public:
  // |browser| supplies the profile used for service lookups and is the modal
  // target for the delete confirmation dialog; it must outlive this bubble.
  // |on_save_workspace| is run when the user activates the Save action.
  WorkspacesBubbleView(views::View* anchor_view,
                       Browser* browser,
                       base::RepeatingClosure on_save_workspace);
  ~WorkspacesBubbleView() override;

  // views::BubbleDialogDelegate:
  views::View* GetContentsView() override;

  // views::View:
  // Disambiguates the GetWidget() inherited from both base classes.
  views::Widget* GetWidget() override;
  const views::Widget* GetWidget() const override;

 private:
  void OnSaveClicked();
  void OnWorkspaceSelected(const std::string& name);
  void OnDeleteClicked(const std::string& name);

  const raw_ref<Browser> browser_;
  base::RepeatingClosure on_save_workspace_;
  base::WeakPtrFactory<WorkspacesBubbleView> weak_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_WORKSPACES_WORKSPACES_BUBBLE_VIEW_H_
