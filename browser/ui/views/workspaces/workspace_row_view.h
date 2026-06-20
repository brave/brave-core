/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_WORKSPACES_WORKSPACE_ROW_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_WORKSPACES_WORKSPACE_ROW_VIEW_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "ui/menus/simple_menu_model.h"
#include "ui/views/view.h"

struct WorkspaceMetadata;

namespace views {
class ImageButton;
class MenuRunner;
}  // namespace views

// Delta from regular size for the title text.
inline constexpr int kTitleFontSizeDelta = 2;

// A workspace list row that highlights its background on hover and shows a
// darker tint when selected.  SetNotifyEnterExitOnChild propagates mouse
// enter/exit from child buttons up to this view so the whole row responds.
class WorkspaceRowView : public views::View,
                         public ui::SimpleMenuModel::Delegate {
  using RowClickedCallback = base::RepeatingCallback<void()>;

  METADATA_HEADER(WorkspaceRowView, views::View)
 public:
  WorkspaceRowView(const WorkspaceMetadata& info,
                   RowClickedCallback on_workspace_selected,
                   RowClickedCallback on_delete_clicked);
  ~WorkspaceRowView() override;

  void SetSelected(bool selected);

  // views::View:
  void OnMouseEntered(const ui::MouseEvent& event) override;
  void OnMouseExited(const ui::MouseEvent& event) override;

  // ui::SimpleMenuModel::Delegate:
  void ExecuteCommand(int command_id, int event_flags) override;

 private:
  void UpdateBackground();
  void UpdateChildSelectionStyling();
  void ShowMoreMenu();

  bool hovered_ = false;
  bool selected_ = false;

  RowClickedCallback on_delete_;
  std::unique_ptr<ui::SimpleMenuModel> menu_model_;
  std::unique_ptr<views::MenuRunner> menu_runner_;
  raw_ptr<views::ImageButton> more_button_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_WORKSPACES_WORKSPACE_ROW_VIEW_H_
