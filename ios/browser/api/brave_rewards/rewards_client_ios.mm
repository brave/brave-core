/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#import "rewards_client_ios.h"

#include "brave/components/brave_rewards/common/mojom/rewards.mojom.h"
#include "brave/components/brave_rewards/common/mojom/rewards_database.mojom.h"
#import "rewards_client_bridge.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// Constructor & Destructor
RewardsClientIOS::RewardsClientIOS(id<RewardsClientBridge> bridge)
    : bridge_(bridge), receiver_(this) {}
RewardsClientIOS::~RewardsClientIOS() {
  bridge_ = nil;
}

mojo::PendingAssociatedRemote<brave_rewards::mojom::RewardsEngineClient>
RewardsClientIOS::MakeRemote() {
  return receiver_.BindNewEndpointAndPassDedicatedRemote();
}

void RewardsClientIOS::FetchFavIcon(const std::string& url,
                                    const std::string& favicon_key,
                                    FetchFavIconCallback callback) {
  [bridge_ fetchFavIcon:url
             faviconKey:favicon_key
               callback:std::move(callback)];
  ;
}
void RewardsClientIOS::LoadLegacyState(LoadLegacyStateCallback callback) {
  [bridge_ loadLegacyState:std::move(callback)];
}
void RewardsClientIOS::LoadPublisherState(LoadPublisherStateCallback callback) {
  [bridge_ loadPublisherState:std::move(callback)];
}
void RewardsClientIOS::LoadURL(brave_rewards::mojom::UrlRequestPtr request,
                               LoadURLCallback callback) {
  [bridge_ loadUrl:std::move(request) callback:std::move(callback)];
}
void RewardsClientIOS::GetSPLTokenAccountBalance(
    const std::string& solana_address,
    const std::string& token_mint_address,
    GetSPLTokenAccountBalanceCallback callback) {
  std::move(callback).Run(nullptr);
}
void RewardsClientIOS::Log(const std::string& file,
                           int32_t line,
                           int32_t verbose_level,
                           const std::string& message) {
  [bridge_ log:file line:line verboseLevel:verbose_level message:message];
}
void RewardsClientIOS::OnPanelPublisherInfo(
    brave_rewards::mojom::Result result,
    brave_rewards::mojom::PublisherInfoPtr publisher_info,
    uint64_t windowId) {
  [bridge_ onPanelPublisherInfo:result
                  publisherInfo:std::move(publisher_info)
                       windowId:windowId];
}
void RewardsClientIOS::OnPublisherRegistryUpdated() {
  [bridge_ onPublisherRegistryUpdated];
}
void RewardsClientIOS::OnPublisherUpdated(const std::string& publisher_id) {
  [bridge_ onPublisherUpdated:publisher_id];
}
void RewardsClientIOS::OnReconcileComplete(
    brave_rewards::mojom::Result result,
    brave_rewards::mojom::ContributionInfoPtr contribution) {
  [bridge_ onReconcileComplete:result contribution:std::move(contribution)];
}
void RewardsClientIOS::PublisherListNormalized(
    std::vector<brave_rewards::mojom::PublisherInfoPtr> list) {
  [bridge_ publisherListNormalized:std::move(list)];
}
void RewardsClientIOS::SetBooleanState(const std::string& name,
                                       bool value,
                                       SetBooleanStateCallback callback) {
  [bridge_ setBooleanState:name value:value callback:std::move(callback)];
}
void RewardsClientIOS::GetBooleanState(const std::string& name,
                                       GetBooleanStateCallback callback) {
  [bridge_ booleanState:name callback:std::move(callback)];
}
void RewardsClientIOS::SetIntegerState(const std::string& name,
                                       int32_t value,
                                       SetIntegerStateCallback callback) {
  [bridge_ setIntegerState:name value:value callback:std::move(callback)];
}
void RewardsClientIOS::GetIntegerState(const std::string& name,
                                       GetIntegerStateCallback callback) {
  [bridge_ integerState:name callback:std::move(callback)];
}
void RewardsClientIOS::SetDoubleState(const std::string& name,
                                      double value,
                                      SetDoubleStateCallback callback) {
  [bridge_ setDoubleState:name value:value callback:std::move(callback)];
}
void RewardsClientIOS::GetDoubleState(const std::string& name,
                                      GetDoubleStateCallback callback) {
  [bridge_ doubleState:name callback:std::move(callback)];
}
void RewardsClientIOS::SetStringState(const std::string& name,
                                      const std::string& value,
                                      SetStringStateCallback callback) {
  [bridge_ setStringState:name value:value callback:std::move(callback)];
}
void RewardsClientIOS::GetStringState(const std::string& name,
                                      GetStringStateCallback callback) {
  [bridge_ stringState:name callback:std::move(callback)];
}
void RewardsClientIOS::SetInt64State(const std::string& name,
                                     int64_t value,
                                     SetInt64StateCallback callback) {
  [bridge_ setInt64State:name value:value callback:std::move(callback)];
}
void RewardsClientIOS::GetInt64State(const std::string& name,
                                     GetInt64StateCallback callback) {
  [bridge_ int64State:name callback:std::move(callback)];
}
void RewardsClientIOS::SetUint64State(const std::string& name,
                                      uint64_t value,
                                      SetUint64StateCallback callback) {
  [bridge_ setUint64State:name value:value callback:std::move(callback)];
}
void RewardsClientIOS::GetUint64State(const std::string& name,
                                      GetUint64StateCallback callback) {
  [bridge_ uint64State:name callback:std::move(callback)];
}
void RewardsClientIOS::SetValueState(const std::string& name,
                                     base::Value value,
                                     SetValueStateCallback callback) {
  [bridge_ setValueState:name
                   value:std::move(value)
                callback:std::move(callback)];
}
void RewardsClientIOS::GetValueState(const std::string& name,
                                     GetValueStateCallback callback) {
  [bridge_ valueState:name callback:std::move(callback)];
}
void RewardsClientIOS::SetTimeState(const std::string& name,
                                    base::Time value,
                                    SetTimeStateCallback callback) {
  [bridge_ setTimeState:name value:value callback:std::move(callback)];
}
void RewardsClientIOS::GetTimeState(const std::string& name,
                                    GetTimeStateCallback callback) {
  [bridge_ timeState:name callback:std::move(callback)];
}
void RewardsClientIOS::ClearState(const std::string& name,
                                  ClearStateCallback callback) {
  [bridge_ clearState:name callback:std::move(callback)];
}
void RewardsClientIOS::GetLegacyWallet(GetLegacyWalletCallback callback) {
  [bridge_ legacyWallet:std::move(callback)];
}
void RewardsClientIOS::ShowNotification(const std::string& type,
                                        const std::vector<std::string>& args,
                                        ShowNotificationCallback callback) {
  [bridge_ showNotification:type args:args callback:std::move(callback)];
}
void RewardsClientIOS::GetClientCountryCode(
    GetClientCountryCodeCallback callback) {
  [bridge_ getClientCountryCode:std::move(callback)];
}
void RewardsClientIOS::GetClientInfo(GetClientInfoCallback callback) {
  [bridge_ clientInfo:std::move(callback)];
}
void RewardsClientIOS::ReconcileStampReset() {
  [bridge_ reconcileStampReset];
}
void RewardsClientIOS::RunDBTransaction(
    brave_rewards::mojom::DBTransactionPtr transaction,
    RunDBTransactionCallback callback) {
  [bridge_ runDbTransaction:std::move(transaction)
                   callback:std::move(callback)];
}
void RewardsClientIOS::ClearAllNotifications() {
  [bridge_ clearAllNotifications];
}
void RewardsClientIOS::ExternalWalletConnected() {
  [bridge_ externalWalletConnected];
}
void RewardsClientIOS::ExternalWalletLoggedOut() {
  [bridge_ externalWalletLoggedOut];
}
void RewardsClientIOS::ExternalWalletReconnected() {
  [bridge_ externalWalletReconnected];
}
void RewardsClientIOS::ExternalWalletDisconnected() {
  [bridge_ externalWalletDisconnected];
}
void RewardsClientIOS::DeleteLog(DeleteLogCallback callback) {
  [bridge_ deleteLog:std::move(callback)];
}
void RewardsClientIOS::EncryptString(const std::string& value,
                                     EncryptStringCallback callback) {
  [bridge_ encryptString:value callback:std::move(callback)];
}
void RewardsClientIOS::DecryptString(const std::string& value,
                                     DecryptStringCallback callback) {
  [bridge_ decryptString:value callback:std::move(callback)];
}
