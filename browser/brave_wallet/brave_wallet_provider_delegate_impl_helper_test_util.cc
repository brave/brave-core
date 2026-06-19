/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/brave_wallet_provider_delegate_impl_helper_test_util.h"

#include <utility>

#include "brave/browser/brave_wallet/brave_wallet_provider_delegate_impl_helper.h"

namespace brave_wallet {

ScopedNewSetupNeededCallbackForTesting::ScopedNewSetupNeededCallbackForTesting(
    base::OnceCallback<void()> callback)
    : callback_(std::move(callback)),
      resetter_(SetNewSetupNeededCallbackForTesting(&callback_)) {}

ScopedNewSetupNeededCallbackForTesting::
    ~ScopedNewSetupNeededCallbackForTesting() = default;

ScopedAccountCreationCallbackForTesting::
    ScopedAccountCreationCallbackForTesting(
        base::OnceCallback<void(std::string_view)> callback)
    : callback_(std::move(callback)),
      resetter_(SetAccountCreationCallbackForTesting(&callback_)) {}

ScopedAccountCreationCallbackForTesting::
    ~ScopedAccountCreationCallbackForTesting() = default;

}  // namespace brave_wallet
