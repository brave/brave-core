/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/dapp/wallet_installation_permission_request.h"

#include "brave/grit/brave_generated_resources.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/gurl.h"

WalletInstallationPermissionRequest::WalletInstallationPermissionRequest(
    content::WebContents* web_contents)
    : web_contents_(web_contents) {
}

WalletInstallationPermissionRequest::~WalletInstallationPermissionRequest() {
}

PermissionRequest::IconId
WalletInstallationPermissionRequest::GetIconId() const {
  return kExtensionIcon;
}

base::string16
WalletInstallationPermissionRequest::GetMessageTextFragment() const {
  return l10n_util::GetStringUTF16(
      IDS_WALLET_PERMISSION_REQUEST_TEXT_FRAGMENT);
}

GURL WalletInstallationPermissionRequest::GetOrigin() const {
  return web_contents_->GetVisibleURL();
}

void WalletInstallationPermissionRequest::PermissionGranted() {
  // TODO(shong): Install wallet.
  NOTIMPLEMENTED();
}

void WalletInstallationPermissionRequest::PermissionDenied() {
  // Do nothing.
}

void WalletInstallationPermissionRequest::Cancelled() {
  // Do nothing.
}

void WalletInstallationPermissionRequest::RequestFinished() {
  delete this;
}

PermissionRequestType
WalletInstallationPermissionRequest::GetPermissionRequestType() const {
  return PermissionRequestType::PERMISSION_WALLET;
}
