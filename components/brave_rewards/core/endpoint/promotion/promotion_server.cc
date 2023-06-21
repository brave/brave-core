/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
#include "brave/components/brave_rewards/core/endpoint/promotion/promotion_server.h"

#include "brave/components/brave_rewards/core/ledger_impl.h"

namespace brave_rewards::internal {
namespace endpoint {

PromotionServer::PromotionServer(LedgerImpl& ledger)
    : get_available_(ledger),
      post_creds_(ledger),
      get_signed_creds_(ledger),
      post_clobbered_claims_(ledger),
      post_bat_loss_(ledger),
      post_captcha_(ledger),
      get_captcha_(ledger),
      put_captcha_(ledger),
      post_safetynet_(ledger),
      put_safetynet_(ledger),
      post_devicecheck_(ledger),
      put_devicecheck_(ledger),
      post_suggestions_(ledger),
      post_suggestions_claim_(ledger) {}

PromotionServer::~PromotionServer() = default;

}  // namespace endpoint
}  // namespace brave_rewards::internal
