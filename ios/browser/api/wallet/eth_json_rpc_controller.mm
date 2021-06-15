/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "brave/ios/browser/api/wallet/eth_json_rpc_controller.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

#import "brave/ios/browser/api/wallet/brave_wallet_service_factory.h"

#include "base/bind.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/eth_json_rpc_controller.h"
#include "ios/chrome/browser/application_context.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state_manager.h"

@interface ETHJSONRPCController () {
  brave_wallet::EthJsonRpcController* _controller;  // NOT OWNED
}
@end

@implementation ETHJSONRPCController

+ (instancetype)sharedController {
  // get from BraveWalletService rpc_controller
  auto* state = GetApplicationContext()
                    ->GetChromeBrowserStateManager()
                    ->GetLastUsedBrowserState();
  auto* controller =
      BraveWalletServiceFactory::GetForBrowserState(state)->rpc_controller();
  return [[ETHJSONRPCController alloc] initWithController:controller];
}

- (instancetype)initWithController:
    (brave_wallet::EthJsonRpcController*)controller {
  if ((self = [super init])) {
    _controller = controller;
  }
  return self;
}

- (void)
    startRequestWithJSONPayload:(NSString*)payload
       autoRetryOnNetworkChange:(BOOL)autoRetryOnNetworkChange
                     completion:
                         (void (^)(int statusCode,
                                   NSString* response,
                                   NSDictionary<NSString*, NSString*>* headers))
                             completion {
  _controller->Request(
      base::SysNSStringToUTF8(payload),
      base::BindOnce(^(const int statusCode, const std::string& response,
                       const std::map<std::string, std::string>& headers) {
        completion(statusCode, base::SysUTF8ToNSString(response), @{});
      }),
      autoRetryOnNetworkChange);
}

- (void)balanceForAddress:(NSString*)address
               completion:(void (^)(bool status, NSString* balance))completion {
  _controller->GetBalance(
      base::SysNSStringToUTF8(address),
      base::BindOnce(^(bool status, const std::string& balance) {
        completion(status, base::SysUTF8ToNSString(balance));
      }));
}

- (WalletNetwork)network {
  return static_cast<WalletNetwork>(_controller->GetNetwork());
}

- (void)setNetwork:(WalletNetwork)network {
  _controller->SetNetwork(static_cast<brave_wallet::Network>(network));
}

@end
