/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <Foundation/Foundation.h>
#import "bat/ledger/ledger_client.h"

@protocol NativeLedgerClientBridge
@required

- (void)confirmationsTransactionHistoryDidChange;
- (void)fetchFavIcon:(const std::string &)url faviconKey:(const std::string &)favicon_key callback:(ledger::FetchIconCallback)callback;
- (void)killTimer:(const uint32_t)timer_id;
- (void)loadLedgerState:(ledger::OnLoadCallback)callback;
- (void)loadPublisherState:(ledger::OnLoadCallback)callback;
- (void)loadState:(const std::string &)name callback:(ledger::OnLoadCallback)callback;
- (void)loadURL:(const std::string &)url headers:(const std::vector<std::string> &)headers content:(const std::string &)content contentType:(const std::string &)contentType method:(const ledger::UrlMethod)method callback:(ledger::LoadURLCallback)callback;
- (void)log:(const char *)file line:(const int)line verboseLevel:(const int)verbose_level message:(const std::string &) message;
- (void)onPanelPublisherInfo:(ledger::Result)result publisherInfo:(ledger::PublisherInfoPtr)publisher_info windowId:(uint64_t)windowId;
- (void)onReconcileComplete:(ledger::Result)result contribution:(ledger::ContributionInfoPtr)contribution;
- (void)resetState:(const std::string &)name callback:(ledger::ResultCallback)callback;
- (void)publisherListNormalized:(ledger::PublisherInfoList)list;
- (void)saveState:(const std::string &)name value:(const std::string &)value callback:(ledger::ResultCallback)callback;
- (void)setConfirmationsIsReady:(const bool)is_ready;
- (void)setTimer:(uint64_t)time_offset timerId:(uint32_t *)timer_id;
- (std::string)URIEncode:(const std::string &)value;
- (void)onContributeUnverifiedPublishers:(ledger::Result)result publisherKey:(const std::string&)publisher_key publisherName:(const std::string&)publisher_name;
- (void)setBooleanState:(const std::string&)name value:(bool)value;
- (bool)getBooleanState:(const std::string&)name;
- (void)setIntegerState:(const std::string&)name value:(int)value;
- (int)getIntegerState:(const std::string&)name;
- (void)setDoubleState:(const std::string&)name value:(double)value;
- (double)getDoubleState:(const std::string&)name;
- (void)setStringState:(const std::string&)name value:(const std::string&)value;
- (std::string)getStringState:(const std::string&)name;
- (void)setInt64State:(const std::string&)name value:(int64_t)value;
- (int64_t)getInt64State:(const std::string&)name;
- (void)setUint64State:(const std::string&)name value:(uint64_t)value;
- (uint64_t)getUint64State:(const std::string&)name;
- (void)clearState:(const std::string&)name;
- (std::map<std::string, ledger::ExternalWalletPtr>)getExternalWallets;
- (void)saveExternalWallet:(const std::string &)wallet_type wallet:(ledger::ExternalWalletPtr)wallet;
- (void)showNotification:(const std::string &)type args:(const std::vector<std::string>&)args callback:(ledger::ResultCallback)callback;
- (void)setTransferFee:(const std::string&)wallet_type transfer_fee:(ledger::TransferFeePtr)transfer_fee;
- (void)removeTransferFee:(const std::string&)wallet_type id:(const std::string&)id;
- (ledger::TransferFeeList)getTransferFees:(const std::string&)wallet_type;
- (bool)getBooleanOption:(const std::string&)name;
- (int)getIntegerOption:(const std::string&)name;
- (double)getDoubleOption:(const std::string&)name;
- (std::string)getStringOption:(const std::string&)name;
- (int64_t)getInt64Option:(const std::string&)name;
- (uint64_t)getUint64Option:(const std::string&)name;
- (ledger::ClientInfoPtr)getClientInfo;
- (void)unblindedTokensReady;
- (void)reconcileStampReset;
- (void)runDBTransaction:(ledger::DBTransactionPtr)transaction callback:(ledger::RunDBTransactionCallback)callback;
- (void)getCreateScript:(ledger::GetCreateScriptCallback)callback;
- (void)pendingContributionSaved:(const ledger::Result)result;
- (void)clearAllNotifications;
- (void)deleteLog:(ledger::ResultCallback)callback;

@end
