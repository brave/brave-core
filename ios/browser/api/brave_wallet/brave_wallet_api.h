/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_BRAVE_WALLET_BRAVE_WALLET_API_H_
#define BRAVE_IOS_BROWSER_API_BRAVE_WALLET_BRAVE_WALLET_API_H_

#import <Foundation/Foundation.h>

@protocol BraveWalletBlockchainRegistry;
@protocol BraveWalletEthereumProvider;
@protocol BraveWalletProviderDelegate;
@protocol BraveWalletSolanaProvider;

typedef NS_ENUM(NSInteger, BraveWalletCoinType);

NS_ASSUME_NONNULL_BEGIN

typedef NSString* BraveWalletProviderScriptKey NS_STRING_ENUM;
OBJC_EXPORT BraveWalletProviderScriptKey const
    BraveWalletProviderScriptKeyEthereum;
OBJC_EXPORT BraveWalletProviderScriptKey const
    BraveWalletProviderScriptKeySolana;
OBJC_EXPORT BraveWalletProviderScriptKey const
    BraveWalletProviderScriptKeySolanaWeb3;
OBJC_EXPORT BraveWalletProviderScriptKey const
    BraveWalletProviderScriptKeyWalletStandard;

OBJC_EXPORT
@interface BraveWalletAPI : NSObject

@property(class, readonly) id<BraveWalletBlockchainRegistry> blockchainRegistry;

- (nullable id<BraveWalletEthereumProvider>)
    ethereumProviderWithDelegate:(id<BraveWalletProviderDelegate>)delegate
               isPrivateBrowsing:(bool)isPrivateBrowsing;

- (nullable id<BraveWalletSolanaProvider>)
    solanaProviderWithDelegate:(id<BraveWalletProviderDelegate>)delegate
             isPrivateBrowsing:(bool)isPrivateBrowsing;

- (NSDictionary<BraveWalletProviderScriptKey, NSString*>*)
    providerScriptsForCoinType:(BraveWalletCoinType)coinType;

- (instancetype)init NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_BRAVE_WALLET_BRAVE_WALLET_API_H_
