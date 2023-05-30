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
    (brave_rewards::mojom::LedgerClient::LoadLedgerStateCallback)callback;
- (void)loadPublisherState:
    (brave_rewards::mojom::LedgerClient::LoadPublisherStateCallback)callback;
- (void)onReconcileComplete:(brave_rewards::mojom::Result)result
               contribution:
                   (brave_rewards::mojom::ContributionInfoPtr)contribution;
- (void)onPanelPublisherInfo:(brave_rewards::mojom::Result)result
               publisherInfo:
                   (brave_rewards::mojom::PublisherInfoPtr)publisherInfo
                    windowId:(uint64_t)windowId;
- (void)fetchFavIcon:(const std::string&)url
          faviconKey:(const std::string&)faviconKey
            callback:(brave_rewards::mojom::LedgerClient::FetchFavIconCallback)
                         callback;
- (void)loadUrl:(brave_rewards::mojom::UrlRequestPtr)request
       callback:(brave_rewards::mojom::LedgerClient::LoadURLCallback)callback;
- (void)publisherListNormalized:
    (std::vector<brave_rewards::mojom::PublisherInfoPtr>)list;
- (void)onPublisherRegistryUpdated;
- (void)onPublisherUpdated:(const std::string&)publisherId;
- (void)booleanState:(const std::string&)name
            callback:
                (brave_rewards::mojom::LedgerClient::GetBooleanStateCallback)
                    callback;
- (void)setBooleanState:(const std::string&)name
                  value:(bool)value
               callback:
                   (brave_rewards::mojom::LedgerClient::SetBooleanStateCallback)
                       callback;
- (void)integerState:(const std::string&)name
            callback:
                (brave_rewards::mojom::LedgerClient::GetIntegerStateCallback)
                    callback;
- (void)setIntegerState:(const std::string&)name
                  value:(int32_t)value
               callback:
                   (brave_rewards::mojom::LedgerClient::SetIntegerStateCallback)
                       callback;
- (void)doubleState:(const std::string&)name
           callback:(brave_rewards::mojom::LedgerClient::GetDoubleStateCallback)
                        callback;
- (void)setDoubleState:(const std::string&)name
                 value:(double)value
              callback:
                  (brave_rewards::mojom::LedgerClient::SetDoubleStateCallback)
                      callback;
- (void)stringState:(const std::string&)name
           callback:(brave_rewards::mojom::LedgerClient::GetStringStateCallback)
                        callback;
- (void)setStringState:(const std::string&)name
                 value:(const std::string&)value
              callback:
                  (brave_rewards::mojom::LedgerClient::SetStringStateCallback)
                      callback;
- (void)int64State:(const std::string&)name
          callback:(brave_rewards::mojom::LedgerClient::GetInt64StateCallback)
                       callback;
- (void)setInt64State:(const std::string&)name
                value:(int64_t)value
             callback:
                 (brave_rewards::mojom::LedgerClient::SetInt64StateCallback)
                     callback;
- (void)uint64State:(const std::string&)name
           callback:(brave_rewards::mojom::LedgerClient::GetUint64StateCallback)
                        callback;
- (void)setUint64State:(const std::string&)name
                 value:(uint64_t)value
              callback:
                  (brave_rewards::mojom::LedgerClient::SetUint64StateCallback)
                      callback;
- (void)valueState:(const std::string&)name
          callback:(brave_rewards::mojom::LedgerClient::GetValueStateCallback)
                       callback;
- (void)setValueState:(const std::string&)name
                value:(base::Value)value
             callback:
                 (brave_rewards::mojom::LedgerClient::SetValueStateCallback)
                     callback;
- (void)timeState:(const std::string&)name
         callback:
             (brave_rewards::mojom::LedgerClient::GetTimeStateCallback)callback;
- (void)setTimeState:(const std::string&)name
               value:(base::Time)value
            callback:(brave_rewards::mojom::LedgerClient::SetTimeStateCallback)
                         callback;
- (void)clearState:(const std::string&)name
          callback:
              (brave_rewards::mojom::LedgerClient::ClearStateCallback)callback;
- (void)isBitFlyerRegion:
    (brave_rewards::mojom::LedgerClient::IsBitFlyerRegionCallback)callback;
- (void)legacyWallet:
    (brave_rewards::mojom::LedgerClient::GetLegacyWalletCallback)callback;
- (void)
    showNotification:(const std::string&)type
                args:(std::vector<std::string>)args
            callback:
                (brave_rewards::mojom::LedgerClient::ShowNotificationCallback)
                    callback;
- (void)clientInfo:
    (brave_rewards::mojom::LedgerClient::GetClientInfoCallback)callback;
- (void)unblindedTokensReady;
- (void)reconcileStampReset;
- (void)
    runDbTransaction:(brave_rewards::mojom::DBTransactionPtr)transaction
            callback:
                (brave_rewards::mojom::LedgerClient::RunDBTransactionCallback)
                    callback;
- (void)log:(const std::string&)file
            line:(int32_t)line
    verboseLevel:(int32_t)verboseLevel
         message:(const std::string&)message;
- (void)clearAllNotifications;
- (void)externalWalletConnected;
- (void)externalWalletLoggedOut;
- (void)externalWalletReconnected;
- (void)deleteLog:
    (brave_rewards::mojom::LedgerClient::DeleteLogCallback)callback;
- (void)encryptString:(const std::string&)value
             callback:
                 (brave_rewards::mojom::LedgerClient::EncryptStringCallback)
                     callback;
- (void)decryptString:(const std::string&)value
             callback:
                 (brave_rewards::mojom::LedgerClient::DecryptStringCallback)
                     callback;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_LEDGER_LEDGER_CLIENT_BRIDGE_H_
