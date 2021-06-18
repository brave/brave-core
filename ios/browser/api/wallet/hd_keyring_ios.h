/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_WALLET_HD_KEYRING_IOS_H_
#define BRAVE_IOS_BROWSER_API_WALLET_HD_KEYRING_IOS_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, HDKeyringType) {
  HDKeyringTypeDefault = 0,
  HDKeyringTypeLedger,
  HDKeyringTypeTrezor,
  HDKeyringTypeBitcoin
};

OBJC_EXPORT
NS_SWIFT_NAME(HDKeyring)
@interface HDKeyringIOS : NSObject

@property(nonatomic, readonly) HDKeyringType type;
@property(nonatomic, readonly) bool isEmpty;

- (void)clearData;

- (void)addAccount;
- (void)addAccounts:(NSUInteger)numberOfAcconts;
- (void)removeAccountWithAddress:(NSString*)address
    NS_SWIFT_NAME(removeAccount(address:));

@property(nonatomic, readonly) NSArray<NSString*>* accountAddresses;
- (NSString*)addressAtIndex:(NSUInteger)index;

- (NSData*)signedMessageForAddress:(NSString*)address
                           message:(NSData*)message
    NS_SWIFT_NAME(signedMessage(address:message:));

- (instancetype)init NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_WALLET_HD_KEYRING_IOS_H_
