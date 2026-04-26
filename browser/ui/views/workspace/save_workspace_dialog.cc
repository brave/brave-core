/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/workspace/save_workspace_dialog.h"

#include <memory>
#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "components/constrained_window/constrained_window_views.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/mojom/dialog_button.mojom.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/layout/box_layout.h"

namespace {
constexpr int kDialogWidth = 360;
constexpr int kPadding = 20;
constexpr int kSpacing = 8;
}  // namespace

// static
void SaveWorkspaceDialog::Show(Browser* browser) {
  auto* dialog = new SaveWorkspaceDialog(browser);
  constrained_window::CreateBrowserModalDialogViews(
      dialog, browser->window()->GetNativeWindow())
      ->Show();
}

SaveWorkspaceDialog::SaveWorkspaceDialog(Browser* browser) : browser_(browser) {
  SetButtonLabel(
      ui::mojom::DialogButton::kOk,
      l10n_util::GetStringUTF16(IDS_WORKSPACE_SAVE_DIALOG_SAVE_BUTTON));
  SetButtonLabel(ui::mojom::DialogButton::kCancel,
                 l10n_util::GetStringUTF16(IDS_WORKSPACE_DIALOG_CANCEL_BUTTON));

  SetAcceptCallback(
      base::BindOnce(&SaveWorkspaceDialog::OnAccept, base::Unretained(this)));

  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical, gfx::Insets(kPadding),
      kSpacing));

  AddChildView(std::make_unique<views::Label>(
      l10n_util::GetStringUTF16(IDS_WORKSPACE_SAVE_DIALOG_NAME_LABEL)));

  auto textfield = std::make_unique<views::Textfield>();
  textfield->SetPlaceholderText(
      l10n_util::GetStringUTF16(IDS_WORKSPACE_SAVE_DIALOG_NAME_PLACEHOLDER));
  textfield->SetText(
      l10n_util::GetStringUTF16(IDS_WORKSPACE_SAVE_DIALOG_DEFAULT_NAME));
  textfield->SelectAll(false);
  textfield->SetPreferredSize(gfx::Size(kDialogWidth - kPadding * 2, 40));
  name_field_ = AddChildView(std::move(textfield));
}

SaveWorkspaceDialog::~SaveWorkspaceDialog() = default;

ui::mojom::ModalType SaveWorkspaceDialog::GetModalType() const {
  return ui::mojom::ModalType::kWindow;
}

std::u16string SaveWorkspaceDialog::GetWindowTitle() const {
  return l10n_util::GetStringUTF16(IDS_WORKSPACE_SAVE_DIALOG_TITLE);
}

bool SaveWorkspaceDialog::IsDialogButtonEnabled(
    ui::mojom::DialogButton button) const {
  if (button == ui::mojom::DialogButton::kOk) {
    return name_field_ && !name_field_->GetText().empty();
  }
  return true;
}

void SaveWorkspaceDialog::OnAccept() {
  if (!name_field_) {
    return;
  }
  std::string name = base::UTF16ToUTF8(name_field_->GetText());
  if (name.empty()) {
    return;
  }
  brave::SaveWorkspace(browser_, name);
}

BEGIN_METADATA(SaveWorkspaceDialog)
END_METADATA
