// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_UI_WEBUI_AI_CHAT_AI_CHAT_COMMUNICATION_TAB_HELPER_H_
#define BRAVE_IOS_BROWSER_UI_WEBUI_AI_CHAT_AI_CHAT_COMMUNICATION_TAB_HELPER_H_

#include <Foundation/Foundation.h>

// Any Swift methods that WebUI needs to call, to this protocol
@protocol AIChatCommunicationProtocol: NSObject
- (void)handleVoiceRecognitionWithConversationID:(NSString*)ConversationID;
- (void)fetchImageForChatUpload:(void(^)(NSURL*))completion;
- (void)openSettings;
- (void)openConversationFullPage:(NSString*)conversationID;
- (void)openURL:(NSURL*)url;
- (void)goPremium;
- (void)refreshPremiumSession;
- (void)managePremium;
@end

@interface AIChatCommunicationController: NSObject
@property(nonatomic, weak) id<AIChatCommunicationProtocol> delegate;

// Any methods swift want to call on the WebUI, goes here
@end

#endif  // BRAVE_IOS_BROWSER_UI_WEBUI_AI_CHAT_AI_CHAT_COMMUNICATION_TAB_HELPER_H_

