// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_BRAVE_SHIELDS_REQUEST_BLOCKING_REQUEST_BLOCKING_TAB_HELPER_BRIDGE_H_
#define BRAVE_IOS_BROWSER_BRAVE_SHIELDS_REQUEST_BLOCKING_REQUEST_BLOCKING_TAB_HELPER_BRIDGE_H_

NS_ASSUME_NONNULL_BEGIN

@protocol RequestBlockingTabHelperBridge

@required

/**
 * Determines whether a specific resource should be blocked.
 *
 * @param requestURL The URL of the requested resource.
 * @param sourceURL The URL of the page that initiated the request.
 * @param resourceType A string indicating the type of resource (e.g., "image",
 * "script", "stylesheet").
 * @param completion A block invoked with YES if the resource should be blocked,
 * NO otherwise.
 */
- (void)shouldBlockRequestURL:(NSURL*)requestURL
                    sourceURL:(NSURL*)sourceURL
                 resourceType:(NSString*)resourceType
                   completion:(void (^)(BOOL shouldBlock))completion
    NS_SWIFT_NAME(shouldBlock(requestURL:sourceURL:resourceType:completion:));

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_BRAVE_SHIELDS_REQUEST_BLOCKING_REQUEST_BLOCKING_TAB_HELPER_BRIDGE_H_
