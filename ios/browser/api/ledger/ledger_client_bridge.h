/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_LEDGER_LEDGER_CLIENT_BRIDGE_H_
#define BRAVE_IOS_BROWSER_API_LEDGER_LEDGER_CLIENT_BRIDGE_H_

#import <Foundation/Foundation.h>
#include <string>
#include <vector>
#include "bat/ledger/ledger_client.h"

@protocol LedgerClientBridge
@required

- (void)fetchFavIcon:(const std::string&)url
          faviconKey:(const std::string&)favicon_key
            callback:(ledger::client::FetchIconCallback)callback;
- (void)loadLedgerState:(ledger::client::OnLoadCallback)callback;
- (void)loadPublisherState:(ledger::client::OnLoadCallback)callback;
- (void)loadURL:(ledger::mojom::UrlRequestPtr)request
       callback:(ledger::client::LoadURLCallback)callback;
- (void)log:(const char*)file
            line:(const int)line
    verboseLevel:(const int)verbose_level
         message:(const std::string&)message;
- (void)onPanelPublisherInfo:(ledger::mojom::Result)result
               publisherInfo:(ledger::mojom::PublisherInfoPtr)publisher_info
                    windowId:(uint64_t)windowId;
- (void)onReconcileComplete:(ledger::mojom::Result)result
               contribution:(ledger::mojom::ContributionInfoPtr)contribution;
- (void)publisherListNormalized:
    (std::vector<ledger::mojom::PublisherInfoPtr>)list;
- (std::string)URIEncode:(const std::string&)value;
- (void)onContributeUnverifiedPublishers:(ledger::mojom::Result)result
                            publisherKey:(const std::string&)publisher_key
                           publisherName:(const std::string&)publisher_name;
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
- (void)setValueState:(const std::string&)name value:(base::Value)value;
- (base::Value)getValueState:(const std::string&)name;
- (void)setTimeState:(const std::string&)name time:(base::Time)time;
- (base::Time)getTimeState:(const std::string&)name;
- (void)clearState:(const std::string&)name;
- (std::string)getLegacyWallet;
- (void)showNotification:(const std::string&)type
                    args:(const std::vector<std::string>&)args
                callback:(ledger::client::LegacyResultCallback)callback;
- (bool)getBooleanOption:(const std::string&)name;
- (int)getIntegerOption:(const std::string&)name;
- (double)getDoubleOption:(const std::string&)name;
- (std::string)getStringOption:(const std::string&)name;
- (int64_t)getInt64Option:(const std::string&)name;
- (uint64_t)getUint64Option:(const std::string&)name;
- (ledger::mojom::ClientInfoPtr)getClientInfo;
- (void)unblindedTokensReady;
- (void)reconcileStampReset;
- (void)runDBTransaction:(ledger::mojom::DBTransactionPtr)transaction
                callback:(ledger::client::RunDBTransactionCallback)callback;
- (void)getCreateScript:(ledger::client::GetCreateScriptCallback)callback;
- (void)pendingContributionSaved:(const ledger::mojom::Result)result;
- (void)clearAllNotifications;
// TODO(zenparsing): This method is no longer called and should be removed.
- (void)walletDisconnected:(const std::string&)wallet_type;
- (void)deleteLog:(ledger::client::LegacyResultCallback)callback;
- (absl::optional<std::string>)encryptString:(const std::string&)value;
- (absl::optional<std::string>)decryptString:(const std::string&)value;

@end

#endif  // BRAVE_IOS_BROWSER_API_LEDGER_LEDGER_CLIENT_BRIDGE_H_
