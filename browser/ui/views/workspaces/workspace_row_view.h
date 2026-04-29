/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_WORKSPACES_WORKSPACE_ROW_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_WORKSPACES_WORKSPACE_ROW_VIEW_H_

#include "brave/browser/workspaces/workspace_metadata.h"
#include "ui/views/view.h"

using WorkspaceRowClickedCallback = base::RepeatingCallback<void()>;

// A workspace list row that highlights its background on hover and shows a
// darker tint when selected.  SetNotifyEnterExitOnChild propagates mouse
// enter/exit from child buttons up to this view so the whole row responds.
class WorkspaceRowView : public views::View {
  METADATA_HEADER(WorkspaceRowView, views::View)
 public:
  WorkspaceRowView(const int dialog_width,
                   const int padding,
                   const int row_height,
                   const WorkspaceMetadata& info,
                   WorkspaceRowClickedCallback OnWorkspaceSelected,
                   WorkspaceRowClickedCallback OnDeleteClicked);
  void SetSelected(bool selected);
  void OnMouseEntered(const ui::MouseEvent& event) override;
  void OnMouseExited(const ui::MouseEvent& event) override;

 private:
  void FormatStats(const WorkspaceMetadata& info);
  void UpdateBackground();

  const int kDialogWidth;
  const int kPadding;
  const int kRowHeight;
  const int kRowContentWidth = kDialogWidth - kPadding * 2;
  bool hovered_ = false;
  bool selected_ = false;
  std::u16string workspace_stats_text_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_WORKSPACES_WORKSPACE_ROW_VIEW_H_
