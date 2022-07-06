/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_PROVIDER_DELEGATE_IMPL_HELPER_H_
#define BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_PROVIDER_DELEGATE_IMPL_HELPER_H_

#include "base/callback.h"
#include "build/build_config.h"

namespace content {
class WebContents;
}

// UI helper functions which are platform-dependent.
namespace brave_wallet {

// Show wallet panel, which handles permission UI, sign message confirmation,
// ...etc.
void ShowPanel(content::WebContents* web_contents);

// Show wallet onboarding page.
void ShowWalletOnboarding(content::WebContents* web_contents);

// Triggers when any kind interaction from a DApp is detected
void WalletInteractionDetected(content::WebContents* web_contents);

// Check are web3 notifications allowed or not. Used on Android to
// show or not a permissions prompt dialog
bool IsWeb3NotificationAllowed();

#if !BUILDFLAG(IS_ANDROID)
void SetCallbackForNewSetupNeededForTesting(base::OnceCallback<void()>);
#endif

}  // namespace brave_wallet

#endif  // BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_PROVIDER_DELEGATE_IMPL_HELPER_H_
