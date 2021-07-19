/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_SERVICE_OBSERVER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_SERVICE_OBSERVER_H_

#include <string>
#include <vector>

#include "base/observer_list_types.h"

namespace brave_wallet {

class BraveWalletServiceObserver : public base::CheckedObserver {
 public:
  ~BraveWalletServiceObserver() override {}

  virtual void OnShowEthereumPermissionPrompt(
      int32_t tab_id,
      const std::vector<std::string>& accounts,
      const std::string& origin) {}
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_SERVICE_OBSERVER_H_
