/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/workspaces/workspace_dialog.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/browser/workspaces/workspace_metadata.h"
#include "brave/browser/workspaces/workspace_service.h"
#include "brave/browser/workspaces/workspace_service_factory.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/mojom/dialog_button.mojom.h"
#include "ui/base/mojom/ui_base_types.mojom-shared.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/widget/widget.h"

namespace {
constexpr int kDialogWidth = 360;
constexpr int kPadding = 20;
constexpr int kSpacing = 8;
}  // namespace

WorkspaceDialog::WorkspaceDialog(BrowserWindowInterface* browser,
                                 std::optional<base::Uuid> workspace_to_rename)
    : browser_(browser), workspace_to_rename_(std::move(workspace_to_rename)) {
  const bool renaming = workspace_to_rename_.has_value();
  // The caller (WorkspacesBubbleController) owns the resulting Widget.
  SetOwnershipOfNewWidget(views::Widget::InitParams::CLIENT_OWNS_WIDGET);
  SetModalType(ui::mojom::ModalType::kWindow);
  SetTitle(l10n_util::GetStringUTF16(
      renaming ? IDS_WORKSPACE_SETTINGS_DIALOG_TITLE
               : IDS_WORKSPACE_CREATE_DIALOG_TITLE));
  SetButtonLabel(
      ui::mojom::DialogButton::kOk,
      l10n_util::GetStringUTF16(
          renaming ? IDS_WORKSPACE_SETTINGS_DIALOG_SAVE_BUTTON
                   : IDS_WORKSPACE_CREATE_DIALOG_CREATE_BUTTON));
  SetButtonLabel(ui::mojom::DialogButton::kCancel,
                 l10n_util::GetStringUTF16(IDS_WORKSPACE_DIALOG_CANCEL_BUTTON));

  SetAcceptCallback(
      base::BindOnce(&WorkspaceDialog::OnAccept, base::Unretained(this)));

  auto contents = std::make_unique<views::View>();
  contents->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical, gfx::Insets(kPadding),
      kSpacing));

  contents->AddChildView(std::make_unique<views::Label>(
      l10n_util::GetStringUTF16(IDS_WORKSPACE_CREATE_DIALOG_NAME_LABEL)));

  // Renaming starts from the name the workspace already has.
  auto* service = WorkspaceServiceFactory::GetForProfile(
      browser_->GetProfile());
  CHECK(service);
  std::optional<WorkspaceMetadata> existing =
      renaming ? service->GetWorkspace(*workspace_to_rename_) : std::nullopt;

  // Cache the initial name so OnAccept() does not need to read it from the
  // Textfield (which is destroyed before the delegate when the dialog closes).
  workspace_name_ =
      existing ? existing->name
               : base::UTF16ToUTF8(l10n_util::GetStringUTF16(
                     IDS_WORKSPACE_CREATE_DIALOG_DEFAULT_NAME));

  auto textfield = std::make_unique<views::Textfield>();
  textfield->SetPlaceholderText(
      l10n_util::GetStringUTF16(IDS_WORKSPACE_CREATE_DIALOG_NAME_PLACEHOLDER));
  textfield->SetText(base::UTF8ToUTF16(workspace_name_));
  textfield->SelectAll(false);
  textfield->SetPreferredSize(gfx::Size(kDialogWidth - kPadding * 2, 40));
  textfield->SetController(this);
  contents->AddChildView(std::move(textfield));

  // Reflect the initial (pre-filled, non-empty) name in the OK button state.
  SetButtonEnabled(ui::mojom::DialogButton::kOk, !workspace_name_.empty());

  SetContentsView(std::move(contents));
}

WorkspaceDialog::~WorkspaceDialog() = default;

void WorkspaceDialog::OnAccept() {
  // The OK button is disabled while the name is empty, so this is non-empty.
  auto* service =
      WorkspaceServiceFactory::GetForProfile(browser_->GetProfile());
  CHECK(service);
  if (workspace_to_rename_) {
    service->RenameWorkspace(*workspace_to_rename_, workspace_name_);
    return;
  }
  service->CreateWorkspaceInWindow(workspace_name_, browser_);
}

void WorkspaceDialog::ContentsChanged(
    views::Textfield* sender,
    const std::u16string& new_contents) {
  workspace_name_ = base::UTF16ToUTF8(new_contents);
  SetButtonEnabled(ui::mojom::DialogButton::kOk, !new_contents.empty());
}
