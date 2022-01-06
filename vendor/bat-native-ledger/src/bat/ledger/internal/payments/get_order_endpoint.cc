/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/payments/get_order_endpoint.h"

#include <string>

#include "bat/ledger/internal/core/environment_config.h"
#include "bat/ledger/internal/payments/post_order_endpoint.h"

namespace ledger {

URLRequest GetOrderEndpoint::MapRequest(const std::string& order_id) {
  std::string host = context().Get<EnvironmentConfig>().payment_service_host();
  return URLRequest::Get("https://" + host + "/v1/orders/" + order_id);
}

absl::optional<PaymentOrder> GetOrderEndpoint::MapResponse(
    const URLResponse& response) {
  // The data returned from this endpoint is identical to the data returned by
  // posting an order. Instead of duplicating the parsing code, defer to
  // `PostOrderEndpoint`.
  return context().Get<PostOrderEndpoint>().MapResponse(response);
}

}  // namespace ledger
