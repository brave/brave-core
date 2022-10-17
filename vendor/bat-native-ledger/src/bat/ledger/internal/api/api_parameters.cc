/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/api/api_parameters.h"

#include <string>
#include <utility>

#include "base/time/time.h"
#include "bat/ledger/internal/common/time_util.h"
#include "bat/ledger/internal/endpoints/request_for.h"
#include "bat/ledger/internal/ledger_impl.h"

using ledger::endpoints::GetParameters;
using ledger::endpoints::RequestFor;

namespace ledger {
namespace api {

APIParameters::APIParameters(LedgerImpl* ledger) : ledger_(ledger) {
  DCHECK(ledger_);
}

APIParameters::~APIParameters() = default;

void APIParameters::Initialize() {
  Fetch(base::DoNothing());
}

void APIParameters::Fetch(ledger::GetRewardsParametersCallback callback) {
  bool first_request = callbacks_.empty();
  callbacks_.push_back(std::move(callback));
  if (!first_request) {
    BLOG(1, "API parameters fetch in progress");
    return;
  }

  refresh_timer_.Stop();

  RequestFor<GetParameters>(ledger_).Send(
      base::BindOnce(&APIParameters::OnFetch, base::Unretained(this)));
}

void APIParameters::OnFetch(GetParameters::Result&& result) {
  if (result.has_value()) {
    DCHECK(result.value());
    ledger_->state()->SetRewardsParameters(*result.value());
    RunCallbacks();
    return SetRefreshTimer(base::Minutes(10), base::Hours(3));
  }

  RunCallbacks();

  switch (result.error()) {
    case GetParameters::Error::kFailedToGetParameters:
      SetRefreshTimer(base::Seconds(90));
      break;
    default:
      SetRefreshTimer(base::Minutes(10));
      break;
  }
}

void APIParameters::RunCallbacks() {
  // Execute callbacks with the current parameters stored in state.
  // If the last fetch failed, callbacks will be run with the last
  // successfully fetched parameters or a default set of parameters.
  auto parameters = ledger_->state()->GetRewardsParameters();
  DCHECK(parameters);

  auto callbacks = std::move(callbacks_);
  for (auto& callback : callbacks) {
    std::move(callback).Run(parameters->Clone());
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

  refresh_timer_.Start(
      FROM_HERE, start_in,
      base::BindOnce(&APIParameters::Fetch, base::Unretained(this),
                     base::DoNothing()));
}

}  // namespace api
}  // namespace ledger
