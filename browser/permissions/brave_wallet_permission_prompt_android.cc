/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/permissions/brave_wallet_permission_prompt_android.h"

#include <utility>

#include "brave/browser/permissions/brave_ethereum_permission_prompt_dialog_controller_android.h"
#include "brave/components/brave_wallet/browser/permission_utils.h"
#include "brave/components/permissions/contexts/brave_wallet_permission_context.h"
#include "components/permissions/android/permission_prompt/permission_prompt_android.h"
#include "content/public/browser/web_contents.h"

BraveWalletPermissionPrompt::BraveWalletPermissionPrompt(
    content::WebContents* web_contents,
    std::unique_ptr<Delegate> delegate)
    : web_contents_(web_contents), delegate_(std::move(delegate)) {
  dialog_controller_ =
      std::make_unique<BraveEthereumPermissionPromptDialogController>(
          this, web_contents_);
  dialog_controller_->ShowDialog();
}

BraveWalletPermissionPrompt::~BraveWalletPermissionPrompt() {}

void BraveWalletPermissionPrompt::ConnectToSite(
    const std::vector<std::string>& accounts) {
  has_interacted_with_dialog_ = true;
  dialog_controller_.reset();
  permissions::BraveWalletPermissionContext::AcceptOrCancel(accounts,
                                                            web_contents_);
}

void BraveWalletPermissionPrompt::CancelConnectToSite() {
  has_interacted_with_dialog_ = true;
  dialog_controller_.reset();
  permissions::BraveWalletPermissionContext::Cancel(web_contents_);
}

void BraveWalletPermissionPrompt::OnDialogDismissed() {
  if (!dialog_controller_) {
    // Dismissed by clicking on dialog buttons.
    return;
  }
  dialog_controller_.reset();
  // If |has_interacted_with_dialog_| is true, |ConnectToSite| or
  // |CancelConnectToSite| should be recorded instead.
  if (!has_interacted_with_dialog_) {
    permissions::BraveWalletPermissionContext::Cancel(web_contents_);
  }
}

void BraveWalletPermissionPrompt::Delegate::Closing() {
  if (!permission_prompt_)
    return;
  permission_prompt_->Closing();
}

BraveWalletPermissionPrompt::Delegate::~Delegate() {
  Closing();
}

BraveWalletPermissionPrompt::Delegate::Delegate() {}

BraveWalletPermissionPrompt::Delegate::Delegate(
    const base::WeakPtr<permissions::PermissionPromptAndroid>&
        permission_prompt)
    : permission_prompt_(permission_prompt) {}
