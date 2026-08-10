/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/v2/api/brave_vpn_api_client.h"

#include <optional>
#include <utility>

#include "base/check.h"
#include "base/check_deref.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/strings/strcat.h"
#include "base/types/expected.h"
#include "brave/components/brave_account/endpoint_client/client.h"
#include "brave/components/brave_account/endpoint_client/with_headers.h"
#include "net/base/net_errors.h"
#include "net/http/http_status_code.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

using brave_account::endpoint_client::Client;
using brave_account::endpoint_client::WithHeaders;

namespace brave_vpn::v2 {

using endpoints::GetSubscriberCredential;
using endpoints::GetSubscriberCredentialV12;
using endpoints::VerifyPurchaseToken;

namespace {

const net::NetworkTrafficAnnotationTag kTrafficAnnotation =
    net::DefineNetworkTrafficAnnotation("brave_vpn_api_client", R"(
      semantics {
        sender: "Brave VPN Service"
        description:
          "This service is used to communicate with Guardian VPN apis"
          "on behalf of the user interacting with the Brave VPN."
        trigger:
          "Triggered by user connecting the Brave VPN."
        data:
          "Servers, hosts and credentials for Brave VPN"
        destination: WEBSITE
      }
      policy {
        cookies_allowed: NO
        policy_exception_justification:
          "Not implemented."
      }
    )");

// Builds a request with the traffic annotation and retry policy common to all
// Brave VPN endpoints.
template <typename Request>
Request MakeRequest() {
  Request request;
  request.network_traffic_annotation_tag =
      net::MutableNetworkTrafficAnnotationTag(kTrafficAnnotation);
  request.retry_options = {
      .max_retries = 1,
      .retry_mode = network::SimpleURLLoader::RETRY_ON_NETWORK_CHANGE};
  return request;
}

// Describes a failed response for which no typed body was recovered; i.e. a
// transport-level or HTTP-level failure rather than a parsed error body.
template <typename Response>
std::optional<std::string> MaybeDescribeUnrecoverableResponse(
    const Response& response) {
  if (response.body) {
    return std::nullopt;
  }
  return response.status_code
      .transform([](int status_code) {
        return absl::StrFormat("HTTP %d %s: body missing or failed to parse",
                               status_code,
                               net::GetHttpReasonPhrase(status_code));
      })
      .value_or(base::StrCat({net::ErrorToString(response.net_error),
                              response.net_error == net::OK
                                  ? " (no response headers received)"
                                  : ""}));
}

}  // namespace

BraveVpnApiClient::BraveVpnApiClient(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : url_loader_factory_(std::move(url_loader_factory)) {
  CHECK(url_loader_factory_);
}

BraveVpnApiClient::~BraveVpnApiClient() = default;

void BraveVpnApiClient::GetSubscriberCredential(
    SubscriberCredentialCallback callback,
    const std::string& product_type,
    const std::string& product_id,
    const std::string& validation_method,
    const std::string& purchase_token,
    const std::string& bundle_id) {
  auto request = MakeRequest<GetSubscriberCredential::Request>();
  request.body.product_type = product_type;
  request.body.product_id = product_id;
  request.body.validation_method = validation_method;
  request.body.purchase_token = purchase_token;
  request.body.bundle_id = bundle_id;

  Client<endpoints::GetSubscriberCredential>::Send(
      url_loader_factory_, std::move(request),
      base::BindOnce(&BraveVpnApiClient::OnGetSubscriberCredentialResponse,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void BraveVpnApiClient::OnGetSubscriberCredentialResponse(
    SubscriberCredentialCallback callback,
    endpoints::GetSubscriberCredential::Response response) {
  if (auto unrecoverable = MaybeDescribeUnrecoverableResponse(response)) {
    return std::move(callback).Run(base::unexpected(*std::move(unrecoverable)));
  }

  std::move(callback).Run(
      std::move(CHECK_DEREF(response.body))
          .transform([](endpoints::GetSubscriberCredentialSuccessBody success) {
            return std::move(success.subscriber_credential);
          })
          .transform_error([](endpoints::VpnErrorBody error) {
            return std::move(error.error_title);
          }));
}

void BraveVpnApiClient::GetSubscriberCredentialV12(
    SubscriberCredentialCallback callback,
    const std::string& skus_credential,
    const std::string& environment) {
  CHECK(!skus_credential.empty());
  CHECK(!environment.empty());

  auto request =
      MakeRequest<WithHeaders<GetSubscriberCredentialV12::Request>>();
  request.body.skus_credential = skus_credential;
  request.headers.SetHeader(endpoints::kHeaderBravePaymentsEnvironment,
                            environment);

  Client<endpoints::GetSubscriberCredentialV12>::Send(
      url_loader_factory_, std::move(request),
      base::BindOnce(&BraveVpnApiClient::OnGetSubscriberCredentialResponse,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void BraveVpnApiClient::VerifyPurchaseToken(
    VerifyPurchaseTokenCallback callback,
    const std::string& purchase_token,
    const std::string& product_id,
    const std::string& product_type,
    const std::string& bundle_id) {
  auto request = MakeRequest<VerifyPurchaseToken::Request>();
  request.body.purchase_token = purchase_token;
  request.body.product_id = product_id;
  request.body.product_type = product_type;
  request.body.bundle_id = bundle_id;

  Client<endpoints::VerifyPurchaseToken>::Send(
      url_loader_factory_, std::move(request),
      base::BindOnce(&BraveVpnApiClient::OnVerifyPurchaseTokenResponse,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void BraveVpnApiClient::OnVerifyPurchaseTokenResponse(
    VerifyPurchaseTokenCallback callback,
    endpoints::VerifyPurchaseToken::Response response) {
  if (auto unrecoverable = MaybeDescribeUnrecoverableResponse(response)) {
    return std::move(callback).Run(base::unexpected(*std::move(unrecoverable)));
  }

  std::move(callback).Run(
      std::move(CHECK_DEREF(response.body))
          .transform([](endpoints::RawJsonResponseBody success) {
            return std::move(success.json);
          })
          .transform_error([](endpoints::RawJsonResponseBody error) {
            return std::move(error.json);
          }));
}

}  // namespace brave_vpn::v2
