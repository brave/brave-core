/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/api/api.h"

#include <utility>

namespace ledger {
namespace api {

API::API(LedgerImpl& ledger)
    : parameters_(std::make_unique<APIParameters>(ledger)) {
  DCHECK(parameters_);
}

API::~API() = default;

void API::Initialize() {
  parameters_->Initialize();
}

void API::FetchParameters(ledger::GetRewardsParametersCallback callback) {
  parameters_->Fetch(std::move(callback));
}

}  // namespace api
}  // namespace ledger
