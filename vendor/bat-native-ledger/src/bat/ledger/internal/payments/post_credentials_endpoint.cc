/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/payments/post_credentials_endpoint.h"

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "bat/ledger/internal/core/environment_config.h"
#include "bat/ledger/internal/core/value_converters.h"

namespace ledger {

namespace {

struct RequestData {
  base::StringPiece item_id;
  PaymentCredentialType type;
  std::vector<base::StringPiece> blinded_creds;

  auto ToValue() const {
    ValueWriter w;
    w.Write("itemId", item_id);
    w.Write("type", type);
    w.Write("blindedCreds", blinded_creds);
    return w.Finish();
  }
};

}  // namespace

URLRequest PostCredentialsEndpoint::MapRequest(
    const std::string& order_id,
    const std::string& item_id,
    PaymentCredentialType type,
    const std::vector<std::string>& blinded_tokens) {
  RequestData data{.item_id = item_id, .type = type};

  for (auto& token : blinded_tokens) {
    data.blinded_creds.push_back(token);
  }

  std::string host = context().Get<EnvironmentConfig>().payment_service_host();
  auto request = URLRequest::Post("https://" + host + "/v1/orders/" + order_id +
                                  "/credentials");
  request.SetBody(data.ToValue());
  return request;
}

bool PostCredentialsEndpoint::MapResponse(const URLResponse& response) {
  if (!response.Succeeded()) {
    context().LogError(FROM_HERE) << "HTTP " << response.status_code();
    return false;
  }

  return true;
}

}  // namespace ledger
