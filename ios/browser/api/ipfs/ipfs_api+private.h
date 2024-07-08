/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_IPFS_IPFS_API_PRIVATE_H_
#define BRAVE_IOS_BROWSER_API_IPFS_IPFS_API_PRIVATE_H_

#import <Foundation/Foundation.h>

#include "brave/ios/browser/api/ipfs/ipfs_api.h"

NS_ASSUME_NONNULL_BEGIN

class ChromeBrowserState;

@interface IpfsAPIImpl : NSObject <IpfsAPI>
@property(nonatomic, nullable) NSURL* nftIpfsGateway;
@property(nonatomic, nullable) NSURL* ipfsGateway;

- (nullable NSURL*)resolveGatewayUrlFor:(NSURL*)input;
- (nullable NSURL*)resolveGatewayUrlForNft:(NSURL*)input;
- (instancetype)initWithBrowserState:(ChromeBrowserState*)mainBrowserState;
- (instancetype)init NS_UNAVAILABLE;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_IPFS_IPFS_API_PRIVATE_H_
