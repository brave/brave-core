// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/browser_tool_provider_factory.h"

#include <memory>
#include <vector>

#include "brave/browser/ai_chat/browser_tool_provider.h"
#include "brave/browser/ai_chat/page_content_blocks.h"
#include "brave/browser/ai_chat/tools/click_tool.h"
#include "brave/browser/ai_chat/tools/drag_and_release_tool.h"
#include "brave/browser/ai_chat/tools/history_tool.h"
#include "brave/browser/ai_chat/tools/move_mouse_tool.h"
#include "brave/browser/ai_chat/tools/navigation_tool.h"
#include "brave/browser/ai_chat/tools/scroll_tool.h"
#include "brave/browser/ai_chat/tools/select_tool.h"
#include "brave/browser/ai_chat/tools/type_tool.h"
#include "brave/browser/ai_chat/tools/wait_tool.h"
#include "brave/components/ai_chat/core/browser/tools/todo_tool.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"
#include "brave/components/ai_chat/core/browser/tools/tool_provider.h"
#include "brave/components/ai_chat/core/browser/tools/tool_utils.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "chrome/browser/actor/actor_keyed_service.h"
#include "chrome/browser/actor/actor_task.h"
#include "chrome/browser/actor/browser_action_util.h"
#include "chrome/browser/actor/task_id.h"
#include "chrome/browser/page_content_annotations/multi_source_page_context_fetcher.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/common/actor.mojom.h"
#include "components/optimization_guide/content/browser/page_content_proto_provider.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/web_contents.h"

namespace ai_chat {

BrowserToolProviderFactory::BrowserToolProviderFactory(
    Profile* profile,
    actor::ActorKeyedService* actor_service)
    : actor_service_(actor_service), profile_(profile) {}

BrowserToolProviderFactory::~BrowserToolProviderFactory() = default;

std::unique_ptr<ToolProvider> BrowserToolProviderFactory::CreateToolProvider() {
  return std::make_unique<BrowserToolProvider>(profile_, actor_service_.get());
}

}  // namespace ai_chat
