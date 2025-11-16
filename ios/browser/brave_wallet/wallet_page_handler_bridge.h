// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_BRAVE_WALLET_WALLET_PAGE_HANDLER_BRIDGE_H_
#define BRAVE_IOS_BROWSER_BRAVE_WALLET_WALLET_PAGE_HANDLER_BRIDGE_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

/// Browser-side handler for general Brave Wallet UI functions
///
/// This is an Obj-C bridge for the PageHandler mojom
/// interface (see brave_wallet.mojom) and only bridges methods that will be
/// called in iOS/mobile
NS_SWIFT_NAME(WalletPageHandler)
@protocol WalletPageHandlerBridge
@required

- (void)showApprovePanelUI;
- (void)showWalletBackupUI;
- (void)unlockWalletUI;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_BRAVE_WALLET_WALLET_PAGE_HANDLER_BRIDGE_H_
