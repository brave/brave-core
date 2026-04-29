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

class WorkspacesBubbleView : public views::BubbleDialogDelegateView {
  METADATA_HEADER(WorkspacesBubbleView, views::BubbleDialogDelegateView)

 public:
  static void Show(views::View* anchor_view, Browser* browser);

  WorkspacesBubbleView(views::View* anchor_view, Browser* browser);
  ~WorkspacesBubbleView() override;

 private:
  void OnSaveClicked();
  void OnWorkspaceSelected(const std::string& name);
  void OnDeleteClicked(const std::string& name);

  raw_ptr<Browser> browser_;
  base::WeakPtrFactory<WorkspacesBubbleView> weak_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_WORKSPACES_WORKSPACES_BUBBLE_VIEW_H_
