/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/brave_wallet_provider_delegate_impl_android.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"

namespace brave_wallet {

BraveWalletProviderDelegateImplAndroid::BraveWalletProviderDelegateImplAndroid(
    content::WebContents* web_contents,
    content::RenderFrameHost* const render_frame_host) {}

void BraveWalletProviderDelegateImplAndroid::RequestEthereumPermissions(
    BraveWalletProviderDelegate::RequestEthereumPermissionsCallback callback) {}

void BraveWalletProviderDelegateImplAndroid::RequestUserApproval(
    const std::string& request_data,
    RequestEthereumChainCallback callback) {}

void BraveWalletProviderDelegateImplAndroid::GetAllowedAccounts(
    BraveWalletProviderDelegate::GetAllowedAccountsCallback callback) {}

}  // namespace brave_wallet
