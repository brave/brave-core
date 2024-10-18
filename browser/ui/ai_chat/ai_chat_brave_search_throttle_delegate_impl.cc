/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/ai_chat/ai_chat_brave_search_throttle_delegate_impl.h"

#include "brave/browser/ui/sidebar/sidebar_utils.h"
#include "brave/components/sidebar/browser/sidebar_item.h"

namespace ai_chat {

void AIChatBraveSearchThrottleDelegateImpl::OpenLeo(
    content::WebContents* web_contents) {
  ActivatePanelItem(web_contents,
                    sidebar::SidebarItem::BuiltInItemType::kChatUI);
}

}  // namespace ai_chat
