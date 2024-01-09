// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_API_AI_CHAT_AI_CHAT_H_
#define BRAVE_IOS_BROWSER_API_AI_CHAT_AI_CHAT_H_

#import <Foundation/Foundation.h>
#import "ai_chat.mojom.objc.h"

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
@interface AIChat : NSObject
@property(nonatomic) bool isAgreementAccepted;

@property(nonatomic, readonly)
    AiChatSuggestionGenerationStatus suggestionsStatus;

@property(nonatomic, readonly) AiChatModel* currentModel;

@property(nonatomic, readonly) NSArray<AiChatModel*>* models;

@property(nonatomic, readonly)
    NSArray<AiChatConversationTurn*>* conversationHistory;

@property(nonatomic, readonly) bool isRequestInProgress;

@property(nonatomic, readonly) NSArray<NSString*>* suggestedQuestions;

@property(nonatomic, readonly) bool hasPendingConversationEntry;

@property(nonatomic, readonly) AiChatAPIError currentAPIError;

@property(nonatomic, readonly) bool canShowPremiumPrompt;

@property(nonatomic) bool shouldSendPageContents;

- (void)changeModel:(NSString*)modelKey;

- (void)submitHumanConversationEntry:(NSString*)text;

- (void)submitSummarizationRequest;

- (void)setConversationActive:(bool)is_conversation_active;

- (void)retryAPIRequest;

- (void)generateQuestions;

- (void)clearConversationHistory;

- (void)getPremiumStatus:(void (^_Nullable)(AiChatPremiumStatus))completion;

- (void)rateMessage:(bool)isLiked
             turnId:(NSUInteger)turnId
         completion:
             (void (^_Nullable)(NSString* _Nullable identifier))completion;

- (void)sendFeedback:(NSString*)category
            feedback:(NSString*)feedback
            ratingId:(NSString*)ratingId
          completion:(void (^_Nullable)(bool))completion;

- (void)dismissPremiumPrompt;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_AI_CHAT_AI_CHAT_H_
