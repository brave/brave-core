/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_API_API_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_API_API_H_

#include <memory>

#include "brave/components/brave_rewards/core/api/api_parameters.h"
#include "brave/components/brave_rewards/core/ledger.h"
#include "brave/components/brave_rewards/core/mojom_structs.h"

namespace ledger {
class LedgerImpl;

namespace api {

class API {
 public:
  explicit API(LedgerImpl* ledger);
  ~API();

  void Initialize();

  void FetchParameters(ledger::GetRewardsParametersCallback callback);

 private:
  std::unique_ptr<APIParameters> parameters_;
};

}  // namespace api
}  // namespace ledger

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_API_API_H_
