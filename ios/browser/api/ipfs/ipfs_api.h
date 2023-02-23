/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_IPFS_IPFS_API_H_
#define BRAVE_IOS_BROWSER_API_IPFS_IPFS_API_H_

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
@protocol IpfsAPI
@required

@property(nonatomic, nullable) NSURL* nftIpfsGateway;

- (nullable NSURL*)resolveGatewayUrlFor:(NSURL*)input;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_IPFS_IPFS_API_H_
