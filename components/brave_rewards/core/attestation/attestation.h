/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ATTESTATION_ATTESTATION_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ATTESTATION_ATTESTATION_H_

#include <map>
#include <string>

#include "brave/components/brave_rewards/core/ledger.h"

namespace brave_rewards::core {
class LedgerImpl;

namespace attestation {

using StartCallback =
    base::OnceCallback<void(mojom::Result, const std::string&)>;

using ConfirmCallback = base::OnceCallback<void(mojom::Result)>;

class Attestation {
 public:
  explicit Attestation(LedgerImpl* ledger);
  virtual ~Attestation();

  virtual void Start(const std::string& payload, StartCallback callback) = 0;

  virtual void Confirm(const std::string& solution,
                       ConfirmCallback callback) = 0;

 protected:
  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace attestation
}  // namespace brave_rewards::core
#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ATTESTATION_ATTESTATION_H_
