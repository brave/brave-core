/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/v2/api/brave_vpn_api_client.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/types/expected.h"
#include "brave/components/brave_account/endpoint_client/client.h"
#include "brave/components/brave_account/endpoint_client/with_headers.h"
#include "net/base/net_errors.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

namespace brave_vpn::v2 {

namespace {
constexpr char kHeaderBravePaymentsEnvironment[] = "Brave-Payments-Environment";

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
#if 0
  // TODO(https://github.com/brave/brave-browser/issues/57112)
  // Enable retry policy once the PR related to the issue has been merged.
  request.retry_options = {
      .max_retries = 1,
      .retry_mode = network::SimpleURLLoader::RETRY_ON_NETWORK_CHANGE};
#endif
  return request;
}

// Describes a failed response for which no typed body was recovered; i.e. a
// transport-level or HTTP-level failure rather than a parsed error body.
template <typename Response>
std::optional<std::string> MaybeDescribeTransportFailure(
    const Response& response) {
  if (response.body) {
    return std::nullopt;
  }
  if (response.net_error != net::OK) {
    return net::ErrorToString(response.net_error);
  }
  if (response.status_code) {
    return absl::StrFormat("Unexpected HTTP status: %d", *response.status_code);
  }
  return "No response received";
}

}  // namespace

BraveVpnApiClient::BraveVpnApiClient(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : url_loader_factory_(std::move(url_loader_factory)) {}

BraveVpnApiClient::~BraveVpnApiClient() = default;

void BraveVpnApiClient::GetSubscriberCredentialV12(
    SubscriberCredentialCallback callback,
    const std::string& skus_credential,
    const std::string& environment) {
  using Endpoint = endpoints::GetSubscriberCredentialV12;
  using Request =
      brave_account::endpoint_client::WithHeaders<Endpoint::Request>;

  auto request = MakeRequest<Request>();
  request.body.skus_credential = skus_credential;
  request.headers.SetHeader(kHeaderBravePaymentsEnvironment, environment);

  brave_account::endpoint_client::Client<Endpoint>::Send(
      url_loader_factory_, std::move(request),
      base::BindOnce(&BraveVpnApiClient::OnGetSubscriberCredentialV12Response,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void BraveVpnApiClient::OnGetSubscriberCredentialV12Response(
    SubscriberCredentialCallback callback,
    endpoints::GetSubscriberCredentialV12::Response response) {
  std::optional<std::string> maybe_transport_error =
      MaybeDescribeTransportFailure(response);
  if (maybe_transport_error.has_value()) {
    std::move(callback).Run(base::unexpected(maybe_transport_error.value()));
    return;
  }

  std::move(callback).Run(
      std::move(*response.body)
          .transform(
              [](endpoints::GetSubscriberCredentialV12ResponseBody success) {
                return std::move(success.subscriber_credential);
              })
          .transform_error([](endpoints::VpnErrorBody error) {
            return std::move(error.error_title);
          }));
}

}  // namespace brave_vpn::v2
