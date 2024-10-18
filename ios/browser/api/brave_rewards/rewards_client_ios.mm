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
void RewardsClientIOS::SetUserPreferenceValue(
    const std::string& path,
    base::Value value,
    SetUserPreferenceValueCallback callback) {
  [bridge_ setUserPreferenceValue:path
                            value:std::move(value)
                         callback:std::move(callback)];
}

void RewardsClientIOS::GetUserPreferenceValue(
    const std::string& path,
    GetUserPreferenceValueCallback callback) {
  [bridge_ userPreferenceValue:path callback:std::move(callback)];
}

void RewardsClientIOS::ClearUserPreferenceValue(
    const std::string& path,
    ClearUserPreferenceValueCallback callback) {
  [bridge_ clearUserPreferenceValue:path callback:std::move(callback)];
}

void RewardsClientIOS::ShowNotification(const std::string& type,
                                        const std::vector<std::string>& args,
                                        ShowNotificationCallback callback) {
  [bridge_ showNotification:type args:args callback:std::move(callback)];
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
