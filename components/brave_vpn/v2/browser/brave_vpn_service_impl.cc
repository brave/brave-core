/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/v2/browser/brave_vpn_service_impl.h"

#include "brave/components/brave_vpn/common/brave_vpn_constants.h"
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"

namespace brave_vpn {
namespace v2 {

BraveVpnServiceImpl::BraveVpnServiceImpl()
    : connection_state_(mojom::ConnectionState::DISCONNECTED),
      purchased_state_(mojom::PurchasedState::NOT_PURCHASED) {}

BraveVpnServiceImpl::~BraveVpnServiceImpl() = default;

bool BraveVpnServiceImpl::IsBraveVPNEnabled() const {
  return true;
}

bool BraveVpnServiceImpl::IsPurchased() const {
  return purchased_state_ == mojom::PurchasedState::PURCHASED;
}

void BraveVpnServiceImpl::ReloadPurchasedState() {}

std::string BraveVpnServiceImpl::GetCurrentEnvironment() const {
  return skus::kEnvProduction;
}

#if !BUILDFLAG(IS_ANDROID)

bool BraveVpnServiceImpl::IsConnected() const {
  return connection_state_ == mojom::ConnectionState::CONNECTED &&
         IsPurchased();
}

void BraveVpnServiceImpl::ToggleConnection() {}

mojom::ConnectionState BraveVpnServiceImpl::GetConnectionState() const {
  return connection_state_;
}

void BraveVpnServiceImpl::RecordWidgetUsageMetrics(bool new_usage) {}

#endif  // !BUILDFLAG(IS_ANDROID)

void BraveVpnServiceImpl::GetPurchasedState(
    GetPurchasedStateCallback callback) {
  std::move(callback).Run(
      mojom::PurchasedInfo::New(purchased_state_, std::nullopt));
}

void BraveVpnServiceImpl::LoadPurchasedState(const std::string& domain) {}

void BraveVpnServiceImpl::GetAllRegions(GetAllRegionsCallback callback) {
  std::move(callback).Run({});
}

#if !BUILDFLAG(IS_ANDROID)

void BraveVpnServiceImpl::GetConnectionState(
    GetConnectionStateCallback callback) {
  std::move(callback).Run(connection_state_);
}

void BraveVpnServiceImpl::Connect() {}

void BraveVpnServiceImpl::Disconnect() {}

void BraveVpnServiceImpl::GetSelectedRegion(
    GetSelectedRegionCallback callback) {
  std::move(callback).Run({});
}

void BraveVpnServiceImpl::SetSelectedRegion(mojom::RegionPtr region) {}

void BraveVpnServiceImpl::ClearSelectedRegion() {}

void BraveVpnServiceImpl::GetProductUrls(GetProductUrlsCallback callback) {
  std::move(callback).Run(mojom::ProductUrls::New(
      kFeedbackUrl, kAboutUrl, GetManageUrl(GetCurrentEnvironment())));
}

void BraveVpnServiceImpl::CreateSupportTicket(
    const std::string& email,
    const std::string& subject,
    const std::string& body,
    CreateSupportTicketCallback callback) {
  std::move(callback).Run(false, {});
}

void BraveVpnServiceImpl::GetSupportData(GetSupportDataCallback callback) {
  std::move(callback).Run({}, {}, {}, {});
}

void BraveVpnServiceImpl::ResetConnectionState() {}

void BraveVpnServiceImpl::EnableOnDemand(bool /*enable*/) {}

void BraveVpnServiceImpl::GetOnDemandState(GetOnDemandStateCallback callback) {
  std::move(callback).Run(false, false);
}

void BraveVpnServiceImpl::EnableSmartProxyRouting(bool /*enable*/) {}

void BraveVpnServiceImpl::GetSmartProxyRoutingState(
    GetSmartProxyRoutingStateCallback callback) {
  std::move(callback).Run(false);
}

#else  // !BUILDFLAG(IS_ANDROID)

void BraveVpnServiceImpl::GetPurchaseToken(GetPurchaseTokenCallback callback) {
  std::move(callback).Run({});
}

#endif  // !BUILDFLAG(IS_ANDROID)

#if BUILDFLAG(IS_ANDROID)

void BraveVpnServiceImpl::GetTimezonesForRegions(ResponseCallback callback) {}

void BraveVpnServiceImpl::GetHostnamesForRegion(
    ResponseCallback callback,
    const std::string& region,
    const std::string& region_precision) {}

void BraveVpnServiceImpl::GetProfileCredentials(
    ResponseCallback callback,
    const std::string& subscriber_credential,
    const std::string& hostname) {}

void BraveVpnServiceImpl::GetWireguardProfileCredentials(
    ResponseCallback callback,
    const std::string& subscriber_credential,
    const std::string& public_key,
    const std::string& hostname) {}

void BraveVpnServiceImpl::VerifyCredentials(
    ResponseCallback callback,
    const std::string& hostname,
    const std::string& client_id,
    const std::string& subscriber_credential,
    const std::string& api_auth_token) {}

void BraveVpnServiceImpl::InvalidateCredentials(
    ResponseCallback callback,
    const std::string& hostname,
    const std::string& client_id,
    const std::string& subscriber_credential,
    const std::string& api_auth_token) {}

void BraveVpnServiceImpl::VerifyPurchaseToken(ResponseCallback callback,
                                              const std::string& purchase_token,
                                              const std::string& product_id,
                                              const std::string& product_type,
                                              const std::string& bundle_id) {}

void BraveVpnServiceImpl::GetSubscriberCredential(
    ResponseCallback callback,
    const std::string& product_type,
    const std::string& product_id,
    const std::string& validation_method,
    const std::string& purchase_token,
    const std::string& bundle_id) {}

void BraveVpnServiceImpl::GetSubscriberCredentialV12(
    ResponseCallback callback) {}

void BraveVpnServiceImpl::RecordAllMetrics() {}

void BraveVpnServiceImpl::RecordAndroidBackgroundP3A(
    int64_t session_start_time_ms,
    int64_t session_end_time_ms) {}

#endif  // BUILDFLAG(IS_ANDROID)

void BraveVpnServiceImpl::Shutdown() {
  BraveVpnService::Shutdown();
}

#if !BUILDFLAG(IS_ANDROID)

void BraveVpnServiceImpl::SetConnectionStateForTesting(  // IN-TEST
    mojom::ConnectionState state) {
  connection_state_ = state;
  NotifyConnectionStateChanged(state);
}

void BraveVpnServiceImpl::SetPurchasedStateForTesting(  // IN-TEST
    const std::string& env,
    mojom::PurchasedState state) {
  purchased_state_ = state;
  NotifyPurchasedStateChanged(state, std::nullopt);
}

#endif  // !BUILDFLAG(IS_ANDROID)

}  // namespace v2
}  // namespace brave_vpn
