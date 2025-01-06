// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ASSOCIATED_TAB_DELEGATE_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ASSOCIATED_TAB_DELEGATE_H_

#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"

namespace ai_chat {

class AssociatedContentDriver;

class AssociatedTabDelegate {
 public:
  virtual ~AssociatedTabDelegate() {}
  virtual AssociatedContentDriver* GetAssociatedContent(
      const mojom::AvailableTabPtr& tab) = 0;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ASSOCIATED_TAB_DELEGATE_H_
