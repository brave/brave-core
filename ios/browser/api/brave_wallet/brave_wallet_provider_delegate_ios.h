/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_BRAVE_WALLET_BRAVE_WALLET_PROVIDER_DELEGATE_IOS_H_
#define BRAVE_IOS_BROWSER_API_BRAVE_WALLET_BRAVE_WALLET_PROVIDER_DELEGATE_IOS_H_

#import <Foundation/Foundation.h>
#import "brave_wallet.mojom.objc.h"

@class URLOriginIOS;

NS_ASSUME_NONNULL_BEGIN

typedef void (^RequestPermissionsCallback)(
    BraveWalletRequestPermissionsError error,
    NSArray<NSString*>* _Nullable allowedAccounts);

OBJC_EXPORT
@protocol BraveWalletProviderDelegate
- (bool)isTabVisible;
- (void)showPanel;
- (URLOriginIOS*)getOrigin;
- (void)walletInteractionDetected;
- (void)showWalletOnboarding;
- (void)showWalletBackup;
- (void)unlockWallet;
- (void)showAccountCreation:(BraveWalletCoinType)type;
- (void)requestPermissions:(BraveWalletCoinType)type
                  accounts:(NSArray<NSString*>*)accounts
                completion:(RequestPermissionsCallback)completion;
- (bool)isAccountAllowed:(BraveWalletCoinType)type account:(NSString*)account;
- (nullable NSArray<NSString*>*)getAllowedAccounts:(BraveWalletCoinType)type
                                          accounts:
                                              (NSArray<NSString*>*)accounts;
- (bool)isPermissionDenied:(BraveWalletCoinType)type;
- (void)addSolanaConnectedAccount:(NSString*)account;
- (void)removeSolanaConnectedAccount:(NSString*)account;
- (bool)isSolanaAccountConnected:(NSString*)account;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_BRAVE_WALLET_BRAVE_WALLET_PROVIDER_DELEGATE_IOS_H_
