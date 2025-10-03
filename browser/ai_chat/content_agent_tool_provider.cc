// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/content_agent_tool_provider.h"

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
#include "brave/components/ai_chat/core/browser/tools/tool.h"
#include "brave/components/ai_chat/core/browser/tools/tool_provider.h"
#include "brave/components/ai_chat/core/browser/tools/tool_utils.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "chrome/browser/actor/actor_keyed_service.h"
#include "chrome/browser/actor/actor_task.h"
#include "chrome/browser/actor/browser_action_util.h"
#include "chrome/browser/actor/task_id.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_navigator.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface_iterator.h"
#include "chrome/common/actor.mojom.h"
#include "components/optimization_guide/content/browser/page_content_proto_provider.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/page_transition_types.h"

namespace ai_chat {

ContentAgentToolProvider::ContentAgentToolProvider(
    Profile* profile,
    actor::ActorKeyedService* actor_service)
    : actor_service_(actor_service), profile_(profile) {
  // This class should only exist with a valid actor service.
  CHECK(actor_service_);

  // Each conversation can have a different actor service task,
  // and operate on a different set of tabs.
  // If we want to delay creation of the task, we'll need to perhaps
  // intercept all tool use calls and create or choose which task to use
  // at that time. Tool::UseTool will have to change to
  // ToolProvider::UseTool, or similar.
  // If we want each conversation message to act on a different set of tabs and
  // not have access to any tabs previously acted on in the same conversation,
  // we should create a new task inside `ToolProvider::OnNewGenerationLoop`.
  task_id_ = actor_service_->CreateTask();

  CreateTools();
}

ContentAgentToolProvider::~ContentAgentToolProvider() = default;

std::vector<base::WeakPtr<Tool>> ContentAgentToolProvider::GetTools() {
  // Note: We don't have the ability to filter tools based on conversation
  // capability here. But for now we don't need to as we only create the content
  // this class if we're allowed to have content agent tools (which is only
  // within agent profiles).
  std::vector<base::WeakPtr<Tool>> tool_ptrs;
  tool_ptrs.reserve(tools_.size());
  std::ranges::transform(tools_, std::back_inserter(tool_ptrs),
                         &Tool::GetWeakPtr);
  return tool_ptrs;
}

void ContentAgentToolProvider::StopAllTasks() {
  if (!task_id_.is_null()) {
    // `success` sets whether the task ends as state kFinished or kCancelled
    actor_service_->StopTask(task_id_, true /* success */);
  }
}

actor::TaskId ContentAgentToolProvider::GetTaskId() {
  return task_id_;
}

void ContentAgentToolProvider::GetOrCreateTabHandleForTask(
    base::OnceCallback<void(tabs::TabHandle)> callback) {
  if (!task_tab_handle_.Get()) {
    // Create a new tab because we are only allowed to act on
    // certain URLs, e.g. NTP. Safer to start on a blank page
    // whilst this feature is focused on AI-initiated tasks instead
    // of acting on existing tabs.
    NavigateParams params(profile_, GURL(url::kAboutBlankURL),
                          ui::PAGE_TRANSITION_FROM_API);
    params.disposition = WindowOpenDisposition::NEW_BACKGROUND_TAB;
    Navigate(&params);
    content::WebContents* new_contents = params.navigated_or_inserted_contents;

    task_tab_handle_ =
        tabs::TabInterface::GetFromContents(new_contents)->GetHandle();
  }
  actor_service_->GetTask(task_id_)->AddTab(
      task_tab_handle_,
      base::BindOnce(&ContentAgentToolProvider::TabAddedToTask,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void ContentAgentToolProvider::TabAddedToTask(
    base::OnceCallback<void(tabs::TabHandle)> callback,
    actor::mojom::ActionResultPtr result) {
  std::move(callback).Run(task_tab_handle_);
}

void ContentAgentToolProvider::ExecuteActions(
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
      base::BindOnce(&ContentAgentToolProvider::OnActionsFinished,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void ContentAgentToolProvider::CreateTools() {
  tools_.clear();

  tools_.push_back(std::make_unique<ClickTool>(this));
  tools_.push_back(std::make_unique<DragAndReleaseTool>(this));
  tools_.push_back(std::make_unique<HistoryTool>(this));
  tools_.push_back(std::make_unique<MoveMouseTool>(this));
  tools_.push_back(std::make_unique<NavigationTool>(this));
  tools_.push_back(std::make_unique<ScrollTool>(this));
  tools_.push_back(std::make_unique<SelectTool>(this));
  tools_.push_back(std::make_unique<TypeTool>(this));
  tools_.push_back(std::make_unique<WaitTool>(this));
}

void ContentAgentToolProvider::OnActionsFinished(
    Tool::UseToolCallback callback,
    actor::mojom::ActionResultCode result_code,
    std::optional<size_t> index_of_failed_action,
    std::vector<actor::ActionResultWithLatencyInfo> action_results) {
  if (result_code == actor::mojom::ActionResultCode::kOk) {
    // Send current page content for result

    // TODO(https://github.com/brave/brave-browser/issues/49259):
    // Use multi_source_page_context_fetcher.h (or use it
    // via ActorKeyedService), now that this API is public outside of glic.

    auto options = blink::mojom::AIPageContentOptions::New();
    options->mode = blink::mojom::AIPageContentMode::kActionableElements;

    // Verify the tab handle is still valid as the tab might have been
    // closed.
    if (!task_tab_handle_.Get() || !task_tab_handle_.Get()->GetContents()) {
      std::move(callback).Run(
          CreateContentBlocksForText("Tab is no longer open"));
      return;
    }

    optimization_guide::GetAIPageContent(
        task_tab_handle_.Get()->GetContents(), std::move(options),
        base::BindOnce(&ContentAgentToolProvider::ReceivedAnnotatedPageContent,
                       weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
  } else {
    DLOG(ERROR) << "Action failed, see actor.mojom for result code meaning: "
                << result_code;
    std::move(callback).Run(CreateContentBlocksForText("Action failed"));
  }
}

void ContentAgentToolProvider::ReceivedAnnotatedPageContent(
    Tool::UseToolCallback callback,
    std::optional<optimization_guide::AIPageContentResult> content) {
  if (!content.has_value()) {
    DLOG(ERROR) << "Error getting page content";
    std::move(callback).Run(
        CreateContentBlocksForText("Error getting page content"));
    return;
  }

  auto apc = content->proto;

  if (!apc.has_root_node()) {
    DLOG(ERROR) << "No root node";
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
