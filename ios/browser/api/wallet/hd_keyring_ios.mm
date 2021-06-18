/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "brave/ios/browser/api/wallet/hd_keyring_ios.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

#include "base/notreached.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/hd_keyring.h"

@interface HDKeyringIOS () {
  brave_wallet::HDKeyring* _keyring;  // NOT OWNED
}
@end

@implementation HDKeyringIOS

- (instancetype)initWithKeyring:(brave_wallet::HDKeyring*)keyring {
  if ((self = [super init])) {
    _keyring = keyring;
  }
  return self;
}

- (HDKeyringType)type {
  return HDKeyringTypeForBraveWalletHDKeyringType(_keyring->type());
}

- (bool)isEmpty {
  return _keyring->empty();
}

- (void)clearData {
  _keyring->ClearData();
}

- (void)addAccount {
  [self addAccounts:1];
}

- (void)addAccounts:(NSUInteger)numberOfAcconts {
  _keyring->AddAccounts(static_cast<size_t>(numberOfAcconts));
}

- (void)removeAccountWithAddress:(NSString*)address {
  _keyring->RemoveAccount(base::SysNSStringToUTF8(address));
}

- (NSData*)signedMessageForAddress:(NSString*)address message:(NSData*)message {
  std::vector<uint8_t> bridgedMessage;
  const uint8_t* msg = static_cast<const uint8_t*>(message.bytes);
  bridgedMessage.assign(msg, msg + message.length);
  const auto signedMessage =
      _keyring->SignMessage(base::SysNSStringToUTF8(address), bridgedMessage);
  return [NSData dataWithBytes:signedMessage.data()
                        length:signedMessage.size()];
}

- (NSArray<NSString*>*)accountAddresses {
  auto addresses = _keyring->GetAccounts();
  auto bridgedAddresses = [[NSMutableArray alloc] init];
  for (const auto& address : addresses) {
    [bridgedAddresses addObject:base::SysUTF8ToNSString(address)];
  }
  return [bridgedAddresses copy];
}

- (NSString*)addressAtIndex:(NSUInteger)index {
  auto address = _keyring->GetAddress(static_cast<size_t>(index));
  return base::SysUTF8ToNSString(address);
}

#pragma mark - Util

static HDKeyringType HDKeyringTypeForBraveWalletHDKeyringType(
    brave_wallet::HDKeyring::Type type) {
  switch (type) {
    case brave_wallet::HDKeyring::Type::kDefault:
      return HDKeyringTypeDefault;
    case brave_wallet::HDKeyring::Type::kLedger:
      return HDKeyringTypeLedger;
    case brave_wallet::HDKeyring::Type::kTrezor:
      return HDKeyringTypeTrezor;
    case brave_wallet::HDKeyring::Type::kBitcoin:
      return HDKeyringTypeBitcoin;
    default:
      NOTREACHED();
  }
}

@end
