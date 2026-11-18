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
#include "brave/browser/ui/views/workspaces/workspace_dialog.h"
#include "brave/browser/ui/views/workspaces/workspaces_bubble_view.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "components/constrained_window/constrained_window_views.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

DEFINE_USER_DATA(WorkspacesBubbleController);

WorkspacesBubbleController::WorkspacesBubbleController(
    ui::UnownedUserDataHost& host)
    : scoped_unowned_user_data_(host, *this) {}

WorkspacesBubbleController::~WorkspacesBubbleController() = default;

// static
WorkspacesBubbleController* WorkspacesBubbleController::From(
    BrowserWindowInterface* browser) {
  return Get(browser->GetUnownedUserDataHost());
}

void WorkspacesBubbleController::ShowBubble(views::View* anchor_view,
                                            BrowserWindowInterface* browser) {
  if (bubble_widget_) {
    return;
  }

  // Capture what the workspace dialog needs: the browser window the bubble
  // acts on
  // and the native window that hosts the anchor (used to parent the modal).
  browser_ = browser;
  parent_window_ = anchor_view->GetWidget()->GetNativeWindow();

  bubble_ = std::make_unique<WorkspacesBubbleView>(
      anchor_view, browser,
      base::BindRepeating(&WorkspacesBubbleController::ShowWorkspaceDialog,
                          weak_factory_.GetWeakPtr()));

  // The controller keeps ownership of the delegate; CreateBubble uses
  // CLIENT_OWNS_WIDGET and routes close through |on_close|.
  bubble_widget_ = views::BubbleDialogDelegate::CreateBubble(
      bubble_.get(), base::BindOnce(&WorkspacesBubbleController::OnBubbleClosed,
                                    weak_factory_.GetWeakPtr()));
  bubble_widget_->Show();
}

void WorkspacesBubbleController::ShowWorkspaceDialog(
    std::optional<base::Uuid> workspace_to_rename) {
  if (create_dialog_widget_) {
    return;
  }

  // WorkspaceDialog sets CLIENT_OWNS_WIDGET on itself; the controller
  // keeps ownership of the delegate and owns the returned Widget.
  create_dialog_ = std::make_unique<WorkspaceDialog>(
      browser_, std::move(workspace_to_rename));
  create_dialog_widget_ =
      base::WrapUnique(constrained_window::CreateBrowserModalDialogViews(
          create_dialog_.get(), parent_window_));
  create_dialog_widget_->MakeCloseSynchronous(
      base::BindOnce(&WorkspacesBubbleController::OnCreateDialogClosed,
                     weak_factory_.GetWeakPtr()));
  create_dialog_widget_->Show();
}

void WorkspacesBubbleController::OnBubbleClosed(views::Widget::ClosedReason) {
  // Defer past the current stack: the delegate that requested the close may
  // still be unwinding. Ownership is kept here rather than released to
  // DeleteSoon so that a bubble closed while the window is being torn down
  // cannot outlive the window its raw_ptr refers to.
  base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&WorkspacesBubbleController::DestroyBubble,
                                weak_factory_.GetWeakPtr()));
}

void WorkspacesBubbleController::DestroyBubble() {
  bubble_widget_.reset();
  bubble_.reset();
}

void WorkspacesBubbleController::OnCreateDialogClosed(
    views::Widget::ClosedReason) {
  base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(&WorkspacesBubbleController::DestroyCreateDialog,
                     weak_factory_.GetWeakPtr()));
}

void WorkspacesBubbleController::DestroyCreateDialog() {
  create_dialog_widget_.reset();
  create_dialog_.reset();
}
