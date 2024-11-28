// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_API_AI_CHAT_AI_CHAT_DELEGATE_H_
#define BRAVE_IOS_BROWSER_API_AI_CHAT_AI_CHAT_DELEGATE_H_

#import <Foundation/Foundation.h>
#import "ai_chat.mojom.objc.h"

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
- (void)onPageHasContent:(AiChatSiteInfo*)siteInfo
       shouldSendContent:(bool)shouldSendContent;
- (void)onServiceStateChanged:(AiChatServiceState*)state;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_AI_CHAT_AI_CHAT_DELEGATE_H_
