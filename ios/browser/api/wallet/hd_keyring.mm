/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "brave/ios/browser/api/wallet/hd_keyring.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

#import "brave/ios/browser/api/wallet/brave_wallet_service_factory.h"

#include "base/strings/sys_string_conversions.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/hd_keyring.h"

@interface HDKeyring () {
  brave_wallet::HDKeyring* _keyring;  // NOT OWNED
}
@end

@implementation HDKeyring

- (instancetype)initWithKeyring:(brave_wallet::HDKeyring*)keyring {
  if ((self = [super init])) {
    _keyring = keyring;
  }
  return self;
}

- (HDKeyringType)type {
  return static_cast<HDKeyringType>(_keyring->type());
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
  uint8_t* msg = (uint8_t*)(message.bytes);
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

@end
