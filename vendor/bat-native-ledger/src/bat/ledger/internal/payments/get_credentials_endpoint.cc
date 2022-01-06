/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/payments/get_credentials_endpoint.h"

#include <string>
#include <utility>
#include <vector>

#include "bat/ledger/internal/core/environment_config.h"
#include "bat/ledger/internal/core/value_converters.h"
#include "net/http/http_status_code.h"

namespace ledger {

namespace {

struct ResponseData {
  std::vector<std::string> signed_creds;
  std::string batch_proof;
  std::string public_key;

  static auto FromValue(const base::Value& value) {
    StructValueReader<ResponseData> r(value);
    r.Read("signedCreds", &ResponseData::signed_creds);
    r.Read("batchProof", &ResponseData::batch_proof);
    r.Read("publicKey", &ResponseData::public_key);
    return r.Finish();
  }
};

}  // namespace

URLRequest GetCredentialsEndpoint::MapRequest(const std::string& order_id,
                                              const std::string& item_id) {
  std::string host = context().Get<EnvironmentConfig>().payment_service_host();
  return URLRequest::Get("https://" + host + "/v1/orders/" + order_id +
                         "/credentials/" + item_id);
}

absl::optional<PaymentCredentials> GetCredentialsEndpoint::MapResponse(
    const URLResponse& response) {
  if (!response.Succeeded()) {
    context().LogError(FROM_HERE) << "HTTP " << response.status_code();
    return {};
  }

  if (response.status_code() == net::HTTP_ACCEPTED) {
    context().LogError(FROM_HERE) << "Credentials are not ready";
    return {};
  }

  auto data = ResponseData::FromValue(response.ReadBodyAsJSON());
  if (!data) {
    context().LogError(FROM_HERE) << "Invalid JSON response";
    return {};
  }

  PaymentCredentials result;
  result.batch_proof = std::move(data->batch_proof);
  result.public_key = std::move(data->public_key);
  result.signed_tokens = std::move(data->signed_creds);

  return std::move(result);
}

}  // namespace ledger
