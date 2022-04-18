/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_BRAVE_WALLET_BRAVE_WALLET_PROVIDER_DELEGATE_IOS_H_
#define BRAVE_IOS_BROWSER_API_BRAVE_WALLET_BRAVE_WALLET_PROVIDER_DELEGATE_IOS_H_

#import <Foundation/Foundation.h>
#import "brave_wallet.mojom.objc.h"

@class URLOriginIOS;

NS_ASSUME_NONNULL_BEGIN

typedef void (^BraveWalletProviderResultsCallback)(
    NSArray<NSString*>* values,
    BraveWalletProviderError error,
    NSString* errorMessage);

typedef void (^RequestSolanaPermissionCallback)(
    NSString* _Nullable account,
    BraveWalletSolanaProviderError error,
    NSString* errorMessage);

typedef void (^IsSelectedAccountAllowedCallback)(NSString* _Nullable account,
                                                 bool allowed);

OBJC_EXPORT
@protocol BraveWalletProviderDelegate
- (void)showPanel;
- (URLOriginIOS*)getOrigin;
- (void)walletInteractionDetected;
- (void)requestEthereumPermissions:
    (BraveWalletProviderResultsCallback)completion;
- (void)getAllowedAccounts:(BraveWalletCoinType)type
    includeAccountsWhenLocked:(BOOL)includeAccountsWhenLocked
                   completion:(BraveWalletProviderResultsCallback)completion;
- (void)requestSolanaPermission:(RequestSolanaPermissionCallback)completion;
- (void)isSelectedAccountAllowed:(BraveWalletCoinType)type
                      completion:(IsSelectedAccountAllowedCallback)completion;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_BRAVE_WALLET_BRAVE_WALLET_PROVIDER_DELEGATE_IOS_H_
