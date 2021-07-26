/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_BRAVE_WALLET_KEYRING_CONTROLLER_IOS_H_
#define BRAVE_IOS_BROWSER_API_BRAVE_WALLET_KEYRING_CONTROLLER_IOS_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@class BraveWalletKeyringInfo;

OBJC_EXPORT
NS_SWIFT_NAME(BraveWallet.KeyringController)
@interface BraveWalletKeyringController : NSObject

#pragma mark - Default Keyring

/// The current default keyring
- (void)defaultKeyringInfo:
    (void (^)(BraveWalletKeyringInfo* keyring))completion;

/// The current keyrings mnemonic string
///
/// @warning: Must unlock before using this API otherwise it will be an empty
/// string
- (void)mnemonicForDefaultKeyring:(void (^)(NSString*))completion
    NS_SWIFT_NAME(mnemonicForDefaultKeyring(_:));

#pragma mark - Keyring Creation/Restore

/// Creates a new default keyring with a password
///
/// @warning `KeyringController` currently only supports one default keyring,
/// calling this
///           while `defaultKeyringCreated` returns true will overwrite the
///           default keyring.
- (void)createWalletWithPassword:(NSString*)password
                      completion:(void (^)(NSString* mnemonic))completion
    NS_SWIFT_NAME(createWallet(password:_:));

/// Restores a keyring using a previous keyrings mneomic string and sets a new
/// password
- (void)restoreWalletWithMnemonic:(NSString*)mnemonic
                         password:(NSString*)password
                       completion:(void (^)(bool isValidMnemonic))completion
    NS_SWIFT_NAME(restoreWallet(mnemonic:password:_:));

#pragma mark - Lock/Unlock

/// Locks the keyring
- (void)lock;

/// Unlock the keyring given some password
///
/// @return: `true` if the password is correct and the wallet unlocked, `false`
/// otherwise.
- (void)unlockWithPassword:(NSString*)password
                completion:(void (^)(bool unlocked))completion
    NS_SWIFT_NAME(unlock(password:_:));

#pragma mark - Backup

/// Whether or not this wallet has been backed up by the user
- (void)isWalletBackedUp:(void (^)(bool isBackedUp))completion;

/// Notifies that the user has confirmed that they've backed up their wallet
- (void)notifyWalletBackupComplete;

#pragma mark - Accounts

- (void)addAccount:(void (^)(bool success))completion;

- (void)addNewAccountName:(NSString*)name;

- (void)setInitialAccountNames:(NSArray<NSString*>*)names;

#pragma mark -

/// Resets the current keyring
- (void)reset;

- (instancetype)init NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_BRAVE_WALLET_KEYRING_CONTROLLER_IOS_H_
