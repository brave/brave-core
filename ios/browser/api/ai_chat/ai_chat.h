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
@interface AiChat : NSObject  // AiChat Namespace for Swift exports
- (instancetype)init NS_UNAVAILABLE;
@end

OBJC_EXPORT
@interface AIChat : NSObject
@property(nonatomic) bool isAgreementAccepted;

@property(nonatomic, readonly)
    NSArray<AiChatConversationTurn*>* conversationHistory;

@property(nonatomic, readonly) NSArray<AiChatActionGroup*>* slashActions;

@property(nonatomic) NSString* defaultModelKey;

- (void)createNewConversation;

- (void)getState:(void (^_Nullable)(AiChatConversationState*))completion;

- (void)setShouldSendPageContents:(bool)shouldSend;

- (void)changeModel:(NSString*)modelKey;

- (void)submitHumanConversationEntry:(NSString*)text;

- (void)submitSuggestion:(NSString*)text;

- (void)submitSummarizationRequest;

- (void)retryAPIRequest;

- (void)generateQuestions;

- (void)clearErrorAndGetFailedMessage:
    (void (^_Nullable)(AiChatConversationTurn*))completion;

- (void)getPremiumStatus:(void (^_Nullable)(AiChatPremiumStatus))completion;

- (void)submitSelectedText:(NSString*)selectedText
                actionType:(AiChatActionType)actionType;

- (void)rateMessage:(bool)isLiked
             turnId:(NSString*)turnId
         completion:
             (void (^_Nullable)(NSString* _Nullable identifier))completion;

- (void)sendFeedback:(NSString*)category
            feedback:(NSString*)feedback
            ratingId:(NSString*)ratingId
         sendPageUrl:(bool)sendPageUrl
          completion:(void (^_Nullable)(bool))completion;

- (void)modifyConversation:(NSUInteger)turnId newText:(NSString*)newText;

- (void)dismissPremiumPrompt;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_AI_CHAT_AI_CHAT_H_
