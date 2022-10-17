/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_API_API_PARAMETERS_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_API_API_PARAMETERS_H_

#include <memory>
#include <vector>

#include "base/timer/timer.h"
#include "bat/ledger/internal/endpoints/get_parameters/get_parameters.h"
#include "bat/ledger/ledger.h"

namespace ledger {
class LedgerImpl;

namespace api {

class APIParameters {
 public:
  explicit APIParameters(LedgerImpl* ledger);
  ~APIParameters();

  void Initialize();

  void Fetch(ledger::GetRewardsParametersCallback callback);

 private:
  void OnFetch(endpoints::GetParameters::Result&&);

  void RunCallbacks();

  void SetRefreshTimer(
      base::TimeDelta delay,
      base::TimeDelta base_delay = base::TimeDelta());

  LedgerImpl* ledger_;  // NOT OWNED
  base::OneShotTimer refresh_timer_;
  std::vector<ledger::GetRewardsParametersCallback> callbacks_;
};

}  // namespace api
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_API_API_PARAMETERS_H_
