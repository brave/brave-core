/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_LEDGER_LEDGER_CLIENT_BRIDGE_H_
#define BRAVE_IOS_BROWSER_API_LEDGER_LEDGER_CLIENT_BRIDGE_H_

#import <Foundation/Foundation.h>
#include <string>
#include <vector>
#include "brave/components/brave_rewards/common/mojom/bat_ledger.mojom.h"

NS_ASSUME_NONNULL_BEGIN

@protocol LedgerClientBridge
@required

- (void)loadLedgerState:
    (ledger::mojom::LedgerClient::LoadLedgerStateCallback)callback;
- (void)loadPublisherState:
    (ledger::mojom::LedgerClient::LoadPublisherStateCallback)callback;
- (void)onReconcileComplete:(ledger::mojom::Result)result
               contribution:(ledger::mojom::ContributionInfoPtr)contribution;
- (void)onPanelPublisherInfo:(ledger::mojom::Result)result
               publisherInfo:(ledger::mojom::PublisherInfoPtr)publisherInfo
                    windowId:(uint64_t)windowId;
- (void)fetchFavIcon:(const std::string&)url
          faviconKey:(const std::string&)faviconKey
            callback:
                (ledger::mojom::LedgerClient::FetchFavIconCallback)callback;
- (void)loadUrl:(ledger::mojom::UrlRequestPtr)request
       callback:(ledger::mojom::LedgerClient::LoadURLCallback)callback;
- (void)uriEncode:(const std::string&)value
         callback:(ledger::mojom::LedgerClient::URIEncodeCallback)callback;
- (void)publisherListNormalized:
    (std::vector<ledger::mojom::PublisherInfoPtr>)list;
- (void)onPublisherRegistryUpdated;
- (void)onPublisherUpdated:(const std::string&)publisherId;
- (void)booleanState:(const std::string&)name
            callback:
                (ledger::mojom::LedgerClient::GetBooleanStateCallback)callback;
- (void)setBooleanState:(const std::string&)name
                  value:(bool)value
               callback:(ledger::mojom::LedgerClient::SetBooleanStateCallback)
                            callback;
- (void)integerState:(const std::string&)name
            callback:
                (ledger::mojom::LedgerClient::GetIntegerStateCallback)callback;
- (void)setIntegerState:(const std::string&)name
                  value:(int32_t)value
               callback:(ledger::mojom::LedgerClient::SetIntegerStateCallback)
                            callback;
- (void)doubleState:(const std::string&)name
           callback:
               (ledger::mojom::LedgerClient::GetDoubleStateCallback)callback;
- (void)setDoubleState:(const std::string&)name
                 value:(double)value
              callback:
                  (ledger::mojom::LedgerClient::SetDoubleStateCallback)callback;
- (void)stringState:(const std::string&)name
           callback:
               (ledger::mojom::LedgerClient::GetStringStateCallback)callback;
- (void)setStringState:(const std::string&)name
                 value:(const std::string&)value
              callback:
                  (ledger::mojom::LedgerClient::SetStringStateCallback)callback;
- (void)int64State:(const std::string&)name
          callback:(ledger::mojom::LedgerClient::GetInt64StateCallback)callback;
- (void)setInt64State:(const std::string&)name
                value:(int64_t)value
             callback:
                 (ledger::mojom::LedgerClient::SetInt64StateCallback)callback;
- (void)uint64State:(const std::string&)name
           callback:
               (ledger::mojom::LedgerClient::GetUint64StateCallback)callback;
- (void)setUint64State:(const std::string&)name
                 value:(uint64_t)value
              callback:
                  (ledger::mojom::LedgerClient::SetUint64StateCallback)callback;
- (void)valueState:(const std::string&)name
          callback:(ledger::mojom::LedgerClient::GetValueStateCallback)callback;
- (void)setValueState:(const std::string&)name
                value:(base::Value)value
             callback:
                 (ledger::mojom::LedgerClient::SetValueStateCallback)callback;
- (void)timeState:(const std::string&)name
         callback:(ledger::mojom::LedgerClient::GetTimeStateCallback)callback;
- (void)setTimeState:(const std::string&)name
               value:(base::Time)value
            callback:
                (ledger::mojom::LedgerClient::SetTimeStateCallback)callback;
- (void)clearState:(const std::string&)name
          callback:(ledger::mojom::LedgerClient::ClearStateCallback)callback;
- (void)booleanOption:(const std::string&)name
             callback:(ledger::mojom::LedgerClient::GetBooleanOptionCallback)
                          callback;
- (void)integerOption:(const std::string&)name
             callback:(ledger::mojom::LedgerClient::GetIntegerOptionCallback)
                          callback;
- (void)doubleOption:(const std::string&)name
            callback:
                (ledger::mojom::LedgerClient::GetDoubleOptionCallback)callback;
- (void)stringOption:(const std::string&)name
            callback:
                (ledger::mojom::LedgerClient::GetStringOptionCallback)callback;
- (void)int64Option:(const std::string&)name
           callback:
               (ledger::mojom::LedgerClient::GetInt64OptionCallback)callback;
- (void)uint64Option:(const std::string&)name
            callback:
                (ledger::mojom::LedgerClient::GetUint64OptionCallback)callback;
- (void)onContributeUnverifiedPublishers:(ledger::mojom::Result)result
                            publisherKey:(const std::string&)publisherKey
                           publisherName:(const std::string&)publisherName;
- (void)legacyWallet:
    (ledger::mojom::LedgerClient::GetLegacyWalletCallback)callback;
- (void)showNotification:(const std::string&)type
                    args:(std::vector<std::string>)args
                callback:(ledger::mojom::LedgerClient::ShowNotificationCallback)
                             callback;
- (void)clientInfo:(ledger::mojom::LedgerClient::GetClientInfoCallback)callback;
- (void)unblindedTokensReady;
- (void)reconcileStampReset;
- (void)runDbTransaction:(ledger::mojom::DBTransactionPtr)transaction
                callback:(ledger::mojom::LedgerClient::RunDBTransactionCallback)
                             callback;
- (void)createScript:
    (ledger::mojom::LedgerClient::GetCreateScriptCallback)callback;
- (void)pendingContributionSaved:(ledger::mojom::Result)result;
- (void)log:(const std::string&)file
            line:(int32_t)line
    verboseLevel:(int32_t)verboseLevel
         message:(const std::string&)message;
- (void)clearAllNotifications;
- (void)externalWalletConnected;
- (void)externalWalletLoggedOut;
- (void)externalWalletReconnected;
- (void)deleteLog:(ledger::mojom::LedgerClient::DeleteLogCallback)callback;
- (void)encryptString:(const std::string&)value
             callback:
                 (ledger::mojom::LedgerClient::EncryptStringCallback)callback;
- (void)decryptString:(const std::string&)value
             callback:
                 (ledger::mojom::LedgerClient::DecryptStringCallback)callback;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_LEDGER_LEDGER_CLIENT_BRIDGE_H_
