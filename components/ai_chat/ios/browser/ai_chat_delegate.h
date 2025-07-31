// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_IOS_BROWSER_AI_CHAT_DELEGATE_H_
#define BRAVE_COMPONENTS_AI_CHAT_IOS_BROWSER_AI_CHAT_DELEGATE_H_

#import <Foundation/Foundation.h>

#ifdef __cplusplus
#include "brave/components/ai_chat/core/common/mojom/ios/ai_chat.mojom.objc.h"
#else
#import "ai_chat.mojom.objc.h"
#endif

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
@protocol AIChatDelegate <NSObject>
- (nullable NSString*)getPageTitle;
- (nullable NSURL*)getLastCommittedURL;
- (void)getPageContentWithCompletion:(void (^)(NSString* _Nullable content,
                                               bool isVideo))completion;
- (bool)isDocumentOnLoadCompletedInPrimaryFrame;

- (void)onHistoryUpdate;
- (void)onAPIRequestInProgress:(bool)inProgress;
- (void)onAPIResponseError:(AiChatAPIError)error;
- (void)onModelChanged:(NSString*)modelKey
             modelList:(NSArray<AiChatModel*>*)modelList;
- (void)onSuggestedQuestionsChanged:(NSArray<NSString*>*)questions
                             status:(AiChatSuggestionGenerationStatus)status;
- (void)onPageHasContent:(NSArray<AiChatAssociatedContent*>*)siteInfo;
- (void)onServiceStateChanged:(AiChatServiceState*)state;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_COMPONENTS_AI_CHAT_IOS_BROWSER_AI_CHAT_DELEGATE_H_
