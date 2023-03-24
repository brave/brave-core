/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/credentials/credentials_factory.h"
#include "brave/components/brave_rewards/core/credentials/credentials_promotion.h"
#include "brave/components/brave_rewards/core/credentials/credentials_sku.h"
#include "brave/components/brave_rewards/core/ledger_impl.h"

namespace brave_rewards::core {
namespace credential {

std::unique_ptr<Credentials> CredentialsFactory::Create(
    LedgerImpl* ledger,
    const mojom::CredsBatchType trigger_type) {
  DCHECK(ledger);

  switch (trigger_type) {
    case mojom::CredsBatchType::NONE: {
      return nullptr;
    }

    case mojom::CredsBatchType::PROMOTION: {
      return std::make_unique<CredentialsPromotion>(ledger);
    }

    case mojom::CredsBatchType::SKU: {
      return std::make_unique<CredentialsSKU>(ledger);
    }
  }
}

}  // namespace credential
}  // namespace brave_rewards::core
