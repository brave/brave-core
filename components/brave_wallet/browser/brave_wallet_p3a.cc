/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/brave_wallet_p3a.h"

#include <utility>

#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/tx_service.h"
#include "brave/components/p3a_utils/feature_usage.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace brave_wallet {

BraveWalletP3A::BraveWalletP3A(BraveWalletService* wallet_service,
                               KeyringService* keyring_service,
                               PrefService* profile_prefs,
                               PrefService* local_state) {}

BraveWalletP3A::BraveWalletP3A() = default;

BraveWalletP3A::~BraveWalletP3A() = default;

void BraveWalletP3A::Bind(
    mojo::PendingReceiver<mojom::BraveWalletP3A> receiver) {
  receivers_.Add(this, std::move(receiver));
}

}  // namespace brave_wallet
