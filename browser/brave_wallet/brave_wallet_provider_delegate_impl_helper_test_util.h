/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_PROVIDER_DELEGATE_IMPL_HELPER_TEST_UTIL_H_
#define BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_PROVIDER_DELEGATE_IMPL_HELPER_TEST_UTIL_H_

#include <string_view>

#include "base/auto_reset.h"
#include "base/functional/callback.h"

namespace brave_wallet {

// These are scope managers to set specific callbacks for certain test-only
// events.

class ScopedNewSetupNeededCallbackForTesting {
 public:
  explicit ScopedNewSetupNeededCallbackForTesting(
      base::OnceCallback<void()> callback);
  ScopedNewSetupNeededCallbackForTesting(
      const ScopedNewSetupNeededCallbackForTesting&) = delete;
  ScopedNewSetupNeededCallbackForTesting& operator=(
      const ScopedNewSetupNeededCallbackForTesting&) = delete;
  ~ScopedNewSetupNeededCallbackForTesting();

 private:
  base::OnceCallback<void()> callback_;
  base::AutoReset<base::OnceCallback<void()>*> resetter_;
};

class ScopedAccountCreationCallbackForTesting {
 public:
  explicit ScopedAccountCreationCallbackForTesting(
      base::OnceCallback<void(std::string_view)> callback);
  ScopedAccountCreationCallbackForTesting(
      const ScopedAccountCreationCallbackForTesting&) = delete;
  ScopedAccountCreationCallbackForTesting& operator=(
      const ScopedAccountCreationCallbackForTesting&) = delete;
  ~ScopedAccountCreationCallbackForTesting();

 private:
  base::OnceCallback<void(std::string_view)> callback_;
  base::AutoReset<base::OnceCallback<void(std::string_view)>*> resetter_;
};

}  // namespace brave_wallet

#endif  // BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_PROVIDER_DELEGATE_IMPL_HELPER_TEST_UTIL_H_
