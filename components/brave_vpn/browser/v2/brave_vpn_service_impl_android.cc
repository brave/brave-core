/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/v2/brave_vpn_service_impl.h"

#include <optional>
#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/notimplemented.h"
#include "base/types/expected.h"
#include "brave/components/brave_vpn/browser/v2/api/brave_vpn_api_client.h"
#include "brave/components/brave_vpn/browser/v2/api/transport_protocol.h"
#include "brave/components/brave_vpn/browser/v2/purchased_state_manager.h"

namespace brave_vpn::v2 {
namespace {
// Bridges the endpoint client's typed result to the legacy ResponseCallback
// contract: value -> (payload, true); error -> (error string, false).
void RunResponseCallback(BraveVpnServiceImpl::ResponseCallback callback,
                         base::expected<std::string, std::string> result) {
  const bool success = result.has_value();
  std::move(callback).Run(
      success ? std::move(result).value() : std::move(result).error(), success);
}
}  // namespace

void BraveVpnServiceImpl::GetPurchaseToken(GetPurchaseTokenCallback callback) {
  NOTIMPLEMENTED();
  std::move(callback).Run({});
}

void BraveVpnServiceImpl::GetTimezonesForRegions(ResponseCallback callback) {
  if (!api_client_) {
    std::move(callback).Run({}, false);
    return;
  }
  api_client_->GetTimezonesForRegions(
      base::BindOnce(&RunResponseCallback, std::move(callback)));
}

void BraveVpnServiceImpl::GetHostnamesForRegion(
    ResponseCallback callback,
    const std::string& region,
    const std::string& region_precision) {
  if (!api_client_) {
    std::move(callback).Run({}, false);
    return;
  }
  api_client_->GetHostnamesForRegion(
      base::BindOnce(&RunResponseCallback, std::move(callback)), region,
      region_precision);
}

void BraveVpnServiceImpl::GetIKEv2ProfileCredentials(
    ResponseCallback callback,
    const std::string& subscriber_credential,
    const std::string& hostname) {
  if (!api_client_) {
    std::move(callback).Run({}, false);
    return;
  }
  api_client_->GetProfileCredentials(
      base::BindOnce(&RunResponseCallback, std::move(callback)), hostname,
      subscriber_credential, endpoints::TransportProtocol::kIKEv2, std::nullopt,
      std::nullopt);
}

void BraveVpnServiceImpl::GetWireguardProfileCredentials(
    ResponseCallback callback,
    const std::string& subscriber_credential,
    const std::string& public_key,
    const std::string& hostname) {
  if (!api_client_) {
    std::move(callback).Run({}, false);
    return;
  }
  api_client_->GetProfileCredentials(
      base::BindOnce(&RunResponseCallback, std::move(callback)), hostname,
      subscriber_credential, endpoints::TransportProtocol::kWireguard,
      public_key, std::nullopt);
}

void BraveVpnServiceImpl::VerifyCredentials(
    ResponseCallback callback,
    const std::string& hostname,
    const std::string& client_id,
    const std::string& /*subscriber_credential*/,
    const std::string& api_auth_token) {
  if (!api_client_) {
    std::move(callback).Run({}, false);
    return;
  }
  // Note: in v1.4 API, the subscriber credential is not used for verification.
  api_client_->VerifyCredentials(
      base::BindOnce(&RunResponseCallback, std::move(callback)), hostname,
      client_id, api_auth_token);
}

void BraveVpnServiceImpl::InvalidateCredentials(
    ResponseCallback callback,
    const std::string& hostname,
    const std::string& client_id,
    const std::string& subscriber_credential,
    const std::string& api_auth_token) {
  if (!api_client_) {
    std::move(callback).Run({}, false);
    return;
  }
  api_client_->InvalidateCredentials(
      base::BindOnce(&RunResponseCallback, std::move(callback)), hostname,
      client_id, api_auth_token, subscriber_credential);
}

void BraveVpnServiceImpl::VerifyPurchaseToken(ResponseCallback callback,
                                              const std::string& purchase_token,
                                              const std::string& product_id,
                                              const std::string& product_type,
                                              const std::string& bundle_id) {
  if (!api_client_) {
    std::move(callback).Run({}, false);
    return;
  }
  api_client_->VerifyPurchaseToken(
      base::BindOnce(&RunResponseCallback, std::move(callback)), purchase_token,
      product_id, product_type, bundle_id);
}

void BraveVpnServiceImpl::GetSubscriberCredential(
    ResponseCallback callback,
    const std::string& product_type,
    const std::string& product_id,
    const std::string& validation_method,
    const std::string& purchase_token,
    const std::string& bundle_id) {
  if (!api_client_) {
    std::move(callback).Run({}, false);
    return;
  }
  api_client_->GetSubscriberCredential(
      base::BindOnce(&RunResponseCallback, std::move(callback)), product_type,
      product_id, validation_method, purchase_token, bundle_id);
}

void BraveVpnServiceImpl::GetSubscriberCredentialV12(
    ResponseCallback callback) {
  std::optional<std::string> subscriber_credential =
      purchased_state_manager_
          ? purchased_state_manager_->GetSubscriberCredential()
          : std::nullopt;
  std::move(callback).Run(subscriber_credential.value_or(""),
                          subscriber_credential.has_value());
}

void BraveVpnServiceImpl::RecordAllMetrics() {
  NOTIMPLEMENTED();
}

void BraveVpnServiceImpl::RecordAndroidBackgroundP3A(
    int64_t session_start_time_ms,
    int64_t session_end_time_ms) {
  NOTIMPLEMENTED();
}

}  // namespace brave_vpn::v2
