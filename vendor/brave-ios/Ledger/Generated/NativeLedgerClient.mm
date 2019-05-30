/* WARNING: THIS FILE IS GENERATED. ANY CHANGES TO THIS FILE WILL BE OVERWRITTEN
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "NativeLedgerClient.h"
#import "NativeLedgerClientBridge.h"

// Constructor & Destructor
NativeLedgerClient::NativeLedgerClient(id<NativeLedgerClientBridge> bridge) : bridge_(bridge) { }
NativeLedgerClient::~NativeLedgerClient() {
  bridge_ = nil;
}

void NativeLedgerClient::ConfirmationsTransactionHistoryDidChange() {
  [bridge_ confirmationsTransactionHistoryDidChange];
}
void NativeLedgerClient::FetchFavIcon(const std::string & url, const std::string & favicon_key, ledger::FetchIconCallback callback) {
  [bridge_ fetchFavIcon:url faviconKey:favicon_key callback:callback];
}
void NativeLedgerClient::FetchGrants(const std::string & lang, const std::string & paymentId) {
  [bridge_ fetchGrants:lang paymentId:paymentId];
}
std::string NativeLedgerClient::GenerateGUID() const {
  return [bridge_ generateGUID];
}
void NativeLedgerClient::GetActivityInfoList(uint32_t start, uint32_t limit, ledger::ActivityInfoFilter filter, ledger::PublisherInfoListCallback callback) {
  [bridge_ getActivityInfoList:start limit:limit filter:filter callback:callback];
}
void NativeLedgerClient::GetCountryCodes(const std::vector<std::string> & countries, ledger::GetCountryCodesCallback callback) {
  [bridge_ getCountryCodes:countries callback:callback];
}
void NativeLedgerClient::GetGrantCaptcha(const std::string & promotion_id, const std::string & promotion_type) {
  [bridge_ getGrantCaptcha:promotion_id promotionType:promotion_type];
}
void NativeLedgerClient::GetOneTimeTips(ledger::PublisherInfoListCallback callback) {
  [bridge_ getOneTimeTips:callback];
}
void NativeLedgerClient::GetPendingContributions(const ledger::PendingContributionInfoListCallback & callback) {
  [bridge_ getPendingContributions:callback];
}
void NativeLedgerClient::GetPendingContributionsTotal(const ledger::PendingContributionsTotalCallback & callback) {
  [bridge_ getPendingContributionsTotal:callback];
}
void NativeLedgerClient::GetRecurringTips(ledger::PublisherInfoListCallback callback) {
  [bridge_ getRecurringTips:callback];
}
void NativeLedgerClient::KillTimer(const uint32_t timer_id) {
  [bridge_ killTimer:timer_id];
}
void NativeLedgerClient::LoadActivityInfo(ledger::ActivityInfoFilter filter, ledger::PublisherInfoCallback callback) {
  [bridge_ loadActivityInfo:filter callback:callback];
}
void NativeLedgerClient::LoadLedgerState(ledger::LedgerCallbackHandler * handler) {
  [bridge_ loadLedgerState:handler];
}
void NativeLedgerClient::LoadMediaPublisherInfo(const std::string & media_key, ledger::PublisherInfoCallback callback) {
  [bridge_ loadMediaPublisherInfo:media_key callback:callback];
}
void NativeLedgerClient::LoadNicewareList(ledger::GetNicewareListCallback callback) {
  [bridge_ loadNicewareList:callback];
}
void NativeLedgerClient::LoadPanelPublisherInfo(ledger::ActivityInfoFilter filter, ledger::PublisherInfoCallback callback) {
  [bridge_ loadPanelPublisherInfo:filter callback:callback];
}
void NativeLedgerClient::LoadPublisherInfo(const std::string & publisher_key, ledger::PublisherInfoCallback callback) {
  [bridge_ loadPublisherInfo:publisher_key callback:callback];
}
void NativeLedgerClient::LoadPublisherList(ledger::LedgerCallbackHandler * handler) {
  [bridge_ loadPublisherList:handler];
}
void NativeLedgerClient::LoadPublisherState(ledger::LedgerCallbackHandler * handler) {
  [bridge_ loadPublisherState:handler];
}
void NativeLedgerClient::LoadState(const std::string & name, ledger::OnLoadCallback callback) {
  [bridge_ loadState:name callback:callback];
}
void NativeLedgerClient::LoadURL(const std::string & url, const std::vector<std::string> & headers, const std::string & content, const std::string & contentType, const ledger::URL_METHOD method, ledger::LoadURLCallback callback) {
  [bridge_ loadURL:url headers:headers content:content contentType:contentType method:method callback:callback];
}
std::unique_ptr<ledger::LogStream> NativeLedgerClient::Log(const char * file, int line, const ledger::LogLevel log_level) const {
  return [bridge_ log:file line:line logLevel:log_level];
}
void NativeLedgerClient::OnExcludedSitesChanged(const std::string & publisher_id, ledger::PUBLISHER_EXCLUDE exclude) {
  [bridge_ onExcludedSitesChanged:publisher_id exclude:exclude];
}
void NativeLedgerClient::OnGrant(ledger::Result result, const ledger::Grant & grant) {
  [bridge_ onGrant:result grant:grant];
}
void NativeLedgerClient::OnGrantCaptcha(const std::string & image, const std::string & hint) {
  [bridge_ onGrantCaptcha:image hint:hint];
}
void NativeLedgerClient::OnGrantFinish(ledger::Result result, const ledger::Grant & grant) {
  [bridge_ onGrantFinish:result grant:grant];
}
void NativeLedgerClient::OnPanelPublisherInfo(ledger::Result result, ledger::PublisherInfoPtr publisher_info, uint64_t windowId) {
  [bridge_ onPanelPublisherInfo:result publisherInfo:std::move(publisher_info) windowId:windowId];
}
void NativeLedgerClient::OnReconcileComplete(ledger::Result result, const std::string & viewing_id, ledger::REWARDS_CATEGORY category, const std::string & probi) {
  [bridge_ onReconcileComplete:result viewingId:viewing_id category:category probi:probi];
}
void NativeLedgerClient::OnRecoverWallet(ledger::Result result, double balance, const std::vector<ledger::Grant> & grants) {
  [bridge_ onRecoverWallet:result balance:balance grants:grants];
}
void NativeLedgerClient::OnRemoveRecurring(const std::string & publisher_key, ledger::RecurringRemoveCallback callback) {
  [bridge_ onRemoveRecurring:publisher_key callback:callback];
}
void NativeLedgerClient::OnRestorePublishers(ledger::OnRestoreCallback callback) {
  [bridge_ onRestorePublishers:callback];
}
void NativeLedgerClient::OnWalletInitialized(ledger::Result result) {
  [bridge_ onWalletInitialized:result];
}
void NativeLedgerClient::OnWalletProperties(ledger::Result result, std::unique_ptr<ledger::WalletInfo> arg1) {
  [bridge_ onWalletProperties:result arg1:std::move(arg1)];
}
void NativeLedgerClient::RemoveAllPendingContributions(const ledger::RemovePendingContributionCallback & callback) {
  [bridge_ removeAllPendingContributions:callback];
}
void NativeLedgerClient::RemovePendingContribution(const std::string & publisher_key, const std::string & viewing_id, uint64_t added_date, const ledger::RemovePendingContributionCallback & callback) {
  [bridge_ removePendingContribution:publisher_key viewingId:viewing_id addedDate:added_date callback:callback];
}
void NativeLedgerClient::ResetState(const std::string & name, ledger::OnResetCallback callback) {
  [bridge_ resetState:name callback:callback];
}
void NativeLedgerClient::SaveActivityInfo(ledger::PublisherInfoPtr publisher_info, ledger::PublisherInfoCallback callback) {
  [bridge_ saveActivityInfo:std::move(publisher_info) callback:callback];
}
void NativeLedgerClient::SaveContributionInfo(const std::string & probi, const int month, const int year, const uint32_t date, const std::string & publisher_key, const ledger::REWARDS_CATEGORY category) {
  [bridge_ saveContributionInfo:probi month:month year:year date:date publisherKey:publisher_key category:category];
}
void NativeLedgerClient::SaveLedgerState(const std::string & ledger_state, ledger::LedgerCallbackHandler * handler) {
  [bridge_ saveLedgerState:ledger_state handler:handler];
}
void NativeLedgerClient::SaveMediaPublisherInfo(const std::string & media_key, const std::string & publisher_id) {
  [bridge_ saveMediaPublisherInfo:media_key publisherId:publisher_id];
}
void NativeLedgerClient::SaveNormalizedPublisherList(ledger::PublisherInfoList normalized_list) {
  [bridge_ saveNormalizedPublisherList:std::move(normalized_list)];
}
void NativeLedgerClient::SavePendingContribution(ledger::PendingContributionList list) {
  [bridge_ savePendingContribution:std::move(list)];
}
void NativeLedgerClient::SavePublisherInfo(ledger::PublisherInfoPtr publisher_info, ledger::PublisherInfoCallback callback) {
  [bridge_ savePublisherInfo:std::move(publisher_info) callback:callback];
}
void NativeLedgerClient::SavePublisherState(const std::string & publisher_state, ledger::LedgerCallbackHandler * handler) {
  [bridge_ savePublisherState:publisher_state handler:handler];
}
void NativeLedgerClient::SavePublishersList(const std::string & publisher_state, ledger::LedgerCallbackHandler * handler) {
  [bridge_ savePublishersList:publisher_state handler:handler];
}
void NativeLedgerClient::SaveState(const std::string & name, const std::string & value, ledger::OnSaveCallback callback) {
  [bridge_ saveState:name value:value callback:callback];
}
void NativeLedgerClient::SetConfirmationsIsReady(const bool is_ready) {
  [bridge_ setConfirmationsIsReady:is_ready];
}
void NativeLedgerClient::SetTimer(uint64_t time_offset, uint32_t * timer_id) {
  [bridge_ setTimer:time_offset timerId:timer_id];
}
std::string NativeLedgerClient::URIEncode(const std::string & value) {
  return [bridge_ URIEncode:value];
}
std::unique_ptr<ledger::LogStream> NativeLedgerClient::VerboseLog(const char * file, int line, int vlog_level) const {
  return [bridge_ verboseLog:file line:line vlogLevel:vlog_level];
}
