/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/payments/post_external_transaction_endpoint.h"

#include <string>

#include "bat/ledger/internal/core/environment_config.h"
#include "bat/ledger/internal/core/value_converters.h"

namespace ledger {

namespace {

struct RequestData {
  base::StringPiece transaction_id;
  base::StringPiece kind;

  auto ToValue() const {
    ValueWriter w;
    w.Write("externalTransactionId", transaction_id);
    w.Write("kind", kind);
    return w.Finish();
  }
};

}  // namespace

URLRequest PostExternalTransactionEndpoint::MapRequest(
    const std::string& order_id,
    const std::string& transaction_id,
    ExternalWalletProvider provider) {
  std::string host = context().Get<EnvironmentConfig>().payment_service_host();
  std::string provider_string = StringifyEnum(provider);
  std::string url = "https://" + host + "/v1/orders/" + order_id +
                    "/transactions/" + provider_string;

  RequestData data{.transaction_id = transaction_id, .kind = provider_string};

  auto request = URLRequest::Post(url);
  request.SetBody(data.ToValue());
  return request;
}

bool PostExternalTransactionEndpoint::MapResponse(const URLResponse& response) {
  if (!response.Succeeded()) {
    context().LogError(FROM_HERE) << "HTTP " << response.status_code();
    return false;
  }
  return true;
}

}  // namespace ledger
