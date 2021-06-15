/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_WALLET_ETH_JSON_RPC_CONTROLLER_H_
#define BRAVE_IOS_BROWSER_API_WALLET_ETH_JSON_RPC_CONTROLLER_H_

#import <Foundation/Foundation.h>

typedef NS_ENUM(NSInteger, WalletNetwork) {
  WalletNetworkMainnet,
  WalletNetworkRinkeby,
  WalletNetworkRopsten,
  WalletNetworkGoerli,
  WalletNetworkKovan,
  WalletNetworkLocalhost,
  WalletNetworkCustom
};

OBJC_EXPORT
@interface ETHJSONRPCController : NSObject

@property(class, nonatomic, readonly)
    ETHJSONRPCController* sharedController NS_SWIFT_NAME(shared);

- (void)
    startRequestWithJSONPayload:(NSString*)payload
       autoRetryOnNetworkChange:(BOOL)autoRetryOnNetworkChange
                     completion:
                         (void (^)(int statusCode,
                                   NSString* response,
                                   NSDictionary<NSString*, NSString*>* headers))
                             completion;

- (void)balanceForAddress:(NSString*)address
               completion:(void (^)(bool status, NSString* balance))completion;

@property(nonatomic) WalletNetwork network;

- (instancetype)init NS_UNAVAILABLE;

@end

#endif  // BRAVE_IOS_BROWSER_API_WALLET_ETH_JSON_RPC_CONTROLLER_H_
