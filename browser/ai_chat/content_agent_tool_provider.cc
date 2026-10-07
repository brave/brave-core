// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/content_agent_tool_provider.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/containers/fixed_flat_set.h"
#include "base/strings/strcat.h"
#include "base/strings/to_string.h"
#include "brave/browser/ai_chat/ai_chat_enterprise_policy_checker.h"
#include "brave/browser/ai_chat/tools/click_tool.h"
#include "brave/browser/ai_chat/tools/drag_and_release_tool.h"
#include "brave/browser/ai_chat/tools/history_tool.h"
#include "brave/browser/ai_chat/tools/move_mouse_tool.h"
#include "brave/browser/ai_chat/tools/navigation_tool.h"
#include "brave/browser/ai_chat/tools/scroll_tool.h"
#include "brave/browser/ai_chat/tools/select_tool.h"
#include "brave/browser/ai_chat/tools/type_tool.h"
#include "brave/browser/ai_chat/tools/wait_tool.h"
#include "brave/components/ai_chat/content/browser/page_content_blocks.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"
#include "brave/components/ai_chat/core/browser/tools/tool_provider.h"
#include "brave/components/ai_chat/core/browser/tools/tool_utils.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "chrome/browser/actor/actor_keyed_service.h"
#include "chrome/browser/actor/actor_proto_conversion.h"
#include "chrome/browser/actor/actor_task.h"
#include "chrome/browser/actor/actor_task_metadata.h"
#include "chrome/browser/actor/tab_observation_strategy.h"
#include "chrome/browser/actor/ui/actor_ui_state_manager.h"
#include "chrome/browser/glic/actor/glic_actor_policy_checker.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/navigator/browser_navigator.h"
#include "chrome/common/actor.mojom.h"
#include "chrome/common/actor/action_result.h"
#include "components/actor/core/aggregated_journal.h"
#include "components/actor/core/journal_details_builder.h"
#include "components/actor/core/task_id.h"
#include "components/actor/core/task_source_info.h"
#include "components/optimization_guide/content/browser/page_content_proto_provider.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/page_transition_types.h"

static_assert(BUILDFLAG(ENABLE_BRAVE_AI_CHAT_AGENT_PROFILE));

namespace ai_chat {

namespace {

constexpr auto kActorStatesToNotify =
    base::MakeFixedFlatSet<actor::ActorTask::State>({
        actor::ActorTask::State::kPausedByUser,
        actor::ActorTask::State::kPausedByActor,
        actor::ActorTask::State::kActing,
        actor::ActorTask::State::kReflecting,
        actor::ActorTask::State::kWaitingOnUser,
    });

std::vector<actor::mojom::JournalDetailsPtr> MakeDetails(
    std::string_view details) {
  if (details.empty()) {
    return {};
  }
  return actor::JournalDetailsBuilder().Add("details", details).Build();
}

// Owns an actor journal entry for one agentic loop phase. Deliberately holds no
// reference to the provider that created it, so it can outlive one: the entry
// keeps a SafeRef to the journal, which belongs to the profile's
// ActorKeyedService.
class JournalPhaseEntry : public AgentJournal::PendingEntry {
 public:
  explicit JournalPhaseEntry(
      std::unique_ptr<actor::AggregatedJournal::PendingAsyncEntry> entry)
      : entry_(std::move(entry)) {}
  ~JournalPhaseEntry() override = default;

  void End(std::string_view details) override {
    if (entry_) {
      entry_->EndEntry(MakeDetails(details));
      entry_.reset();
    }
  }

 private:
  std::unique_ptr<actor::AggregatedJournal::PendingAsyncEntry> entry_;
};

size_t CountTextChars(
    const std::vector<mojom::ContentBlockPtr>& content_blocks) {
  size_t chars = 0;
  for (const auto& block : content_blocks) {
    if (block->is_text_content_block()) {
      chars += block->get_text_content_block()->text.size();
    }
  }
  return chars;
}

}  // namespace

ContentAgentToolProvider::ContentAgentToolProvider(
    Profile* profile,
    actor::ActorKeyedService* actor_service,
    actor::ui::ActorUiStateManagerInterface& ui_state_manager)
    : actor_service_(actor_service),
      profile_(profile),
      ui_state_manager_(ui_state_manager) {
  // This class should only exist if the feature is enabled
  CHECK(ai_chat::features::IsAIChatAgentProfileEnabled());
  // This class should only exist with a valid actor service
  CHECK(actor_service_);

  // Each conversation can have a different actor service task,
  // and operate on a different set of tabs.
  // If we want to delay creation of the task, we'll need to perhaps
  // intercept all tool use calls and create or choose which task to use
  // at that time. Tool::UseTool will have to change to
  // ToolProvider::UseTool, or similar.
  // If we want each conversation message to act on a different set of tabs and
  // not have access to any tabs previously acted on in the same conversation,
  // we should create a new task inside
  // `ToolProvider::UpdateToolsForNewGenerationLoop`.
  task_id_ = actor_service_->CreateTaskWithOptions(
      actor::TaskSourceInfo(actor::TaskSourceInfo::Client::kExperimentalActor,
                            /*id=*/std::nullopt),
      AIChatEnterprisePolicyChecker::NoEnterprisePolicyChecker(),
      /*options=*/nullptr, /*delegate=*/nullptr, &*ui_state_manager_);

  actor_task_state_changed_subscription_ =
      actor_service_->AddTaskStateChangedCallback(base::BindRepeating(
          &ContentAgentToolProvider::OnActorTaskStateChanged,
          base::Unretained(this)));

  CreateTools();
}

ContentAgentToolProvider::~ContentAgentToolProvider() {
  // When this tool provider and its owned uses (e.g. conversation)are closed,
  // we should hand back state of the tab to regular uses of the browser.
  // Don't use StopAllTasks(): it creates a replacement task that would be
  // orphaned (nobody left to stop it) and hold a dangling reference to the
  // current UI state manager once this provider is gone.
  if (!task_id_.is_null()) {
    actor_service_->StopTask(task_id_,
                             actor::ActorTask::StoppedReason::kTaskComplete);
  }
}

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

void ContentAgentToolProvider::OnGenerationCompleteWithNoToolsToHandle() {
  // Marks all tools for this round of the loop being completed, we can return
  // control back to the tab(s).
  StopAllTasks();
}

void ContentAgentToolProvider::PauseAllTasks() {
  // When user asks to pause the task, we can return control to the tabs.
  if (!task_id_.is_null()) {
    if (auto* task = actor_service_->GetTask(task_id_)) {
      task->Pause(false);
    }
  }
}

void ContentAgentToolProvider::ResumeAllTasks() {
  if (!task_id_.is_null()) {
    if (auto* task = actor_service_->GetTask(task_id_)) {
      task->Resume();
    }
  }
}

void ContentAgentToolProvider::StopAllTasks() {
  if (!task_id_.is_null()) {
    actor::TaskId stopping_task_id = std::move(task_id_);
    task_id_ = actor_service_->CreateTaskWithOptions(
        actor::TaskSourceInfo(actor::TaskSourceInfo::Client::kExperimentalActor,
                              /*id=*/std::nullopt),
        AIChatEnterprisePolicyChecker::NoEnterprisePolicyChecker(),
        /*options=*/nullptr, /*delegate=*/nullptr, &*ui_state_manager_);
    actor_service_->StopTask(stopping_task_id,
                             actor::ActorTask::StoppedReason::kTaskComplete);
  }
}

bool ContentAgentToolProvider::IsPausedByUser() {
  // Tools shouldn't be attempted to run whilst actor task is paused by user
  actor::ActorTask* task = actor_service_->GetTask(task_id_);
  if (!task) {
    return false;
  }
  return task->IsUnderUserControl();
}

AgentJournal* ContentAgentToolProvider::GetAgentJournal() {
  return this;
}

std::unique_ptr<AgentJournal::PendingEntry> ContentAgentToolProvider::Begin(
    std::string_view event_name,
    std::string_view details) {
  return std::make_unique<JournalPhaseEntry>(
      actor_service_->GetJournal().CreatePendingAsyncEntry(
          GetTaskURL(), task_id_, actor::MakeBrowserTrackUUID(task_id_),
          event_name, MakeDetails(details)));
}

void ContentAgentToolProvider::Log(std::string_view event_name,
                                   std::string_view details) {
  actor_service_->GetJournal().Log(GetTaskURL(), task_id_,
                                   actor::MakeBrowserTrackUUID(task_id_),
                                   event_name, MakeDetails(details));
}

GURL ContentAgentToolProvider::GetTaskURL() const {
  auto* tab = task_tab_handle_.Get();
  if (!tab || !tab->GetContents()) {
    return GURL();
  }
  return tab->GetContents()->GetLastCommittedURL();
}

actor::TaskId ContentAgentToolProvider::GetTaskId() {
  return task_id_;
}

void ContentAgentToolProvider::GetOrCreateTabHandleForTask(
    base::OnceCallback<void(tabs::TabHandle)> callback) {
  tab_setup_phase_ = std::make_unique<AgentPhase>(
      this, kAgentPhaseTabSetup,
      base::StrCat({"new_tab=", base::ToString(!task_tab_handle_.Get())}));

  if (!task_tab_handle_.Get()) {
    // Create a new tab because we are only allowed to act on
    // certain URLs, e.g. NTP. Safer to start on a blank page
    // whilst this feature is focused on AI-initiated tasks instead
    // of acting on existing tabs.
    NavigateParams params(profile_, GURL(url::kAboutBlankURL),
                          ui::PAGE_TRANSITION_FROM_API);
    params.disposition = WindowOpenDisposition::NEW_FOREGROUND_TAB;
    Navigate(&params);
    content::WebContents* new_contents = params.navigated_or_inserted_contents;

    task_tab_handle_ =
        tabs::TabInterface::GetFromContents(new_contents)->GetHandle();

    for (auto& observer : observers_) {
      observer.OnContentTaskStarted(task_tab_handle_.raw_value());
    }
  }
  auto* task = actor_service_->GetTask(task_id_);
  if (!task) {
    // Task was removed (e.g. stopped between generation
    // completing and tool execution). Still call the callback
    // to avoid hanging the tool chain.
    tab_setup_phase_->SetEndDetails("task_removed");
    tab_setup_phase_.reset();
    std::move(callback).Run(task_tab_handle_);
    return;
  }
  task->AddTab(
      task_tab_handle_, /*stop_task_on_detach=*/true,
      base::BindOnce(&ContentAgentToolProvider::TabAddedToTask,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
  task->Resume();
}

void ContentAgentToolProvider::TabAddedToTask(
    base::OnceCallback<void(tabs::TabHandle)> callback,
    actor::mojom::ActionResultPtr result) {
  if (tab_setup_phase_) {
    tab_setup_phase_->SetEndDetails(actor::ToDebugString(*result));
    tab_setup_phase_.reset();
  }
  std::move(callback).Run(task_tab_handle_);
}

void ContentAgentToolProvider::ExecuteActions(
    optimization_guide::proto::Actions actions,
    Tool::UseToolCallback callback) {
  actor::BuildToolRequestResult requests = actor::BuildToolRequest(actions);

  if (!requests.has_value()) {
    DLOG(ERROR) << "Action Failed to convert BrowserAction to ToolRequests.";
    Log(kAgentEventActionResult, "invalid_parameters");
    std::move(callback).Run(CreateContentBlocksForText(
                                "Error: action failed - incorrect parameters"),
                            {});
    return;
  }

  actuation_phase_ = std::make_unique<AgentPhase>(
      this, kAgentPhaseActuation,
      base::StrCat({"actions=", base::ToString(actions.actions_size())}));

  actor_service_->PerformActions(
      actor::TaskId(actions.task_id()), std::move(requests.value()),
      actor::ActorTaskMetadata(),
      base::BindOnce(&ContentAgentToolProvider::OnActionsFinished,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void ContentAgentToolProvider::OnActorTaskStateChanged(actor::ActorTask& task) {
  DVLOG(4) << __func__ << " " << task.GetState();
  if (!task_id_.is_null() && task_id_ == task.id() &&
      kActorStatesToNotify.contains(task.GetState())) {
    NotifyTaskStateChanged();
  }
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
    std::vector<actor::ActionResultWithLatencyInfo> action_results,
    actor::TabObservationStrategy observation_strategy) {
  actor::mojom::ActionResultCode result_code =
      actor::mojom::ActionResultCode::kOk;
  std::optional<size_t> index_of_failed_action;
  ExtractErrorResult(action_results, &result_code, index_of_failed_action);

  // Per action, three things the actor produces that the model never sees: its
  // own timing, its English failure message, and the observation policies the
  // loop ignores - it always extracts, never screenshots. Recording them makes
  // the cost of discarding them visible. The policies are read from each result
  // rather than from `observation_strategy`, which is only locked - and so only
  // readable - on the paths where actions actually ran.
  for (size_t i = 0; i < action_results.size(); ++i) {
    const auto& action_result = action_results[i];
    Log(kAgentEventActionResult,
        base::StrCat(
            {"index=", base::ToString(i), " duration_ms=",
             base::ToString((action_result.end_time - action_result.start_time)
                                .InMilliseconds()),
             " screenshot_policy=",
             base::ToString(action_result.result->screenshot_policy),
             " extraction_policy=",
             base::ToString(action_result.result->page_content_policy), " ",
             actor::ToDebugString(*action_result.result)}));
  }

  if (actuation_phase_) {
    actuation_phase_->SetEndDetails(base::StrCat(
        {"result=", base::ToString(result_code), " failed_index=",
         index_of_failed_action ? base::ToString(*index_of_failed_action)
                                : "none"}));
    actuation_phase_.reset();
  }

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
          CreateContentBlocksForText("Error: tab is no longer open"), {});
      return;
    }

    observation_phase_ = std::make_unique<AgentPhase>(
        this, kAgentPhaseObservation, "mode=actionable_elements");

    optimization_guide::GetAIPageContent(
        task_tab_handle_.Get()->GetContents(), std::move(options),
        base::BindOnce(&ContentAgentToolProvider::ReceivedAnnotatedPageContent,
                       weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
  } else if (result_code ==
             actor::mojom::ActionResultCode::kEmptyActionSequence) {
    DLOG(ERROR) << "Actions were empty";
    std::move(callback).Run(CreateContentBlocksForText(
                                "Error: action failed - no actions specified"),
                            {});
  } else {
    DLOG(ERROR) << "Action failed, see actor.mojom for result code meaning: "
                << result_code;
    std::move(callback).Run(CreateContentBlocksForText("Error: action failed"),
                            {});
  }
}

void ContentAgentToolProvider::ReceivedAnnotatedPageContent(
    Tool::UseToolCallback callback,
    optimization_guide::AIPageContentResultOrError content) {
  if (!content.has_value()) {
    DLOG(ERROR) << "Error getting page content";
    EndObservationPhase("extraction_failed");
    std::move(callback).Run(
        CreateContentBlocksForText("Error: could not get page content"), {});
    return;
  }

  auto apc = content->proto;

  if (!apc.has_root_node()) {
    DLOG(ERROR) << "No root node";
    EndObservationPhase("no_root_node");
    std::move(callback).Run(CreateContentBlocksForText("No root node"), {});
    return;
  }

  std::vector<mojom::ContentBlockPtr> content_blocks;
  {
    AgentPhase serialization_phase(this, kAgentPhaseObservationSerialization,
                                   "");
    content_blocks = ConvertAnnotatedPageContentToBlocks(apc);
  }

  EndObservationPhase(
      base::StrCat({"chars=", base::ToString(CountTextChars(content_blocks)),
                    " blocks=", base::ToString(content_blocks.size())}));

  content_blocks.insert(
      content_blocks.begin(),
      std::move(CreateContentBlocksForText("Action successful")[0]));
  std::move(callback).Run(std::move(content_blocks), {});
}

void ContentAgentToolProvider::EndObservationPhase(std::string_view details) {
  if (observation_phase_) {
    observation_phase_->SetEndDetails(std::string(details));
    observation_phase_.reset();
  }
}

}  // namespace ai_chat
