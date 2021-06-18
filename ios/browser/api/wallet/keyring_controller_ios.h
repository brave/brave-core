/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_WALLET_KEYRING_CONTROLLER_IOS_H_
#define BRAVE_IOS_BROWSER_API_WALLET_KEYRING_CONTROLLER_IOS_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@class HDKeyringIOS;

OBJC_EXPORT
NS_SWIFT_NAME(KeyringController)
@interface KeyringControllerIOS : NSObject

#pragma mark - Default Keyring

/// The current default keyring
///
/// @warning Must unlock before using this API otherwise it will be nil
@property(nullable, nonatomic, readonly) HDKeyringIOS* defaultKeyring;

/// The current keyrings mnemonic string
///
/// @warning: Must unlock before using this API otherwise it will be an empty
/// string
@property(nonatomic, readonly) NSString* mnemonicForDefaultKeyring;

/// Whether or not the default keyring has been created
@property(nonatomic, readonly) bool isDefaultKeyringCreated;

#pragma mark - Keyring Creation/Restore

/// Creates a new default keyring with a password
///
/// @warning `KeyringController` currently only supports one default keyring,
/// calling this
///           while `defaultKeyringCreated` returns true will overwrite the
///           default keyring.
- (nullable HDKeyringIOS*)createDefaultKeyringWithPassword:(NSString*)password
    NS_SWIFT_NAME(createDefaultKeyring(password:));

/// Restores a keyring using a previous keyrings mneomic string and sets a new
/// password
///
/// @warning `KeyringController` currently only supports one default keyring,
/// calling this
///           while `defaultKeyringCreated` returns true will overwrite the
///           default keyring.
- (nullable HDKeyringIOS*)restoreDefaultKeyringWithMneomic:(NSString*)mnemonic
                                                  password:(NSString*)password
    NS_SWIFT_NAME(restoreDefaultKeyring(mnemonic:password:));

#pragma mark - Lock/Unlock

/// Whether or not the keyring is currently locked
@property(nonatomic, readonly) bool isLocked;

/// Locks the keyring
- (void)lock;

/// Unlock the keyring given some password
///
/// @return: `true` if the password is correct and the wallet unlocked, `false`
/// otherwise.
- (bool)unlockWithPassword:(NSString*)password NS_SWIFT_NAME(unlock(password:));

#pragma mark -

/// Resets the current keyring
- (void)reset;

- (instancetype)init NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_WALLET_KEYRING_CONTROLLER_IOS_H_
