/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/brave_account/brave_account_service_factory.h"

#include "base/memory/scoped_refptr.h"
#include "brave/components/brave_account/brave_account_service.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/web/public/browser_state.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_account {

// static
BraveAccountService* BraveAccountServiceFactory::GetForBrowserState(
    web::BrowserState* state) {
  return GetFor(state);
}

std::unique_ptr<KeyedService>
BraveAccountServiceFactory::BuildServiceInstanceFor(
    web::BrowserState* state) const {
  return std::make_unique<BraveAccountService>(
      ProfileIOS::FromBrowserState(state)->GetPrefs(),
      state->GetSharedURLLoaderFactory());
}

}  // namespace brave_account
