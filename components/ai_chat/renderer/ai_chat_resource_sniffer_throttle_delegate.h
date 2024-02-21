// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_RENDERER_AI_CHAT_RESOURCE_SNIFFER_THROTTLE_DELEGATE_H_
#define BRAVE_COMPONENTS_AI_CHAT_RENDERER_AI_CHAT_RESOURCE_SNIFFER_THROTTLE_DELEGATE_H_

#include "brave/components/ai_chat/core/common/mojom/page_content_extractor.mojom.h"

namespace ai_chat {

class AIChatResourceSnifferThrottleDelegate {
 public:
  virtual void OnInterceptedPageContentChanged(
      mojom::PageContentPtr content) = 0;

 protected:
  virtual ~AIChatResourceSnifferThrottleDelegate() = default;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_RENDERER_AI_CHAT_RESOURCE_SNIFFER_THROTTLE_DELEGATE_H_
