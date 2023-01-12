/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_IPFS_IPFS_API_
#define BRAVE_IOS_BROWSER_API_IPFS_IPFS_API_

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

NS_SWIFT_NAME(IpfsAPI)
OBJC_EXPORT
@interface IpfsAPI : NSObject

- (nullable NSURL*)resolveGatewayUrlFor:(NSString*)input;
- (nullable NSURL*)getNftIPFSGateway;
- (void)setNftIPFSGateway:(NSString*)input;

- (instancetype)init NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_IPFS_IPFS_API_
