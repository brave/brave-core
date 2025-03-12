/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/ethereum_remote_client_util.h"

#include <memory>

#include "base/environment.h"
#include "brave/browser/ethereum_remote_client/pref_names.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/buildflags.h"
#include "brave/components/constants/brave_services_key.h"
#include "components/prefs/pref_service.h"

namespace extensions {

bool ShouldLoadEthereumRemoteClientExtension(PrefService* prefs) {
  // Only load Crypto Wallets if it is set as the default wallet
  auto default_wallet = brave_wallet::GetDefaultEthereumWallet(prefs);
  const bool is_opted_into_cw = prefs->GetBoolean(kERCOptedIntoCryptoWallets);
  return HasInfuraProjectID() && is_opted_into_cw &&
         default_wallet == brave_wallet::mojom::DefaultWallet::CryptoWallets;
}

bool HasInfuraProjectID() {
  std::string project_id = GetInfuraProjectID();

  if (!project_id.empty()) {
    return true;
  }

  auto env = base::Environment::Create();
  return env->HasVar("BRAVE_INFURA_PROJECT_ID");
}

std::string GetInfuraProjectID() {
  auto env = base::Environment::Create();
  return env->GetVar("BRAVE_INFURA_PROJECT_ID")
      .value_or(BUILDFLAG(BRAVE_INFURA_PROJECT_ID));
}

std::string GetBraveKey() {
  auto env = base::Environment::Create();
  return env->GetVar("BRAVE_SERVICES_KEY")
      .value_or(BUILDFLAG(BRAVE_SERVICES_KEY));
}

}  // namespace extensions
