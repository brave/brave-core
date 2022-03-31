/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>
#include <utility>

#include "base/time/time.h"
#include "bat/ledger/internal/api/api_parameters.h"
#include "bat/ledger/internal/common/time_util.h"
#include "bat/ledger/internal/ledger_impl.h"

using std::placeholders::_1;
using std::placeholders::_2;

namespace ledger {
namespace api {

APIParameters::APIParameters(LedgerImpl* ledger) :
    ledger_(ledger),
    api_server_(std::make_unique<endpoint::APIServer>(ledger)) {
  DCHECK(ledger_ && api_server_);
}

APIParameters::~APIParameters() = default;

void APIParameters::Initialize() {
  Fetch([](type::RewardsParametersPtr) {});
}

void APIParameters::Fetch(ledger::GetRewardsParametersCallback callback) {
  bool first_request = callbacks_.empty();
  callbacks_.push_back(callback);
  if (!first_request) {
    BLOG(1, "API parameters fetch in progress");
    return;
  }

  refresh_timer_.Stop();

  auto url_callback = std::bind(&APIParameters::OnFetch,
      this,
      _1,
      _2);

  api_server_->get_parameters()->Request(url_callback);
}

void APIParameters::OnFetch(
    const type::Result result,
    const type::RewardsParameters& parameters) {
  if (result == type::Result::RETRY_SHORT) {
    RunCallbacks();
    SetRefreshTimer(base::Seconds(90));
    return;
  }

  if (result != type::Result::LEDGER_OK) {
    BLOG(1, "Couldn't parse response");
    RunCallbacks();
    SetRefreshTimer(base::Minutes(10));
    return;
  }

  ledger_->state()->SetRewardsParameters(parameters);
  RunCallbacks();
  SetRefreshTimer(base::Minutes(10), base::Hours(3));
}

void APIParameters::RunCallbacks() {
  // Execute callbacks with the current parameters stored in state.
  // If the last fetch failed, callbacks will be run with the last
  // successfully fetched parameters or a default set of parameters.
  auto parameters = ledger_->state()->GetRewardsParameters();
  DCHECK(parameters);

  auto callbacks = std::move(callbacks_);
  for (auto& callback : callbacks) {
    callback(parameters->Clone());
  }
}

void APIParameters::SetRefreshTimer(
    base::TimeDelta delay,
    base::TimeDelta base_delay) {
  if (refresh_timer_.IsRunning()) {
    BLOG(1, "Params timer in progress");
    return;
  }

  base::TimeDelta start_in =
      base_delay + util::GetRandomizedDelay(delay);

  BLOG(1, "Params timer set for " << start_in);

  refresh_timer_.Start(FROM_HERE, start_in,
      base::BindOnce(&APIParameters::Fetch,
          base::Unretained(this),
          [](type::RewardsParametersPtr) {}));
}

}  // namespace api
}  // namespace ledger
