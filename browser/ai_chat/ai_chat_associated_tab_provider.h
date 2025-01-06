// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_AI_CHAT_ASSOCIATED_TAB_PROVIDER_H_
#define BRAVE_BROWSER_AI_CHAT_AI_CHAT_ASSOCIATED_TAB_PROVIDER_H_

#include "brave/components/ai_chat/core/browser/associated_tab_delegate.h"

namespace ai_chat {

class AIChatAssociatedTabProvider : public AssociatedTabDelegate {
 public:
  AIChatAssociatedTabProvider();
  ~AIChatAssociatedTabProvider() override;

  AssociatedContentDriver* GetAssociatedContent(
      const mojom::AvailableTabPtr& tab) override;
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_AI_CHAT_ASSOCIATED_TAB_PROVIDER_H_
