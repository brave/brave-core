// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_IOS_BROWSER_AI_CHAT_ASSOCIATED_CONTENT_PAGE_FETCHER_H_
#define BRAVE_COMPONENTS_AI_CHAT_IOS_BROWSER_AI_CHAT_ASSOCIATED_CONTENT_PAGE_FETCHER_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@protocol AIChatAssociatedContentPageFetcher
@required
- (void)fetchPageContent:(void (^)(NSString* _Nullable content,
                                   BOOL isVideo))completionHandler;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_COMPONENTS_AI_CHAT_IOS_BROWSER_AI_CHAT_ASSOCIATED_CONTENT_PAGE_FETCHER_H_
