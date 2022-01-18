/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_BRAVE_WALLET_BRAVE_WALLET_PROVIDER_DELEGATE_IOS_H_
#define BRAVE_IOS_BROWSER_API_BRAVE_WALLET_BRAVE_WALLET_PROVIDER_DELEGATE_IOS_H_

#import <Foundation/Foundation.h>
#import "brave_wallet.mojom.objc.h"

NS_ASSUME_NONNULL_BEGIN

typedef void (^BraveWalletProviderResultsCallback)(
    NSArray<NSString*>* values,
    BraveWalletProviderError error,
    NSString* errorMessage);

OBJC_EXPORT
@protocol BraveWalletProviderDelegate
- (void)showPanel;
- (NSURL*)getOrigin;
- (void)requestEthereumPermissions:
    (BraveWalletProviderResultsCallback)completion;
- (void)getAllowedAccounts:(BOOL)includeAccountsWhenLocked
                completion:(BraveWalletProviderResultsCallback)completion;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_BRAVE_WALLET_BRAVE_WALLET_PROVIDER_DELEGATE_IOS_H_
