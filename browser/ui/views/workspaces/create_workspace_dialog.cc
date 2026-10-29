/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/workspaces/create_workspace_dialog.h"

#include <memory>
#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "base/strings/utf_string_conversions.h"
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

CreateWorkspaceDialog::CreateWorkspaceDialog(BrowserWindowInterface* browser)
    : browser_(browser) {
  // The caller (WorkspacesBubbleController) owns the resulting Widget.
  SetOwnershipOfNewWidget(views::Widget::InitParams::CLIENT_OWNS_WIDGET);
  SetModalType(ui::mojom::ModalType::kWindow);
  SetTitle(l10n_util::GetStringUTF16(IDS_WORKSPACE_CREATE_DIALOG_TITLE));
  SetButtonLabel(
      ui::mojom::DialogButton::kOk,
      l10n_util::GetStringUTF16(IDS_WORKSPACE_CREATE_DIALOG_CREATE_BUTTON));
  SetButtonLabel(ui::mojom::DialogButton::kCancel,
                 l10n_util::GetStringUTF16(IDS_WORKSPACE_DIALOG_CANCEL_BUTTON));

  SetAcceptCallback(
      base::BindOnce(&CreateWorkspaceDialog::OnAccept, base::Unretained(this)));

  auto contents = std::make_unique<views::View>();
  contents->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical, gfx::Insets(kPadding),
      kSpacing));

  contents->AddChildView(std::make_unique<views::Label>(
      l10n_util::GetStringUTF16(IDS_WORKSPACE_CREATE_DIALOG_NAME_LABEL)));

  // Cache the initial name so OnAccept() does not need to read it from the
  // Textfield (which is destroyed before the delegate when the dialog closes).
  workspace_name_ = base::UTF16ToUTF8(
      l10n_util::GetStringUTF16(IDS_WORKSPACE_CREATE_DIALOG_DEFAULT_NAME));

  auto textfield = std::make_unique<views::Textfield>();
  textfield->SetPlaceholderText(
      l10n_util::GetStringUTF16(IDS_WORKSPACE_CREATE_DIALOG_NAME_PLACEHOLDER));
  textfield->SetText(
      l10n_util::GetStringUTF16(IDS_WORKSPACE_CREATE_DIALOG_DEFAULT_NAME));
  textfield->SelectAll(false);
  textfield->SetPreferredSize(gfx::Size(kDialogWidth - kPadding * 2, 40));
  textfield->SetController(this);
  contents->AddChildView(std::move(textfield));

  // Reflect the initial (pre-filled, non-empty) name in the OK button state.
  SetButtonEnabled(ui::mojom::DialogButton::kOk, !workspace_name_.empty());

  SetContentsView(std::move(contents));
}

CreateWorkspaceDialog::~CreateWorkspaceDialog() = default;

void CreateWorkspaceDialog::OnAccept() {
  // The OK button is disabled while the name is empty, so this is non-empty.
  auto* service =
      WorkspaceServiceFactory::GetForProfile(browser_->GetProfile());
  CHECK(service);
  service->CreateWorkspaceInWindow(workspace_name_, browser_);
}

void CreateWorkspaceDialog::ContentsChanged(
    views::Textfield* sender,
    const std::u16string& new_contents) {
  workspace_name_ = base::UTF16ToUTF8(new_contents);
  SetButtonEnabled(ui::mojom::DialogButton::kOk, !new_contents.empty());
}
