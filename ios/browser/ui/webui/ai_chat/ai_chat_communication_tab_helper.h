// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_UI_WEBUI_AI_CHAT_AI_CHAT_COMMUNICATION_TAB_HELPER_H_
#define BRAVE_IOS_BROWSER_UI_WEBUI_AI_CHAT_AI_CHAT_COMMUNICATION_TAB_HELPER_H_

#include <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@class AIChatCommunicationController;

// Any Swift methods that WebUI needs to call, to this protocol
OBJC_EXPORT
@protocol AIChatCommunicationProtocol
- (void)handleVoiceRecognition:(AIChatCommunicationController*)controller
            withConversationId:(NSString*)conversationId;
- (void)fetchImageForChatUpload:(AIChatCommunicationController*)controller
                     completion:(void (^)(NSURL* _Nullable))completion;
- (void)openSettings:(AIChatCommunicationController*)controller;
- (void)openConversationFullPage:(AIChatCommunicationController*)controller
                  conversationId:(NSString*)conversationId;
- (void)openURL:(AIChatCommunicationController*)controller url:(NSURL*)url;
- (void)goPremium:(AIChatCommunicationController*)controller;
- (void)managePremium:(AIChatCommunicationController*)controller;
@end

OBJC_EXPORT
@interface AIChatCommunicationController : NSObject
@property(nonatomic, weak, nullable) id<AIChatCommunicationProtocol> delegate;

// Any methods swift want to call on the WebUI, goes here
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_UI_WEBUI_AI_CHAT_AI_CHAT_COMMUNICATION_TAB_HELPER_H_
