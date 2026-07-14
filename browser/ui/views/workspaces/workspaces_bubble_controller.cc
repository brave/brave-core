/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/workspaces/workspaces_bubble_controller.h"

#include <memory>
#include <utility>

#include "base/functional/bind.h"
#include "base/location.h"
#include "base/task/single_thread_task_runner.h"
#include "brave/browser/ui/views/workspaces/save_workspace_dialog.h"
#include "brave/browser/ui/views/workspaces/workspaces_bubble_view.h"
#include "components/constrained_window/constrained_window_views.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

WorkspacesBubbleController::WorkspacesBubbleController() = default;

WorkspacesBubbleController::~WorkspacesBubbleController() {
  if (save_dialog_widget_) {
    // Remove the observer before CloseNow() so OnWidgetDestroyed() does not
    // fire during destruction and try to access members being torn down. Null
    // the member first because CloseNow() synchronously deletes the widget
    // (NATIVE_WIDGET_OWNS_WIDGET), which would leave a dangling raw_ptr.
    auto* widget = save_dialog_widget_.get();
    save_dialog_widget_ = nullptr;
    widget->RemoveObserver(this);
    widget->CloseNow();
    // save_dialog_ is cleaned up by its unique_ptr member destructor.
  }
}

void WorkspacesBubbleController::ShowBubble(views::View* anchor_view,
                                            Profile* profile) {
  if (bubble_widget_) {
    return;
  }

  // Capture what the save dialog needs: the profile and the browser window
  // that hosts the anchor (used to parent the modal).
  profile_ = profile;
  parent_window_ = anchor_view->GetWidget()->GetNativeWindow();

  bubble_ = std::make_unique<WorkspacesBubbleView>(
      anchor_view, profile,
      base::BindRepeating(&WorkspacesBubbleController::ShowSaveDialog,
                          weak_factory_.GetWeakPtr()));

  // The controller keeps ownership of the delegate; CreateBubble uses
  // CLIENT_OWNS_WIDGET and routes close through |on_close|.
  bubble_widget_ = views::BubbleDialogDelegate::CreateBubble(
      bubble_.get(), base::BindOnce(&WorkspacesBubbleController::OnBubbleClosed,
                                    weak_factory_.GetWeakPtr()));
  bubble_widget_->Show();
}

void WorkspacesBubbleController::ShowSaveDialog() {
  if (save_dialog_widget_) {
    return;
  }

  // NATIVE_WIDGET_OWNS_WIDGET (default): when the user dismisses the dialog the
  // normal native-window close sequence runs, re-enabling the browser window.
  // The widget then auto-deletes; OnWidgetDestroyed() clears the raw_ptr and
  // defers deletion of the delegate.
  save_dialog_ = std::make_unique<SaveWorkspaceDialog>(profile_);
  save_dialog_widget_ = constrained_window::CreateBrowserModalDialogViews(
      save_dialog_.get(), parent_window_);
  save_dialog_widget_->AddObserver(this);
  save_dialog_widget_->Show();
}

void WorkspacesBubbleController::OnBubbleClosed(views::Widget::ClosedReason) {
  // Destroy the Widget before its delegate, and defer both past the current
  // stack (the delegate that requested the close may still be unwinding). The
  // two DeleteSoon tasks run in posting order, so the Widget is freed first.
  auto runner = base::SingleThreadTaskRunner::GetCurrentDefault();
  runner->DeleteSoon(FROM_HERE, bubble_widget_.release());
  runner->DeleteSoon(FROM_HERE, bubble_.release());
}

void WorkspacesBubbleController::OnWidgetDestroyed(views::Widget* widget) {
  DCHECK_EQ(widget, save_dialog_widget_);
  widget->RemoveObserver(this);
  save_dialog_widget_ = nullptr;
  // Defer delegate deletion: the widget may still access widget_delegate_ after
  // this callback returns (e.g. in Widget::~Widget()'s cleanup path).
  base::SingleThreadTaskRunner::GetCurrentDefault()->DeleteSoon(
      FROM_HERE, save_dialog_.release());
}
