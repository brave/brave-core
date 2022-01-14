/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/brave_wallet_provider_delegate_impl_helper.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "content/public/browser/web_contents.h"

#include "base/notreached.h"

namespace brave_wallet {

void ShowPanel(content::WebContents* web_contents) {
  BraveWalletService* brave_wallet_service =
      brave_wallet::BraveWalletServiceFactory::GetServiceForContext(
          web_contents->GetBrowserContext());
  DCHECK(brave_wallet_service);
  brave_wallet_service->ShowPanel();
}

void ShowWalletOnboarding(content::WebContents* web_contents) {
  BraveWalletService* brave_wallet_service =
      brave_wallet::BraveWalletServiceFactory::GetServiceForContext(
          web_contents->GetBrowserContext());
  DCHECK(brave_wallet_service);
  brave_wallet_service->ShowWalletOnboarding();
}

}  // namespace brave_wallet
