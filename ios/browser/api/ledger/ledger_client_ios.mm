/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "ledger_client_ios.h"
#import "ledger_client_bridge.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// Constructor & Destructor
LedgerClientIOS::LedgerClientIOS(id<LedgerClientBridge> bridge)
    : bridge_(bridge) {}
LedgerClientIOS::~LedgerClientIOS() {
  bridge_ = nil;
}

void LedgerClientIOS::FetchFavIcon(const std::string& url,
                                   const std::string& favicon_key,
                                   ledger::client::FetchIconCallback callback) {
  [bridge_ fetchFavIcon:url faviconKey:favicon_key callback:callback];
}
void LedgerClientIOS::LoadLedgerState(ledger::client::OnLoadCallback callback) {
  [bridge_ loadLedgerState:callback];
}
void LedgerClientIOS::LoadPublisherState(
    ledger::client::OnLoadCallback callback) {
  [bridge_ loadPublisherState:callback];
}
void LedgerClientIOS::LoadURL(ledger::type::UrlRequestPtr request,
                              ledger::client::LoadURLCallback callback) {
  [bridge_ loadURL:std::move(request) callback:callback];
}
void LedgerClientIOS::Log(const char* file,
                          const int line,
                          const int verbose_level,
                          const std::string& message) {
  [bridge_ log:file line:line verboseLevel:verbose_level message:message];
}
void LedgerClientIOS::OnPanelPublisherInfo(
    ledger::type::Result result,
    ledger::type::PublisherInfoPtr publisher_info,
    uint64_t windowId) {
  [bridge_ onPanelPublisherInfo:result
                  publisherInfo:std::move(publisher_info)
                       windowId:windowId];
}
void LedgerClientIOS::OnReconcileComplete(
    ledger::type::Result result,
    ledger::type::ContributionInfoPtr contribution) {
  [bridge_ onReconcileComplete:result contribution:std::move(contribution)];
}
void LedgerClientIOS::PublisherListNormalized(
    ledger::type::PublisherInfoList list) {
  [bridge_ publisherListNormalized:std::move(list)];
}
std::string LedgerClientIOS::URIEncode(const std::string& value) {
  return [bridge_ URIEncode:value];
}
void LedgerClientIOS::OnContributeUnverifiedPublishers(
    ledger::type::Result result,
    const std::string& publisher_key,
    const std::string& publisher_name) {
  return [bridge_ onContributeUnverifiedPublishers:result
                                      publisherKey:publisher_key
                                     publisherName:publisher_name];
}
void LedgerClientIOS::SetBooleanState(const std::string& name, bool value) {
  [bridge_ setBooleanState:name value:value];
}
bool LedgerClientIOS::GetBooleanState(const std::string& name) const {
  return [bridge_ getBooleanState:name];
}
void LedgerClientIOS::SetIntegerState(const std::string& name, int value) {
  [bridge_ setIntegerState:name value:value];
}
int LedgerClientIOS::GetIntegerState(const std::string& name) const {
  return [bridge_ getIntegerState:name];
}
void LedgerClientIOS::SetDoubleState(const std::string& name, double value) {
  [bridge_ setDoubleState:name value:value];
}
double LedgerClientIOS::GetDoubleState(const std::string& name) const {
  return [bridge_ getDoubleState:name];
}
void LedgerClientIOS::SetStringState(const std::string& name,
                                     const std::string& value) {
  [bridge_ setStringState:name value:value];
}
std::string LedgerClientIOS::GetStringState(const std::string& name) const {
  return [bridge_ getStringState:name];
}
void LedgerClientIOS::SetInt64State(const std::string& name, int64_t value) {
  [bridge_ setInt64State:name value:value];
}
int64_t LedgerClientIOS::GetInt64State(const std::string& name) const {
  return [bridge_ getInt64State:name];
}
void LedgerClientIOS::SetUint64State(const std::string& name, uint64_t value) {
  [bridge_ setUint64State:name value:value];
}
uint64_t LedgerClientIOS::GetUint64State(const std::string& name) const {
  return [bridge_ getUint64State:name];
}
void LedgerClientIOS::ClearState(const std::string& name) {
  [bridge_ clearState:name];
}
std::string LedgerClientIOS::GetLegacyWallet() {
  return [bridge_ getLegacyWallet];
}
void LedgerClientIOS::ShowNotification(
    const std::string& type,
    const std::vector<std::string>& args,
    ledger::client::ResultCallback callback) {
  [bridge_ showNotification:type args:args callback:callback];
}
bool LedgerClientIOS::GetBooleanOption(const std::string& name) const {
  return [bridge_ getBooleanOption:name];
}
int LedgerClientIOS::GetIntegerOption(const std::string& name) const {
  return [bridge_ getIntegerOption:name];
}
double LedgerClientIOS::GetDoubleOption(const std::string& name) const {
  return [bridge_ getDoubleOption:name];
}
std::string LedgerClientIOS::GetStringOption(const std::string& name) const {
  return [bridge_ getStringOption:name];
}
int64_t LedgerClientIOS::GetInt64Option(const std::string& name) const {
  return [bridge_ getInt64Option:name];
}
uint64_t LedgerClientIOS::GetUint64Option(const std::string& name) const {
  return [bridge_ getUint64Option:name];
}
ledger::type::ClientInfoPtr LedgerClientIOS::GetClientInfo() {
  return [bridge_ getClientInfo];
}
void LedgerClientIOS::UnblindedTokensReady() {
  [bridge_ unblindedTokensReady];
}
void LedgerClientIOS::ReconcileStampReset() {
  [bridge_ reconcileStampReset];
}
void LedgerClientIOS::RunDBTransaction(
    ledger::type::DBTransactionPtr transaction,
    ledger::client::RunDBTransactionCallback callback) {
  [bridge_ runDBTransaction:std::move(transaction) callback:callback];
}
void LedgerClientIOS::GetCreateScript(
    ledger::client::GetCreateScriptCallback callback) {
  [bridge_ getCreateScript:callback];
}
void LedgerClientIOS::PendingContributionSaved(
    const ledger::type::Result result) {
  [bridge_ pendingContributionSaved:result];
}
void LedgerClientIOS::ClearAllNotifications() {
  [bridge_ clearAllNotifications];
}
void LedgerClientIOS::WalletDisconnected(const std::string& wallet_type) {
  [bridge_ walletDisconnected:wallet_type];
}
void LedgerClientIOS::DeleteLog(ledger::client::ResultCallback callback) {
  [bridge_ deleteLog:callback];
}
absl::optional<std::string> LedgerClientIOS::EncryptString(
    const std::string& value) {
  return [bridge_ encryptString:value];
}
absl::optional<std::string> LedgerClientIOS::DecryptString(
    const std::string& value) {
  return [bridge_ decryptString:value];
}
