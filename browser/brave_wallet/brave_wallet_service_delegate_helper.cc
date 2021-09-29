/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "brave/browser/brave_wallet/brave_wallet_service_delegate_impl.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service_delegate.h"
#include "build/build_config.h"
#include "content/public/browser/browser_context.h"

namespace brave_wallet {

// static
std::unique_ptr<BraveWalletServiceDelegate> BraveWalletServiceDelegate::Create(
    content::BrowserContext* browser_context) {
#if defined(OS_ANDROID)
  return std::make_unique<BraveWalletServiceDelegate>();
#else
  return std::make_unique<BraveWalletServiceDelegateImpl>(browser_context);
#endif
}

}  // namespace brave_wallet
