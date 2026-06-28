/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/workspaces/workspaces_bubble_controller.h"

#include <memory>
#include <utility>

#include "base/functional/bind.h"
#include "base/location.h"
#include "base/memory/ptr_util.h"
#include "base/task/single_thread_task_runner.h"
#include "brave/browser/ui/views/workspaces/save_workspace_dialog.h"
#include "brave/browser/ui/views/workspaces/workspaces_bubble_view.h"
#include "components/constrained_window/constrained_window_views.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

WorkspacesBubbleController::WorkspacesBubbleController() = default;

WorkspacesBubbleController::~WorkspacesBubbleController() = default;

void WorkspacesBubbleController::ShowBubble(views::View* anchor_view,
                                            Profile* profile) {
  if (bubble_widget_) {
    return;
  }

  // Capture what the save dialog needs: the profile and the browser window
  // that hosts the anchor (used to parent the modal).
  profile_ = profile;
  parent_window_ = anchor_view->GetWidget()->GetNativeWindow();

  auto bubble = std::make_unique<WorkspacesBubbleView>(
      anchor_view, profile,
      base::BindRepeating(&WorkspacesBubbleController::ShowSaveDialog,
                          weak_factory_.GetWeakPtr()));

  // CreateBubble uses CLIENT_OWNS_WIDGET and routes close through |on_close|.
  bubble_widget_ = views::BubbleDialogDelegate::CreateBubble(
      bubble.release(),
      base::BindOnce(&WorkspacesBubbleController::OnBubbleClosed,
                     weak_factory_.GetWeakPtr()));
  bubble_widget_->Show();
}

void WorkspacesBubbleController::ShowSaveDialog() {
  if (save_dialog_widget_) {
    return;
  }

  // SaveWorkspaceDialog sets CLIENT_OWNS_WIDGET on itself; the returned Widget
  // is therefore owned here.
  auto dialog = std::make_unique<SaveWorkspaceDialog>(profile_);
  save_dialog_widget_ =
      base::WrapUnique(constrained_window::CreateBrowserModalDialogViews(
          dialog.release(), parent_window_));
  save_dialog_widget_->MakeCloseSynchronous(
      base::BindOnce(&WorkspacesBubbleController::OnSaveDialogClosed,
                     weak_factory_.GetWeakPtr()));
  save_dialog_widget_->Show();
}

void WorkspacesBubbleController::OnBubbleClosed(views::Widget::ClosedReason) {
  base::SingleThreadTaskRunner::GetCurrentDefault()->DeleteSoon(
      FROM_HERE, bubble_widget_.release());
}

void WorkspacesBubbleController::OnSaveDialogClosed(
    views::Widget::ClosedReason) {
  base::SingleThreadTaskRunner::GetCurrentDefault()->DeleteSoon(
      FROM_HERE, save_dialog_widget_.release());
}
