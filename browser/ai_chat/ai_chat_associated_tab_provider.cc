// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/ai_chat_associated_tab_provider.h"

#include "brave/components/ai_chat/content/browser/ai_chat_tab_helper.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "content/public/browser/frame_tree_node_id.h"
#include "content/public/browser/web_contents.h"

namespace ai_chat {

AIChatAssociatedTabProvider::AIChatAssociatedTabProvider() = default;
AIChatAssociatedTabProvider::~AIChatAssociatedTabProvider() = default;

AssociatedContentDriver* AIChatAssociatedTabProvider::GetAssociatedContent(
    const mojom::AvailableTabPtr& tab) {
  auto* contents = content::WebContents::FromFrameTreeNodeId(
      static_cast<content::FrameTreeNodeId>(tab->frame_tree_node_id));
  if (!contents) {
    return nullptr;
  }

  return AIChatTabHelper::FromWebContents(contents);
}

}  // namespace ai_chat
