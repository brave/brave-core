/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "brave/ios/browser/api/brave_wallet/keyring_controller_ios.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

#include "base/strings/sys_string_conversions.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/ios/browser/api/brave_wallet/brave_wallet.mojom.objc+private.h"

@interface BraveWalletKeyringController () {
  brave_wallet::mojom::KeyringController* _controller;  // NOT OWNED
}
@end

@implementation BraveWalletKeyringController

- (instancetype)initWithController:
    (brave_wallet::mojom::KeyringController*)controller {
  if ((self = [super init])) {
    _controller = controller;
  }
  return self;
}

- (void)defaultKeyringInfo:(void (^)(BraveWalletKeyringInfo*))completion {
  auto callback = ^(brave_wallet::mojom::KeyringInfoPtr keyring) {
    auto info = [[BraveWalletKeyringInfo alloc]
        initWithKeyringInfoPtr:std::move(keyring)];
    completion(info);
  };
  _controller->GetDefaultKeyringInfo(base::BindOnce(callback));
}

- (void)mnemonicForDefaultKeyring:(void (^)(NSString*))completion {
  auto callback = ^(const std::string& phrase) {
    completion(base::SysUTF8ToNSString(phrase));
  };
  _controller->GetMnemonicForDefaultKeyring(base::BindOnce(callback));
}

- (void)createWalletWithPassword:(NSString*)password
                      completion:(void (^)(NSString*))completion {
  auto callback = ^(const std::string& phrase) {
    completion(base::SysUTF8ToNSString(phrase));
  };
  _controller->CreateWallet(base::SysNSStringToUTF8(password),
                            base::BindOnce(callback));
}

- (void)restoreWalletWithMnemonic:(NSString*)mnemonic
                         password:(NSString*)password
                       completion:(void (^)(bool))completion {
  auto callback = ^(bool isValid) {
    completion(isValid);
  };
  _controller->RestoreWallet(base::SysNSStringToUTF8(mnemonic),
                             base::SysNSStringToUTF8(password),
                             base::BindOnce(callback));
}

- (void)lock {
  _controller->Lock();
}

- (void)unlockWithPassword:(NSString*)password
                completion:(void (^)(bool))completion {
  auto callback = ^(bool isUnlocked) {
    completion(isUnlocked);
  };
  _controller->Unlock(base::SysNSStringToUTF8(password),
                      base::BindOnce(callback));
}

- (void)isWalletBackedUp:(void (^)(bool))completion {
  auto callback = ^(bool isBackedUp) {
    completion(isBackedUp);
  };
  _controller->IsWalletBackedUp(base::BindOnce(callback));
}

- (void)notifyWalletBackupComplete {
  _controller->NotifyWalletBackupComplete();
}

- (void)addAccount:(void (^)(bool))completion {
  auto callback = ^(bool success) {
    completion(success);
  };
  _controller->AddAccount(base::BindOnce(callback));
}

- (void)addNewAccountName:(NSString*)name {
  _controller->AddNewAccountName(base::SysNSStringToUTF8(name));
}

- (void)setInitialAccountNames:(NSArray<NSString*>*)names {
  std::vector<std::string> _names;
  for (NSString* name in names) {
    _names.push_back(base::SysNSStringToUTF8(name));
  }
  _controller->SetInitialAccountNames(_names);
}

- (void)reset {
  _controller->Reset();
}

@end
