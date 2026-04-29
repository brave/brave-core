/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/workspaces/open_workspace_dialog.h"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/thread_pool.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/browser/ui/views/workspaces/workspace_row_view.h"
#include "brave/browser/workspaces/workspace_service.h"
#include "brave/browser/workspaces/workspace_service_factory.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/simple_message_box.h"
#include "components/constrained_window/constrained_window_views.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/mojom/dialog_button.mojom.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/view_utils.h"

namespace {

constexpr int kDialogWidth = 420;
constexpr int kListHeight = 240;
constexpr int kPadding = 16;
constexpr int kRowSpacing = 2;
constexpr int kRowHeight = 36;

}  // namespace

// static
void OpenWorkspaceDialog::Show(Browser* browser) {
  auto* service = WorkspaceServiceFactory::GetForProfile(browser->profile());
  if (!service) {
    return;
  }
  auto* dialog = new OpenWorkspaceDialog(browser, service->ListWorkspaces());
  constrained_window::CreateBrowserModalDialogViews(
      dialog, browser->window()->GetNativeWindow())
      ->Show();
}

OpenWorkspaceDialog::OpenWorkspaceDialog(
    Browser* browser,
    std::vector<WorkspaceMetadata> workspaces)
    : browser_(browser), workspaces_(std::move(workspaces)) {
  SetButtonLabel(
      ui::mojom::DialogButton::kOk,
      l10n_util::GetStringUTF16(IDS_WORKSPACE_OPEN_DIALOG_OPEN_BUTTON));
  SetButtonLabel(ui::mojom::DialogButton::kCancel,
                 l10n_util::GetStringUTF16(IDS_WORKSPACE_DIALOG_CANCEL_BUTTON));

  SetAcceptCallback(base::BindOnce(&OpenWorkspaceDialog::OnAccept,
                                   weak_factory_.GetWeakPtr()));

  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical, gfx::Insets(kPadding),
      kRowSpacing));

  if (workspaces_.empty()) {
    AddChildView(std::make_unique<views::Label>(
        l10n_util::GetStringUTF16(IDS_WORKSPACE_OPEN_DIALOG_EMPTY_MESSAGE)));
  } else {
    // Scrollable list of workspaces.  Height is set by BuildWorkspaceList so
    // it fits the content exactly, up to kListHeight.
    auto scroll_view = std::make_unique<views::ScrollView>();

    auto list = std::make_unique<views::View>();
    list->SetLayoutManager(std::make_unique<views::BoxLayout>(
        views::BoxLayout::Orientation::kVertical, gfx::Insets(), kRowSpacing));
    list_container_ = list.get();

    scroll_view->SetContents(std::move(list));
    scroll_view_ = AddChildView(std::move(scroll_view));

    BuildWorkspaceList();
  }
}

OpenWorkspaceDialog::~OpenWorkspaceDialog() = default;

void OpenWorkspaceDialog::BuildWorkspaceList() {
  if (!list_container_) {
    return;
  }
  list_container_->RemoveAllChildViews();

  for (size_t i = 0; i < workspaces_.size(); ++i) {
    const auto& info = workspaces_[i];
    // Each row is a horizontal container: name button (flex) + delete button.
    auto row = std::make_unique<WorkspaceRowView>(
        kDialogWidth, kPadding, kRowHeight, info,
        base::BindRepeating(&OpenWorkspaceDialog::OnWorkspaceSelected,
                            weak_factory_.GetWeakPtr(), static_cast<int>(i)),
        base::BindRepeating(&OpenWorkspaceDialog::OnDeleteClicked,
                            weak_factory_.GetWeakPtr(), static_cast<int>(i)));
    list_container_->AddChildView(std::move(row));
  }

  // Resize the scroll view to fit the current row count exactly, up to
  // kListHeight (at which point the list becomes scrollable).
  if (scroll_view_) {
    int n = static_cast<int>(workspaces_.size());
    int desired = n > 0 ? n * kRowHeight + (n - 1) * kRowSpacing : 0;
    int height = std::min(desired, kListHeight);
    scroll_view_->ClipHeightTo(height, height);
    InvalidateLayout();
  }

  UpdateRowSelection();
}

void OpenWorkspaceDialog::UpdateRowSelection() {
  if (!list_container_) {
    return;
  }
  const auto& children = list_container_->children();
  for (size_t i = 0; i < children.size(); ++i) {
    views::AsViewClass<WorkspaceRowView>(children[i].get())
        ->SetSelected(static_cast<int>(i) == selected_index_);
  }
}

void OpenWorkspaceDialog::OnWorkspaceSelected(int index) {
  selected_index_ = index;
  UpdateRowSelection();
  DialogModelChanged();
}

ui::mojom::ModalType OpenWorkspaceDialog::GetModalType() const {
  return ui::mojom::ModalType::kWindow;
}

std::u16string OpenWorkspaceDialog::GetWindowTitle() const {
  return l10n_util::GetStringUTF16(IDS_WORKSPACE_OPEN_DIALOG_TITLE);
}

bool OpenWorkspaceDialog::IsDialogButtonEnabled(
    ui::mojom::DialogButton button) const {
  if (button == ui::mojom::DialogButton::kOk) {
    return selected_index_ >= 0 &&
           selected_index_ < static_cast<int>(workspaces_.size());
  }
  return true;
}

void OpenWorkspaceDialog::OnAccept() {
  if (selected_index_ < 0 ||
      selected_index_ >= static_cast<int>(workspaces_.size())) {
    return;
  }

  auto* service = WorkspaceServiceFactory::GetForProfile(browser_->profile());
  if (!service) {
    return;
  }

  service->RestoreWorkspace(workspaces_[selected_index_].name);
}

void OpenWorkspaceDialog::OnDeleteClicked(int index) {
  if (index < 0 || index >= static_cast<int>(workspaces_.size())) {
    return;
  }
  std::string name = workspaces_[index].name;
  chrome::ShowQuestionMessageBoxAsync(
      browser_->window()->GetNativeWindow(),
      l10n_util::GetStringUTF16(IDS_WORKSPACE_DELETE_CONFIRM_TITLE),
      l10n_util::GetStringFUTF16(IDS_WORKSPACE_DELETE_CONFIRM_MESSAGE,
                                 base::UTF8ToUTF16(workspaces_[index].name)),
      base::BindOnce(&OpenWorkspaceDialog::OnDeleteConfirmed,
                     weak_factory_.GetWeakPtr(), std::move(name)));
}

void OpenWorkspaceDialog::OnDeleteConfirmed(std::string name,
                                            chrome::MessageBoxResult result) {
  if (result != chrome::MESSAGE_BOX_RESULT_YES) {
    return;
  }
  auto* service = WorkspaceServiceFactory::GetForProfile(browser_->profile());
  if (!service) {
    return;
  }

  // TODO: fix me!
  // base::ThreadPool::PostTaskAndReplyWithResult(
  //     FROM_HERE,
  //     {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
  //      base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
  //     base::BindOnce(&WorkspaceService::DeleteWorkspace, name),
  //     base::BindOnce(&OpenWorkspaceDialog::OnDeleteCompleted,
  //                    weak_factory_.GetWeakPtr(), std::move(name)));
}

void OpenWorkspaceDialog::OnDeleteCompleted(std::string name, bool success) {
  if (!success) {
    return;
  }
  auto* service = WorkspaceServiceFactory::GetForProfile(browser_->profile());
  if (service) {
    service->RemoveWorkspaceMetadata(name);
  }
  auto it = std::find_if(
      workspaces_.begin(), workspaces_.end(),
      [&name](const WorkspaceMetadata& w) { return w.name == name; });
  if (it == workspaces_.end()) {
    return;
  }
  if (selected_index_ >= 0 &&
      static_cast<size_t>(selected_index_) ==
          static_cast<size_t>(it - workspaces_.begin())) {
    selected_index_ = -1;
  }
  workspaces_.erase(it);
  BuildWorkspaceList();
  DialogModelChanged();
}

BEGIN_METADATA(OpenWorkspaceDialog)
END_METADATA
