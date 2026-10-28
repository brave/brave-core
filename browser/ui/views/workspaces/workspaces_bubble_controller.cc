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
#include "brave/browser/ui/views/workspaces/create_workspace_dialog.h"
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

  // Capture what the save dialog needs: the browser window the bubble acts on
  // and the native window that hosts the anchor (used to parent the modal).
  browser_ = browser;
  parent_window_ = anchor_view->GetWidget()->GetNativeWindow();

  bubble_ = std::make_unique<WorkspacesBubbleView>(
      anchor_view, browser,
      base::BindRepeating(&WorkspacesBubbleController::ShowCreateDialog,
                          weak_factory_.GetWeakPtr()));

  // The controller keeps ownership of the delegate; CreateBubble uses
  // CLIENT_OWNS_WIDGET and routes close through |on_close|.
  bubble_widget_ = views::BubbleDialogDelegate::CreateBubble(
      bubble_.get(), base::BindOnce(&WorkspacesBubbleController::OnBubbleClosed,
                                    weak_factory_.GetWeakPtr()));
  bubble_widget_->Show();
}

void WorkspacesBubbleController::ShowCreateDialog() {
  if (create_dialog_widget_) {
    return;
  }

  // CreateWorkspaceDialog sets CLIENT_OWNS_WIDGET on itself; the controller
  // keeps ownership of the delegate and owns the returned Widget.
  create_dialog_ = std::make_unique<CreateWorkspaceDialog>(browser_);
  create_dialog_widget_ =
      base::WrapUnique(constrained_window::CreateBrowserModalDialogViews(
          create_dialog_.get(), parent_window_));
  create_dialog_widget_->MakeCloseSynchronous(
      base::BindOnce(&WorkspacesBubbleController::OnCreateDialogClosed,
                     weak_factory_.GetWeakPtr()));
  create_dialog_widget_->Show();
}

void WorkspacesBubbleController::OnBubbleClosed(views::Widget::ClosedReason) {
  // Destroy the Widget before its delegate, and defer both past the current
  // stack (the delegate that requested the close may still be unwinding). The
  // two DeleteSoon tasks run in posting order, so the Widget is freed first.
  auto runner = base::SingleThreadTaskRunner::GetCurrentDefault();
  runner->DeleteSoon(FROM_HERE, bubble_widget_.release());
  runner->DeleteSoon(FROM_HERE, bubble_.release());
}

void WorkspacesBubbleController::OnCreateDialogClosed(
    views::Widget::ClosedReason) {
  auto runner = base::SingleThreadTaskRunner::GetCurrentDefault();
  runner->DeleteSoon(FROM_HERE, create_dialog_widget_.release());
  runner->DeleteSoon(FROM_HERE, create_dialog_.release());
}
