/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/permissions/brave_wallet_permission_prompt_android.h"

#include <utility>

#include "brave/browser/permissions/brave_dapp_permission_prompt_dialog_controller_android.h"
#include "brave/components/brave_wallet/browser/permission_utils.h"
#include "brave/components/permissions/contexts/brave_wallet_permission_context.h"
#include "components/permissions/android/permission_prompt/permission_prompt_android.h"
#include "components/permissions/permission_request.h"
#include "content/public/browser/web_contents.h"
#include "url/origin.h"

namespace {

// The request origin has the account address appended to its host. Parse it
// out to get the origin that receives the grant, which the prompt shows. For a
// request from an iframe it differs from the tab's origin.
url::Origin GetRequestingOrigin(const permissions::PermissionRequest& request) {
  const url::Origin origin = url::Origin::Create(request.requesting_origin());
  url::Origin requesting_origin;
  if (brave_wallet::ParseRequestingOriginFromSubRequest(
          request.request_type(), origin, &requesting_origin,
          /*account=*/nullptr)) {
    return requesting_origin;
  }
  return origin;
}

}  // namespace

BraveWalletPermissionPrompt::BraveWalletPermissionPrompt(
    content::WebContents* web_contents,
    std::unique_ptr<Delegate> delegate,
    brave_wallet::mojom::CoinType coin_type,
    const permissions::PermissionRequest& request)
    : web_contents_(web_contents), delegate_(std::move(delegate)) {
  dialog_controller_ =
      std::make_unique<BraveDappPermissionPromptDialogController>(
          this, web_contents_, coin_type, GetRequestingOrigin(request));
  dialog_controller_->ShowDialog();
}

BraveWalletPermissionPrompt::~BraveWalletPermissionPrompt() {}

void BraveWalletPermissionPrompt::ConnectToSite(
    const std::vector<std::string>& accounts,
    int permission_lifetime_option) {
  has_interacted_with_dialog_ = true;
  dialog_controller_.reset();
  // TODO(SergeyZhukovsky): Use the real option that the user chooses, using
  // `kForever` here is for landing new API changes separately.
  permissions::BraveWalletPermissionContext::AcceptOrCancel(
      accounts, brave_wallet::mojom::PermissionLifetimeOption::kForever,
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
  if (!permission_prompt_) {
    return;
  }
  permission_prompt_->Dismiss(/*prompt_options=*/std::monostate());
}

BraveWalletPermissionPrompt::Delegate::~Delegate() {
  Closing();
}

BraveWalletPermissionPrompt::Delegate::Delegate() {}

BraveWalletPermissionPrompt::Delegate::Delegate(
    const base::WeakPtr<permissions::PermissionPromptAndroid>&
        permission_prompt)
    : permission_prompt_(permission_prompt) {}
