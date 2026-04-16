/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/workspace/open_workspace_dialog.h"

#include <memory>
#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/thread_pool.h"
#include "base/time/time.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/browser/workspace/brave_workspace_service.h"
#include "brave/browser/workspace/brave_workspace_service_factory.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "components/constrained_window/constrained_window_views.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/mojom/dialog_button.mojom.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/layout/box_layout.h"

namespace {

constexpr int kDialogWidth = 420;
constexpr int kListHeight = 240;
constexpr int kPadding = 16;
constexpr int kRowSpacing = 2;

std::u16string FormatDate(base::Time time) {
  base::Time::Exploded exploded;
  time.LocalExplode(&exploded);
  return base::UTF8ToUTF16(absl::StrFormat(
      "%04d-%02d-%02d", exploded.year, exploded.month, exploded.day_of_month));
}

}  // namespace

// static
void OpenWorkspaceDialog::Show(Browser* browser) {
  auto* service =
      BraveWorkspaceServiceFactory::GetForProfile(browser->profile());
  if (!service) {
    return;
  }
  base::FilePath dir = service->GetWorkspacesDir();
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
       base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
      base::BindOnce(&BraveWorkspaceService::ListWorkspacesInDir,
                     std::move(dir)),
      base::BindOnce(
          [](Browser* browser, std::vector<WorkspaceInfo> workspaces) {
            auto* dialog =
                new OpenWorkspaceDialog(browser, std::move(workspaces));
            constrained_window::CreateBrowserModalDialogViews(
                dialog, browser->window()->GetNativeWindow())
                ->Show();
          },
          base::Unretained(browser)));
}

OpenWorkspaceDialog::OpenWorkspaceDialog(Browser* browser,
                                         std::vector<WorkspaceInfo> workspaces)
    : browser_(browser), workspaces_(std::move(workspaces)) {
  SetButtonLabel(
      ui::mojom::DialogButton::kOk,
      l10n_util::GetStringUTF16(IDS_WORKSPACE_OPEN_DIALOG_OPEN_BUTTON));
  SetButtonLabel(ui::mojom::DialogButton::kCancel,
                 l10n_util::GetStringUTF16(IDS_WORKSPACE_DIALOG_CANCEL_BUTTON));

  SetAcceptCallback(
      base::BindOnce(&OpenWorkspaceDialog::OnAccept, base::Unretained(this)));

  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical, gfx::Insets(kPadding),
      kRowSpacing));

  if (workspaces_.empty()) {
    AddChildView(std::make_unique<views::Label>(
        l10n_util::GetStringUTF16(IDS_WORKSPACE_OPEN_DIALOG_EMPTY_MESSAGE)));
  } else {
    // Scrollable list of workspaces.
    auto scroll_view = std::make_unique<views::ScrollView>();
    scroll_view->ClipHeightTo(kListHeight, kListHeight);
    scroll_view->SetPreferredSize(gfx::Size(kDialogWidth - kPadding * 2, 40));

    auto list = std::make_unique<views::View>();
    list->SetLayoutManager(std::make_unique<views::BoxLayout>(
        views::BoxLayout::Orientation::kVertical, gfx::Insets(), kRowSpacing));
    list_container_ = list.get();

    scroll_view->SetContents(std::move(list));
    AddChildView(std::move(scroll_view));

    BuildWorkspaceList();

    // Delete button placed below the list.
    auto delete_btn = std::make_unique<views::LabelButton>(
        base::BindRepeating(&OpenWorkspaceDialog::OnDeleteClicked,
                            base::Unretained(this)),
        l10n_util::GetStringUTF16(IDS_WORKSPACE_OPEN_DIALOG_DELETE_BUTTON));
    delete_button_ = AddChildView(std::move(delete_btn));
    delete_button_->SetEnabled(false);
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

    // Each workspace is rendered as a full-width label button showing the
    // workspace name and creation date.
    auto row_button = std::make_unique<views::LabelButton>(
        base::BindRepeating(&OpenWorkspaceDialog::OnWorkspaceSelected,
                            base::Unretained(this), static_cast<int>(i)),
        base::UTF8ToUTF16(info.name) + u"  " + FormatDate(info.created_at));
    row_button->SetHorizontalAlignment(gfx::ALIGN_LEFT);
    row_button->SetPreferredSize(
        gfx::Size(kDialogWidth - kPadding * 2 - kRowSpacing * 2, 36));

    list_container_->AddChildView(std::move(row_button));
  }
}

void OpenWorkspaceDialog::OnWorkspaceSelected(int index) {
  selected_index_ = index;
  if (delete_button_) {
    delete_button_->SetEnabled(true);
  }
  // Update the OK button state.
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
  brave::RestoreWorkspace(browser_, workspaces_[selected_index_].name);
}

void OpenWorkspaceDialog::OnDeleteClicked() {
  if (selected_index_ < 0 ||
      selected_index_ >= static_cast<int>(workspaces_.size())) {
    return;
  }
  auto* service =
      BraveWorkspaceServiceFactory::GetForProfile(browser_->profile());
  if (!service) {
    return;
  }

  // Disable immediately to prevent double-clicks while I/O is in flight.
  if (delete_button_) {
    delete_button_->SetEnabled(false);
  }

  int index = selected_index_;
  std::string name = workspaces_[index].name;
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
       base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
      base::BindOnce(&BraveWorkspaceService::DeleteWorkspace,
                     base::Unretained(service), std::move(name)),
      base::BindOnce(&OpenWorkspaceDialog::OnDeleteCompleted,
                     weak_factory_.GetWeakPtr(), index));
}

void OpenWorkspaceDialog::OnDeleteCompleted(int index, bool success) {
  if (!success || index < 0 ||
      index >= static_cast<int>(workspaces_.size())) {
    return;
  }
  workspaces_.erase(workspaces_.begin() + index);
  selected_index_ = -1;
  BuildWorkspaceList();
  DialogModelChanged();
}

BEGIN_METADATA(OpenWorkspaceDialog)
END_METADATA
