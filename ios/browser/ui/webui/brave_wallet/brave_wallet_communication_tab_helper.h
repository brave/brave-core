// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_UI_WEBUI_BRAVE_WALLET_BRAVE_WALLET_COMMUNICATION_TAB_HELPER_H_
#define BRAVE_IOS_BROWSER_UI_WEBUI_BRAVE_WALLET_BRAVE_WALLET_COMMUNICATION_TAB_HELPER_H_

#include <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@class BraveWalletCommunicationController;

// Any Swift methods that WebUI needs to call, to this protocol
OBJC_EXPORT
@protocol BraveWalletCommunicationProtocol
- (void)webUIShowWalletApprovePanelUI;
- (void)webUIShowWalletBackupUI;
- (void)webUIUnlockWallet;
- (void)webUIShowOnboarding:(BOOL)isNewAccount;
@end

OBJC_EXPORT
@interface BraveWalletCommunicationController : NSObject
@property(nonatomic, weak, nullable) id<BraveWalletCommunicationProtocol>
    delegate;

// Any methods swift want to call on the WebUI, goes here
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_UI_WEBUI_BRAVE_WALLET_BRAVE_WALLET_COMMUNICATION_TAB_HELPER_H_
