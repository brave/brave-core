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
#include "brave/components/brave_vpn/browser/v2/api/device_endpoints.h"
#include "brave/components/brave_vpn/browser/v2/api/purchase_endpoints.h"
#include "brave/components/brave_vpn/browser/v2/api/region_endpoints.h"
#include "brave/components/brave_vpn/browser/v2/api/support_endpoints.h"
#include "net/base/net_errors.h"
#include "net/http/http_status_code.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

using brave_account::endpoint_client::Client;
using brave_account::endpoint_client::WithHeaders;

namespace brave_vpn::v2 {

using endpoints::CreateSupportTicket;
using endpoints::GetAvailableMultihopExitRegions;
using endpoints::GetHostnamesForRegion;
using endpoints::GetProfileCredentials;
using endpoints::GetServerRegions;
using endpoints::GetSubscriberCredential;
using endpoints::GetSubscriberCredentialV12;
using endpoints::GetTimezonesForRegions;
using endpoints::InvalidateCredentials;
using endpoints::SetMultihopExitRegion;
using endpoints::VerifyCredentials;
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

void BraveVpnApiClient::OnRawJsonResponse(RawJsonCallback callback,
                                          RawJsonResponse response) {
  if (auto unrecoverable = MaybeDescribeUnrecoverableResponse(response)) {
    return std::move(callback).Run(base::unexpected(*std::move(unrecoverable)));
  }

  std::move(callback).Run(
      std::move(CHECK_DEREF(response.body))
          .transform([](endpoints::RawJsonResponseBody success) {
            return std::move(success.json);
          })
          .transform_error([](endpoints::VpnErrorBody error) {
            return std::move(error.error_title);
          }));
}

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

void BraveVpnApiClient::CreateSupportTicket(
    RawJsonCallback callback,
    const std::string& email,
    const std::string& subject,
    const std::string& body,
    const std::string& subscriber_credential,
    const std::string& timezone) {
  auto request = MakeRequest<CreateSupportTicket::Request>();
  request.body.email = email;
  request.body.subject = subject;
  request.body.body = body;
  request.body.subscriber_credential = subscriber_credential;
  request.body.timezone = timezone;

  Client<endpoints::CreateSupportTicket>::Send(
      url_loader_factory_, std::move(request),
      base::BindOnce(&BraveVpnApiClient::OnRawJsonResponse,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void BraveVpnApiClient::GetServerRegions(RawJsonCallback callback,
                                         const std::string& region_precision) {
  auto request = MakeRequest<GetServerRegions::Request>();
  request.url_replacements.SetPath(
      base::StrCat({GetServerRegions::URL().path(), "/", region_precision}));

  Client<endpoints::GetServerRegions>::Send(
      url_loader_factory_, std::move(request),
      base::BindOnce(&BraveVpnApiClient::OnRawJsonResponse,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void BraveVpnApiClient::GetTimezonesForRegions(RawJsonCallback callback) {
  auto request = MakeRequest<GetTimezonesForRegions::Request>();
  Client<endpoints::GetTimezonesForRegions>::Send(
      url_loader_factory_, std::move(request),
      base::BindOnce(&BraveVpnApiClient::OnRawJsonResponse,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void BraveVpnApiClient::GetHostnamesForRegion(
    RawJsonCallback callback,
    const std::string& region,
    const std::string& region_precision) {
  auto request = MakeRequest<GetHostnamesForRegion::Request>();
  request.body.region = region;
  request.body.region_precision = region_precision;

  Client<endpoints::GetHostnamesForRegion>::Send(
      url_loader_factory_, std::move(request),
      base::BindOnce(&BraveVpnApiClient::OnRawJsonResponse,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void BraveVpnApiClient::GetProfileCredentials(
    RawJsonCallback callback,
    const std::string& hostname,
    const std::string& subscriber_credential,
    endpoints::TransportProtocol transport_protocol,
    const std::optional<std::string>& public_key,
    const std::optional<std::string>& multihop_exit_region) {
  CHECK_EQ(public_key.has_value(),
           transport_protocol == endpoints::TransportProtocol::kWireguard)
      << "public key must be set if and only if transport protocol is "
         "WireGuard";

  auto request = MakeRequest<GetProfileCredentials::Request>();
  request.body.subscriber_credential = subscriber_credential;
  request.body.transport_protocol = transport_protocol;
  request.body.public_key = public_key.value_or("");
  request.body.multihop_exit_region = multihop_exit_region;
  request.url_replacements.SetHost(hostname);

  Client<endpoints::GetProfileCredentials>::Send(
      url_loader_factory_, std::move(request),
      base::BindOnce(&BraveVpnApiClient::OnRawJsonResponse,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void BraveVpnApiClient::VerifyCredentials(RawJsonCallback callback,
                                          const std::string& hostname,
                                          const std::string& client_id,
                                          const std::string& api_auth_token) {
  auto request = MakeRequest<WithHeaders<VerifyCredentials::Request>>();
  request.headers.SetHeader(endpoints::kHeaderGrdApiAuthToken, api_auth_token);
  request.url_replacements.SetHost(hostname);
  request.url_replacements.SetPath(
      base::StrCat({VerifyCredentials::URL().path(), "/", client_id, "/",
                    endpoints::kDeviceApiVerifyCredentialsSuffix}));

  Client<endpoints::VerifyCredentials>::Send(
      url_loader_factory_, std::move(request),
      base::BindOnce(&BraveVpnApiClient::OnRawJsonResponse,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void BraveVpnApiClient::InvalidateCredentials(
    RawJsonCallback callback,
    const std::string& hostname,
    const std::string& client_id,
    const std::string& api_auth_token,
    const std::string& subscriber_credential) {
  auto request = MakeRequest<InvalidateCredentials::Request>();
  request.body.api_auth_token = api_auth_token;
  request.body.subscriber_credential = subscriber_credential;
  request.url_replacements.SetHost(hostname);
  request.url_replacements.SetPath(
      base::StrCat({InvalidateCredentials::URL().path(), "/", client_id, "/",
                    endpoints::kDeviceApiInvalidateCredentialsSuffix}));

  Client<endpoints::InvalidateCredentials>::Send(
      url_loader_factory_, std::move(request),
      base::BindOnce(&BraveVpnApiClient::OnRawJsonResponse,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void BraveVpnApiClient::GetAvailableMultihopExitRegions(
    RawJsonCallback callback,
    const std::string& hostname,
    const std::string& client_id,
    const std::string& api_auth_token) {
  auto request =
      MakeRequest<WithHeaders<GetAvailableMultihopExitRegions::Request>>();
  request.headers.SetHeader(endpoints::kHeaderGrdApiAuthToken, api_auth_token);
  request.url_replacements.SetHost(hostname);
  request.url_replacements.SetPath(base::StrCat(
      {GetAvailableMultihopExitRegions::URL().path(), "/", client_id, "/",
       endpoints::kDeviceApiConfigMultihopSuffix}));

  Client<endpoints::GetAvailableMultihopExitRegions>::Send(
      url_loader_factory_, std::move(request),
      base::BindOnce(&BraveVpnApiClient::OnRawJsonResponse,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void BraveVpnApiClient::SetMultihopExitRegion(
    RawJsonCallback callback,
    const std::string& hostname,
    const std::string& client_id,
    const std::string& api_auth_token,
    const std::string& multihop_exit_region) {
  CHECK(!multihop_exit_region.empty());
  CHECK_NE(multihop_exit_region, endpoints::kMultihopDisabledValue)
      << "Use ClearMultihopExitRegion() to disable multihop.";
  DoSetMultihopExitRegion(std::move(callback), hostname, client_id,
                          api_auth_token, multihop_exit_region);
}

void BraveVpnApiClient::ClearMultihopExitRegion(
    RawJsonCallback callback,
    const std::string& hostname,
    const std::string& client_id,
    const std::string& api_auth_token) {
  DoSetMultihopExitRegion(std::move(callback), hostname, client_id,
                          api_auth_token, endpoints::kMultihopDisabledValue);
}

void BraveVpnApiClient::DoSetMultihopExitRegion(
    RawJsonCallback callback,
    const std::string& hostname,
    const std::string& client_id,
    const std::string& api_auth_token,
    const std::string& multihop_exit_region) {
  auto request = MakeRequest<SetMultihopExitRegion::Request>();
  request.body.api_auth_token = api_auth_token;
  request.body.multihop_exit_region = multihop_exit_region;
  request.url_replacements.SetHost(hostname);
  request.url_replacements.SetPath(
      base::StrCat({SetMultihopExitRegion::URL().path(), "/", client_id, "/",
                    endpoints::kDeviceApiConfigMultihopSuffix}));

  Client<endpoints::SetMultihopExitRegion>::Send(
      url_loader_factory_, std::move(request),
      base::BindOnce(&BraveVpnApiClient::OnRawJsonResponse,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

}  // namespace brave_vpn::v2
