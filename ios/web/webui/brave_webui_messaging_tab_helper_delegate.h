// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_WEB_WEBUI_BRAVE_WEBUI_MESSAGING_TAB_HELPER_DELEGATE_H_
#define BRAVE_IOS_WEB_WEBUI_BRAVE_WEBUI_MESSAGING_TAB_HELPER_DELEGATE_H_

#include <Foundation/Foundation.h>

OBJC_EXPORT
@protocol BraveWebUIMessagingTabHelperDelegate

// AIChat Specific
- (void)aiChatOpenSettings;
- (void)aiChatGoPremium;
- (void)aiChatManagePremium;
- (void)aiChatHandleVoiceRecognition:(NSString*)conversationUUID;
- (void)aiChatUploadImage:(void(^)(NSURL*))callback;
@end

#endif  // BRAVE_IOS_WEB_WEBUI_BRAVE_WEBUI_MESSAGING_TAB_HELPER_DELEGATE_H_
