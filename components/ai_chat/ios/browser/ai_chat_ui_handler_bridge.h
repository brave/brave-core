// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_IOS_BROWSER_AI_CHAT_UI_HANDLER_BRIDGE_H_
#define BRAVE_COMPONENTS_AI_CHAT_IOS_BROWSER_AI_CHAT_UI_HANDLER_BRIDGE_H_

#import <Foundation/Foundation.h>

@class AiChatUploadedFile;

NS_ASSUME_NONNULL_BEGIN

/// Browser-side handler for general AI Chat UI functions
///
/// This is an Obj-C bridge for the AIChatUIHandler mojom interface (see
/// ai_chat.mojom) and only bridges methods that will be called in iOS/mobile
NS_SWIFT_NAME(AIChatUIHandler)
@protocol AIChatUIHandlerBridge
@required

/// Handle when a user taps on the microphone icon and call the completion
/// handler with a text version of the users prompt or nil if the user had
/// cancelled or rejected the microphone permission
- (void)handleVoiceRecognitionRequest:
    (void (^)(NSString* _Nullable query))completion;

/// Handle when the user taps the element to upload files to Leo.
- (void)handleFileUploadRequest:(BOOL)useMediaCapture
              completionHandler:
                  (void (^)(NSArray<AiChatUploadedFile*>* _Nullable files))
                      completion;

/// Open the AI chat settings UI
- (void)openAIChatSettings;

/// Handle when the user taps "Go Premium"
- (void)goPremium;

/// Handle when the user taps "Manage Premium"
- (void)managePremium;

/// Handle when the user taps a close button on the UI
- (void)closeUI;

/// Handle opening a URL in a new tab
- (void)openURL:(NSURL*)url;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_COMPONENTS_AI_CHAT_IOS_BROWSER_AI_CHAT_UI_HANDLER_BRIDGE_H_
