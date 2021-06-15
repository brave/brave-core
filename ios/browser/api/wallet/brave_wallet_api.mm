/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "brave/ios/browser/api/wallet/brave_wallet_api.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

#import "brave/ios/browser/api/wallet/brave_wallet_service_factory.h"
#import "brave/ios/browser/api/wallet/keyring_controller_ios+private.h"

#include "base/strings/sys_string_conversions.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/keyring_controller.h"

@interface BraveWalletAPI () {
  brave_wallet::BraveWalletService* _service;  // NOT OWNED
}
@property(nonatomic, readwrite) KeyringControllerIOS* keyringController;
@end

@implementation BraveWalletAPI

- (instancetype)initWithWalletService:
    (brave_wallet::BraveWalletService*)service {
  if ((self = [super init])) {
    _service = service;
  }
  return self;
}

- (KeyringControllerIOS*)keyringController {
  if (!_keyringController) {
    _keyringController = [[KeyringControllerIOS alloc]
        initWithController:_service->keyring_controller()];
  }
  return _keyringController;
}

@end
