// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_AI_CHAT_AI_CHAT_ASSOCIATED_CONTENT_DRIVER_BRIDGE_H_
#define BRAVE_IOS_BROWSER_AI_CHAT_AI_CHAT_ASSOCIATED_CONTENT_DRIVER_BRIDGE_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

NS_SWIFT_NAME(AIChatAssociatedContentDriver)
@protocol AIChatAssociatedContentDriverBridge
@required

@property(readonly, nullable) NSString* pageTitle;
@property(readonly, nullable) NSURL* lastCommittedURL;
- (void)fetchPageContent:(void (^)(NSString* _Nullable content,
                                   BOOL isVideo))completionHandler;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_AI_CHAT_AI_CHAT_ASSOCIATED_CONTENT_DRIVER_BRIDGE_H_
