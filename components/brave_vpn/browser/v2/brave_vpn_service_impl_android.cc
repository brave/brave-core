/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/v2/brave_vpn_service_impl.h"

#include "base/notimplemented.h"

namespace brave_vpn {
namespace v2 {

void BraveVpnServiceImpl::GetPurchaseToken(GetPurchaseTokenCallback callback) {
  NOTIMPLEMENTED();
  std::move(callback).Run({});
}

void BraveVpnServiceImpl::GetTimezonesForRegions(ResponseCallback callback) {
  NOTIMPLEMENTED();
}

void BraveVpnServiceImpl::GetHostnamesForRegion(
    ResponseCallback callback,
    const std::string& region,
    const std::string& region_precision) {
  NOTIMPLEMENTED();
}

void BraveVpnServiceImpl::GetProfileCredentials(
    ResponseCallback callback,
    const std::string& subscriber_credential,
    const std::string& hostname) {
  NOTIMPLEMENTED();
}

void BraveVpnServiceImpl::GetWireguardProfileCredentials(
    ResponseCallback callback,
    const std::string& subscriber_credential,
    const std::string& public_key,
    const std::string& hostname) {
  NOTIMPLEMENTED();
}

void BraveVpnServiceImpl::VerifyCredentials(
    ResponseCallback callback,
    const std::string& hostname,
    const std::string& client_id,
    const std::string& subscriber_credential,
    const std::string& api_auth_token) {
  NOTIMPLEMENTED();
}

void BraveVpnServiceImpl::InvalidateCredentials(
    ResponseCallback callback,
    const std::string& hostname,
    const std::string& client_id,
    const std::string& subscriber_credential,
    const std::string& api_auth_token) {
  NOTIMPLEMENTED();
}

void BraveVpnServiceImpl::VerifyPurchaseToken(ResponseCallback callback,
                                              const std::string& purchase_token,
                                              const std::string& product_id,
                                              const std::string& product_type,
                                              const std::string& bundle_id) {
  NOTIMPLEMENTED();
}

void BraveVpnServiceImpl::GetSubscriberCredential(
    ResponseCallback callback,
    const std::string& product_type,
    const std::string& product_id,
    const std::string& validation_method,
    const std::string& purchase_token,
    const std::string& bundle_id) {
  NOTIMPLEMENTED();
}

void BraveVpnServiceImpl::GetSubscriberCredentialV12(
    ResponseCallback callback) {
  NOTIMPLEMENTED();
}

void BraveVpnServiceImpl::RecordAllMetrics() {
  NOTIMPLEMENTED();
}

void BraveVpnServiceImpl::RecordAndroidBackgroundP3A(
    int64_t session_start_time_ms,
    int64_t session_end_time_ms) {
  NOTIMPLEMENTED();
}

}  // namespace v2
}  // namespace brave_vpn
