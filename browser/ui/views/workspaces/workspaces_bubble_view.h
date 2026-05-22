/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_WORKSPACES_WORKSPACES_BUBBLE_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_WORKSPACES_WORKSPACES_BUBBLE_VIEW_H_

#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"

class Profile;

class WorkspacesBubbleView : public views::BubbleDialogDelegateView {
  METADATA_HEADER(WorkspacesBubbleView, views::BubbleDialogDelegateView)

 public:
  static void Show(views::View* anchor_view, Profile* profile);

  WorkspacesBubbleView(views::View* anchor_view, Profile* profile);
  ~WorkspacesBubbleView() override = default;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_WORKSPACES_WORKSPACES_BUBBLE_VIEW_H_
