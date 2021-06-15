/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "brave/ios/browser/api/wallet/keyring_controller.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

#import "brave/ios/browser/api/wallet/brave_wallet_service_factory.h"
#import "brave/ios/browser/api/wallet/hd_keyring.h"
#import "brave/ios/browser/api/wallet/hd_keyring_private.h"

#include "base/strings/sys_string_conversions.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/hd_keyring.h"
#include "brave/components/brave_wallet/browser/keyring_controller.h"
#include "ios/chrome/browser/application_context.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state_manager.h"

@interface KeyringController () {
  brave_wallet::KeyringController* _controller;  // NOT OWNED
}
@end

@implementation KeyringController

+ (instancetype)sharedController {
  // get from BraveWalletService keyring_controller
  auto* state = GetApplicationContext()
                    ->GetChromeBrowserStateManager()
                    ->GetLastUsedBrowserState();
  auto* controller = BraveWalletServiceFactory::GetForBrowserState(state)
                         ->keyring_controller();
  return [[KeyringController alloc] initWithController:controller];
}

- (instancetype)initWithController:
    (brave_wallet::KeyringController*)controller {
  if ((self = [super init])) {
    _controller = controller;
  }
  return self;
}

- (nullable HDKeyring*)defaultKeyring {
  auto* keyring = _controller->GetDefaultKeyring();
  if (keyring == nullptr) {
    return nil;
  }
  return [[HDKeyring alloc] initWithKeyring:keyring];
}

- (NSString*)mnemonicForDefaultKeyring {
  auto mnemonicString = _controller->GetMnemonicForDefaultKeyring();
  return base::SysUTF8ToNSString(mnemonicString);
}

- (bool)isDefaultKeyringCreated {
  return _controller->IsDefaultKeyringCreated();
}

- (nullable HDKeyring*)createDefaultKeyringWithPassword:(NSString*)password {
  auto* keyring =
      _controller->CreateDefaultKeyring(base::SysNSStringToUTF8(password));
  if (keyring == nullptr) {
    return nil;
  }
  return [[HDKeyring alloc] initWithKeyring:keyring];
}

- (nullable HDKeyring*)restoreDefaultKeyringWithMneomic:(NSString*)mnemonic
                                               password:(NSString*)password {
  auto* keyring = _controller->RestoreDefaultKeyring(
      base::SysNSStringToUTF8(mnemonic), base::SysNSStringToUTF8(password));
  if (keyring == nullptr) {
    return nil;
  }
  return [[HDKeyring alloc] initWithKeyring:keyring];
}

- (bool)isLocked {
  return _controller->IsLocked();
}

- (void)lock {
  _controller->Lock();
}

- (bool)unlockWithPassword:(NSString*)password {
  return _controller->Unlock(base::SysNSStringToUTF8(password));
}

- (void)reset {
  _controller->Reset();
}

@end
