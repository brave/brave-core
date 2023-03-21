/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_CREDENTIALS_CREDENTIALS_FACTORY_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_CREDENTIALS_CREDENTIALS_FACTORY_H_

#include <memory>

#include "brave/components/brave_rewards/core/credentials/credentials.h"
#include "brave/components/brave_rewards/core/ledger_callbacks.h"

namespace ledger {
class LedgerImpl;

namespace credential {

class CredentialsFactory {
 public:
  static std::unique_ptr<Credentials> Create(
      LedgerImpl* ledger,
      const mojom::CredsBatchType trigger_type);
};

}  // namespace credential
}  // namespace ledger

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_CREDENTIALS_CREDENTIALS_FACTORY_H_
