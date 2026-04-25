// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_IOS_BROWSER_AI_CHAT_PRIVATE_H_
#define BRAVE_COMPONENTS_AI_CHAT_IOS_BROWSER_AI_CHAT_PRIVATE_H_

#include "base/memory/scoped_refptr.h"
#include "brave/components/ai_chat/ios/browser/ai_chat.h"

@protocol AIChatDelegate;

namespace ai_chat {
class AIChatService;
class ModelService;
}  // namespace ai_chat

namespace network {
class SharedURLLoaderFactory;
}

class PrefService;

@interface AIChat (Private)
- (instancetype)initWithAIChatService:(ai_chat::AIChatService*)service
                         modelService:(ai_chat::ModelService*)modelService
                         profilePrefs:(PrefService*)prefsService
                sharedURLoaderFactory:
                    (scoped_refptr<network::SharedURLLoaderFactory>)
                        sharedURLoaderFactory
                             delegate:(id<AIChatDelegate>)delegate;
@end

#endif  // BRAVE_COMPONENTS_AI_CHAT_IOS_BROWSER_AI_CHAT_PRIVATE_H_
