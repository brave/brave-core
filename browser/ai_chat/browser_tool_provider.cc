// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/browser_tool_provider.h"

#include <memory>
#include <vector>

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

BrowserToolProvider::BrowserToolProvider(
    Profile* profile,
    raw_ptr<actor::ActorKeyedService> actor_service)
    : actor_service_(actor_service), profile_(profile) {
  // Only create page actor tools if this profile is allowed to.
  if (actor_service_) {
    // Each conversation can have a different actor service task,
    // and operate on a different set of tabs.
    // If we want to delay creation of the task, we'll need to perhaps
    // intercept all tool use calls and create or choose which task to use
    // at that time. Tool::UseTool will have to change to
    // ToolProvider::UseTool, or similar.
    task_id_ = actor_service_->CreateTask();
  }
  CreateTools();
}

BrowserToolProvider::~BrowserToolProvider() {
  // Relinquish control of the tabs
  StopAllTasks();
}

std::vector<base::WeakPtr<Tool>> BrowserToolProvider::GetTools() {
  // TODO(petemill): Filter tools based on conversation capability.
  // For now we don't create the content agent tools if we don't have the
  // actor_service_ (which is only provided for agent profiles).
  std::vector<base::WeakPtr<Tool>> tool_ptrs;
  tool_ptrs.reserve(tools_.size());
  // TODO(petemill): Use a separate class variable for each tool, no need
  // for a vector.
  for (const auto& tool : tools_) {
    tool_ptrs.push_back(tool->GetWeakPtr());
  }
  return tool_ptrs;
}

void BrowserToolProvider::StopAllTasks() {
  if (actor_service_ && !task_id_.is_null()) {
    actor_service_->StopTask(task_id_);
  }
}

actor::TaskId BrowserToolProvider::GetTaskId() {
  return task_id_;
}

void BrowserToolProvider::GetOrCreateTabHandleForTask(
    base::OnceCallback<void(tabs::TabHandle)> callback) {
  if (!task_tab_handle_.Get()) {
    // Get the most recently active browser for this profile.
    Browser* browser =
        chrome::FindTabbedBrowser(profile_, /*match_original_profiles=*/false);
    // If no browser exists create one.
    if (!browser) {
      browser = Browser::Create(
          Browser::CreateParams(profile_, /*user_gesture=*/false));
    }
    // Create a new tab because we are only allowed to act on
    // certain URLs. Safer to start on a blank page.
    content::WebContents* new_contents = chrome::AddAndReturnTabAt(
        browser, GURL(url::kAboutBlankURL), -1, false);

    task_tab_handle_ =
        tabs::TabInterface::GetFromContents(new_contents)->GetHandle();

    for (auto& observer : observers_) {
      observer.OnContentTaskStarted(task_tab_handle_);
    }
  }
  actor_service_->GetTask(task_id_)->AddTab(
      task_tab_handle_, base::BindOnce(&BrowserToolProvider::TabAdded,
                                       weak_ptr_factory_.GetWeakPtr(),
                                       task_tab_handle_, std::move(callback)));
}

void BrowserToolProvider::TabAdded(
    tabs::TabHandle tab_handle,
    base::OnceCallback<void(tabs::TabHandle)> callback,
    actor::mojom::ActionResultPtr result) {
  std::move(callback).Run(tab_handle);
}

void BrowserToolProvider::ExecuteActions(
    optimization_guide::proto::Actions actions,
    Tool::UseToolCallback callback) {
  actor::BuildToolRequestResult requests = actor::BuildToolRequest(actions);

  if (!requests.has_value()) {
    DLOG(ERROR) << "Action Failed to convert BrowserAction to ToolRequests.";
    std::move(callback).Run(
        CreateContentBlocksForText("Action failed - incorrect parameters"));
    return;
  }

  actor_service_->PerformActions(
      actor::TaskId(actions.task_id()), std::move(requests.value()),
      base::BindOnce(&BrowserToolProvider::OnActionsFinished,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void BrowserToolProvider::CreateTools() {
  tools_.clear();

  if (actor_service_) {
    // TaskId is consistent for this conversation. But we might
    // want to make this more dynamic and have the the tool use request
    // specify which task (set of tabs) to act on.
    tools_.push_back(std::make_unique<ClickTool>(this, actor_service_));
    tools_.push_back(
        std::make_unique<DragAndReleaseTool>(this, actor_service_));
    tools_.push_back(std::make_unique<HistoryTool>(this, actor_service_));
    tools_.push_back(std::make_unique<MoveMouseTool>(this, actor_service_));
    tools_.push_back(std::make_unique<NavigationTool>(this, actor_service_));
    tools_.push_back(std::make_unique<ScrollTool>(this, actor_service_));
    tools_.push_back(std::make_unique<SelectTool>(this, actor_service_));
    tools_.push_back(std::make_unique<TypeTool>(this, actor_service_));
    tools_.push_back(std::make_unique<WaitTool>(this, actor_service_));
    // Note: TODO tool is only created for agent conversations (assuming
    // actor_service_ is only available for agent profiles). Consider instead
    // looking at conversation capability to determine whether to provide this
    // tool.
    tools_.push_back(std::make_unique<TodoTool>());
  }
}

void BrowserToolProvider::OnActionsFinished(
    Tool::UseToolCallback callback,
    actor::mojom::ActionResultCode result_code,
    std::optional<size_t> index_of_failed_action) {
  if (result_code == actor::mojom::ActionResultCode::kOk) {
    // TODO(cr140): Use multi_source_page_context_fetcher.h (or use it
    // via ActorKeyedService). For now we have to call into glic.
    // Or we can avoid glic via:
    // - Annotated page content from `optimization_guide::GetAIPageContent`
    // - Screenshot from our own implementation
    // glic::mojom::GetTabContextOptions options;
    // options.include_annotated_page_content = true;
    // options.include_viewport_screenshot = true;

    // Get page content
    auto options = blink::mojom::AIPageContentOptions::New();
    options->mode = blink::mojom::AIPageContentMode::kActionableElements;

    optimization_guide::GetAIPageContent(
        task_tab_handle_.Get()->GetContents(), std::move(options),
        base::BindOnce(&BrowserToolProvider::ReceivedAnnotatedPageContent,
                       weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
  } else {
    DLOG(ERROR) << "Action failed, see actor.mojom for result code meaning: "
                << result_code;
    std::move(callback).Run(CreateContentBlocksForText("Action failed"));
  }
}

void BrowserToolProvider::ReceivedAnnotatedPageContent(
    Tool::UseToolCallback callback,
    std::optional<optimization_guide::AIPageContentResult> content) {
  if (!content.has_value()) {
    LOG(ERROR) << "Error getting page content";
    std::move(callback).Run(
        CreateContentBlocksForText("Error getting page content"));
    return;
  }

  auto apc = content->proto;

  if (!apc.has_root_node()) {
    LOG(ERROR) << "No root node";
    std::move(callback).Run(CreateContentBlocksForText("No root node"));
    return;
  }

  auto content_blocks = ConvertAnnotatedPageContentToBlocks(apc);
  content_blocks.insert(
      content_blocks.begin(),
      std::move(CreateContentBlocksForText("Action successful")[0]));
  std::move(callback).Run(std::move(content_blocks));
}

}  // namespace ai_chat
