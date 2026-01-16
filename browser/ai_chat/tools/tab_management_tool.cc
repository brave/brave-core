// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tools/tab_management_tool.h"

#include <algorithm>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/debug/dump_without_crashing.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/memory/raw_ptr.h"
#include "base/notreached.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/sequenced_task_runner.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/tools/tool_input_properties.h"
#include "brave/components/ai_chat/core/browser/tools/tool_utils.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface_iterator.h"
#include "chrome/browser/ui/tabs/tab_enums.h"
#include "chrome/browser/ui/tabs/tab_group_model.h"
#include "chrome/browser/ui/tabs/tab_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "components/tab_groups/tab_group_color.h"
#include "components/tab_groups/tab_group_id.h"
#include "components/tab_groups/tab_group_visual_data.h"
#include "components/tabs/public/tab_group.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "services/data_decoder/public/cpp/data_decoder.h"
#include "ui/base/base_window.h"

static_assert(BUILDFLAG(ENABLE_TAB_MANAGEMENT_TOOL));
static_assert(!BUILDFLAG(IS_ANDROID));

namespace ai_chat {

namespace {

// Fallback timeout for tab removal operations.
// Tab closure can involve user interaction via unload handlers.
// If users close within the timeout, then we'll respond to the tool request
// with the closed tab. If the tab isn't removed within the timeout, then we'll
// respond with the tab still present.
constexpr base::TimeDelta kTabRemovalTimeout = base::Seconds(10);

// Returns a sorted, de-duplicated list of indices that are valid for the
// provided TabStripModel. Any indices outside of the current tab bounds are
// dropped.
std::vector<int> MakeSortedUniqueValidIndices(const std::vector<int>& indices,
                                              const TabStripModel* tab_strip) {
  CHECK(tab_strip);
  std::vector<int> filtered_indices;
  const int tab_count = tab_strip->count();
  filtered_indices.reserve(indices.size());
  for (int index : indices) {
    if (index >= 0 && index < tab_count) {
      filtered_indices.push_back(index);
    }
  }

  if (filtered_indices.empty()) {
    return filtered_indices;
  }

  std::sort(filtered_indices.begin(), filtered_indices.end());
  filtered_indices.erase(
      std::unique(filtered_indices.begin(), filtered_indices.end()),
      filtered_indices.end());
  return filtered_indices;
}

bool GetTabGroupColorId(const std::string_view group_color,
                        tab_groups::TabGroupColorId& color_out) {
  if (group_color == "grey") {
    color_out = tab_groups::TabGroupColorId::kGrey;
  } else if (group_color == "blue") {
    color_out = tab_groups::TabGroupColorId::kBlue;
  } else if (group_color == "red") {
    color_out = tab_groups::TabGroupColorId::kRed;
  } else if (group_color == "yellow") {
    color_out = tab_groups::TabGroupColorId::kYellow;
  } else if (group_color == "green") {
    color_out = tab_groups::TabGroupColorId::kGreen;
  } else if (group_color == "pink") {
    color_out = tab_groups::TabGroupColorId::kPink;
  } else if (group_color == "purple") {
    color_out = tab_groups::TabGroupColorId::kPurple;
  } else if (group_color == "cyan") {
    color_out = tab_groups::TabGroupColorId::kCyan;
  } else if (group_color == "orange") {
    color_out = tab_groups::TabGroupColorId::kOrange;
  } else {
    return false;
  }
  return true;
}

std::string GetTabGroupColorString(tab_groups::TabGroupColorId color) {
  switch (color) {
    case tab_groups::TabGroupColorId::kGrey:
      return "grey";
    case tab_groups::TabGroupColorId::kBlue:
      return "blue";
    case tab_groups::TabGroupColorId::kRed:
      return "red";
    case tab_groups::TabGroupColorId::kYellow:
      return "yellow";
    case tab_groups::TabGroupColorId::kGreen:
      return "green";
    case tab_groups::TabGroupColorId::kPink:
      return "pink";
    case tab_groups::TabGroupColorId::kPurple:
      return "purple";
    case tab_groups::TabGroupColorId::kCyan:
      return "cyan";
    case tab_groups::TabGroupColorId::kOrange:
      return "orange";
    default:
      return "unknown";
  }
}

// Helper class that waits for tabs to be destroyed after
// close operations. This is necessary because tab closing is asynchronous -
// CloseWebContentsAt() initiates closure but the WebContents destruction may
// be delayed by e.g. unload handlers. If we return results immediately, the tab
// list may still show the tabs as present, even though they've been marked for
// closure.
//
// Usage: TabsClosedWaiter::Run(tab_handles, callback, timeout)
//
// The waiter will invoke the callback either when all tabs have been destroyed
// or when the timeout expires, whichever comes first. This is a self-owned
// object that deletes itself after completion.
class TabsClosedWaiter : public content::WebContentsObserver {
 public:
  static void Run(std::vector<tabs::TabHandle> handles,
                  base::OnceClosure on_done,
                  base::TimeDelta fallback_timeout = kTabRemovalTimeout) {
    auto* waiter = new TabsClosedWaiter(std::move(handles), std::move(on_done),
                                        fallback_timeout);
    waiter->Start();
  }

  ~TabsClosedWaiter() override = default;

 private:
  class TabObserver : public content::WebContentsObserver {
   public:
    TabObserver(TabsClosedWaiter* waiter, content::WebContents* web_contents)
        : content::WebContentsObserver(web_contents), waiter_(waiter) {}

    // It should be safe to observe WebContentsDestroyed because we don't
    // reference the WebContents anywhere and the sole purpose of this class is
    // to wait for this method and then get destroyed.
    void WebContentsDestroyed() override { waiter_->CheckAndMaybeFinish(); }

   private:
    TabsClosedWaiter* waiter_;
  };

  TabsClosedWaiter(std::vector<tabs::TabHandle> handles,
                   base::OnceClosure on_done,
                   base::TimeDelta timeout)
      : handles_(std::move(handles)),
        on_done_(std::move(on_done)),
        timeout_(timeout) {}

  void Start() {
    if (handles_.empty()) {
      ForceFinish();
      return;
    }
    // Check whenever one of the provided tabs closes
    tab_observers_.reserve(handles_.size());
    for (tabs::TabHandle handle : handles_) {
      if (auto* tab = handle.Get()) {
        tab_observers_.push_back(
            std::make_unique<TabObserver>(this, tab->GetContents()));
      }
    }

    // Immediate check next task
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindOnce(&TabsClosedWaiter::CheckAndMaybeFinish,
                                  weak_factory_.GetWeakPtr()));
    // Fallback timer
    timer_.Start(FROM_HERE, timeout_,
                 base::BindOnce(&TabsClosedWaiter::ForceFinish,
                                weak_factory_.GetWeakPtr()));
  }

  void CheckAndMaybeFinish() {
    // Call finish if all tabs are closed
    for (tabs::TabHandle handle : handles_) {
      if (handle.Get()) {
        return;
      }
    }
    ForceFinish();
  }

  void ForceFinish() {
    if (finished_) {
      return;
    }
    finished_ = true;
    timer_.Stop();
    tab_observers_.clear();

    // Post the callback on the next task, just like TabManagementTool in order
    // to best-effort ensure that any active window change is reflected.
    if (on_done_) {
      base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
          FROM_HERE, std::move(on_done_));
    }

    // Self-delete on next task
    base::SequencedTaskRunner::GetCurrentDefault()->DeleteSoon(FROM_HERE, this);
  }

  std::vector<tabs::TabHandle> handles_;
  std::vector<std::unique_ptr<TabObserver>> tab_observers_;
  base::OnceClosure on_done_;
  base::OneShotTimer timer_;
  base::TimeDelta timeout_;
  bool finished_ = false;
  base::WeakPtrFactory<TabsClosedWaiter> weak_factory_{this};
};

}  // namespace

TabManagementTool::TabManagementTool(Profile* profile) : profile_(profile) {}

TabManagementTool::~TabManagementTool() = default;

std::vector<tabs::TabInterface*> TabManagementTool::GetTabsFromHandles(
    const std::vector<int>& handles) {
  std::vector<tabs::TabInterface*> tabs;
  tabs.reserve(handles.size());

  for (int handle_id : handles) {
    tabs::TabHandle handle = tabs::TabHandle(handle_id);
    if (auto* tab = handle.Get()) {
      tabs.push_back(tab);
    }
  }

  return tabs;
}

std::string_view TabManagementTool::Name() const {
  return mojom::kTabManagementToolName;
}

std::string_view TabManagementTool::Description() const {
  return "Manage browser tabs - list, move, close tabs and manage tab "
         "groups. "
         "This tool can list all open tabs with their window, group, URL and "
         "title information, "
         "move tabs or entire groups between windows or positions,"
         "close tabs, "
         "and create or modify tab groups. Groups are per-window, so make any "
         "moves before grouping. Use window_id=-1 to move tabs/groups to a new "
         "window. "
         "Use move_group_id to move an entire group at once (cannot be "
         "combined with tab_ids in the same request). "
         "When moving tabs, the active tab state is preserved - if you move "
         "the active tab, "
         "it remains active in its new location. "
         "After each operation, the updated tab list is returned. "
         "To move both individual tabs and groups, make separate move "
         "requests. "
         "If possible and you know the operations and IDs ahead of time, "
         "try to make multiple parallel requests to use this tool without "
         "waiting for the answer. Every time this tool needs to be used, if "
         "there has been a user message since the last time it has been used "
         "then the list of tabs should be read again as there has been a gap "
         "in time and the list might have changed significantly.\nThe list "
         "operation should be the first operation as every other operation "
         "requires IDs found in the list of tabs and windows. You must provide "
         "the plan parameter when using the list operation.";
}

std::optional<base::Value::Dict> TabManagementTool::InputProperties() const {
  return CreateInputProperties(
      {{"action", StringProperty("The action to perform",
                                 std::vector<std::string>{
                                     "list", "move", "close", "create_group",
                                     "update_group", "remove_from_group"})},
       {"plan",
        StringProperty("Human readable plan of what the assistant intends to "
                       "do with the list of tabs and with the tab management "
                       "tool. This should be provided during the very first "
                       "list operation in a conversation and allows the user "
                       "to approve or deny the tab management operations.")},
       {"tab_ids",
        ArrayProperty("List of tab IDs to operate on (for move, "
                      "close, create_group, remove_from_group). Cannot be used "
                      "with move_group_id.",
                      IntegerProperty("Tab ID"))},
       {"move_group_id",
        StringProperty("Group ID to move entirely (for move operation). "
                       "Mutually exclusive with tab_ids - use separate "
                       "requests to move both individual tabs and groups.")},
       {"window_id",
        IntegerProperty(
            "Target window ID (for move operation). Use -1 to create a new "
            "window. If group_id is provided, this will be inferred from that "
            "group's window and window_id should not be provided.")},
       {"group_id",
        StringProperty("Target group ID for update_group or move (for move "
                       "operation window ID will be inferred by the group "
                       "windowand is mutually exclusive) or group to update")},
       {"index",
        IntegerProperty("Target index position (for move operations)")},
       {"group_title", StringProperty("Title for new or updated group")},
       {"group_color",
        StringProperty(
            "Color for new or updated group",
            std::vector<std::string>{"grey", "blue", "red", "yellow", "green",
                                     "pink", "purple", "cyan", "orange"})},
       {"add_to_end", BooleanProperty("Add tabs to end of group instead of "
                                      "beginning (for move to group)")}});
}

std::optional<std::vector<std::string>> TabManagementTool::RequiredProperties()
    const {
  return std::vector<std::string>{"action"};
}

bool TabManagementTool::IsAgentTool() const {
  return true;
}

std::variant<bool, mojom::PermissionChallengePtr>
TabManagementTool::RequiresUserInteractionBeforeHandling(
    const mojom::ToolUseEvent& tool_use) const {
  if (user_has_granted_permission_) {
    return false;
  }
  // Provide PermissionChallenge only if input is valid and we were provided
  // with a plan. If it isn't valid, it will be rejected by UseTool.
  auto input = base::JSONReader::ReadDict(tool_use.arguments_json,
                                          base::JSON_PARSE_CHROMIUM_EXTENSIONS);

  if (!input.has_value()) {
    return false;
  }

  const auto* plan = input->FindString("plan");
  if (!plan || plan->empty()) {
    return false;
  }

  return mojom::PermissionChallenge::New(std::nullopt, *plan);
}

void TabManagementTool::UserPermissionGranted(const std::string& tool_use_id) {
  user_has_granted_permission_ = true;
}

void TabManagementTool::UseTool(const std::string& input_json,
                                UseToolCallback callback) {
  // TODO: keep sidebar open to a AI Chat conversation if it's open in the
  // active window so that user can follow along. Or open in a full page if
  // we're going to remove the active window with the active conversation.

  auto input = base::JSONReader::ReadDict(input_json,
                                          base::JSON_PARSE_CHROMIUM_EXTENSIONS);

  if (!input.has_value()) {
    std::move(callback).Run(
        CreateContentBlocksForText("Failed to parse input JSON. Please provide "
                                   "valid JSON with an 'action' field."));
    return;
  }

  const auto& dict = input.value();

  // Verify we have permission
  if (!user_has_granted_permission_) {
    // Report to LLM as to why we couldn't grant permission yet
    auto* plan = dict.FindString("plan");
    if (!plan || plan->empty()) {
      // No plan provided, so we can't grant permission
      std::move(callback).Run(CreateContentBlocksForText(
          "No plan provided which the user will be asked to approve. Provide a "
          "plan for the first use of this tool."));
      return;
    }
    // If valid permission, we shouldn't be here
    base::debug::DumpWithoutCrashing();
    // Still provide output so the conversation can proceed without running this
    // tool.
    std::move(callback).Run(CreateContentBlocksForText("Unknown error"));
    return;
  }

  CHECK(user_has_granted_permission_);

  const auto* action = dict.FindString("action");

  if (!action) {
    std::move(callback).Run(CreateContentBlocksForText(
        "Missing required 'action' field. Must be one of: list, move, close, "
        "create_group, update_group, remove_from_group"));
    return;
  }

  if (*action == "list") {
    HandleListTabs(std::move(callback));
  } else if (*action == "move") {
    HandleMoveTabs(std::move(callback), dict);
  } else if (*action == "close") {
    HandleCloseTabs(std::move(callback), dict);
  } else if (*action == "create_group") {
    HandleCreateGroup(std::move(callback), dict);
  } else if (*action == "update_group") {
    HandleUpdateGroup(std::move(callback), dict);
  } else if (*action == "remove_from_group") {
    HandleRemoveFromGroup(std::move(callback), dict);
  } else {
    std::move(callback).Run(CreateContentBlocksForText(
        "Invalid action. Must be one of: list, move, close, "
        "create_group, update_group, remove_from_group"));
  }
}

base::Value::Dict TabManagementTool::GenerateTabList() const {
  base::Value::Dict result;
  base::Value::List windows;

  // Iterate through all browser windows for this profile
  for (BrowserWindowInterface* browser : GetAllBrowserWindowInterfaces()) {
    if (browser->GetProfile() != profile_) {
      continue;
    }

    base::Value::Dict window_info;
    window_info.Set("window_id", browser->GetSessionID().id());
    window_info.Set("is_active", browser->IsActive());

    TabStripModel* tab_strip = browser->GetTabStripModel();
    if (!tab_strip) {
      continue;
    }

    base::Value::List tabs;
    base::Value::Dict groups;

    // First, collect group information
    if (tab_strip->SupportsTabGroups() && tab_strip->group_model()) {
      for (const auto& group_id : tab_strip->group_model()->ListTabGroups()) {
        const TabGroup* group = tab_strip->group_model()->GetTabGroup(group_id);
        if (group) {
          base::Value::Dict group_info;
          const auto* visual_data = group->visual_data();
          group_info.Set("title", base::UTF16ToUTF8(visual_data->title()));
          group_info.Set("color", GetTabGroupColorString(visual_data->color()));
          group_info.Set("is_collapsed", tab_strip->IsGroupCollapsed(group_id));

          groups.Set(group_id.ToString(), std::move(group_info));
        }
      }
    }

    // Collect tab information
    for (int i = 0; i < tab_strip->count(); ++i) {
      tabs::TabInterface* tab = tab_strip->GetTabAtIndex(i);
      if (!tab) {
        continue;
      }

      content::WebContents* web_contents = tab->GetContents();

      base::Value::Dict tab_info;
      // Use TabHandle instead of extension tab ID
      tab_info.Set("tab_id", tab->GetHandle().raw_value());
      tab_info.Set("index", i);
      tab_info.Set("url", web_contents->GetURL().spec());
      tab_info.Set("title", base::UTF16ToUTF8(web_contents->GetTitle()));
      tab_info.Set("is_active", i == tab_strip->active_index());
      tab_info.Set("is_pinned", tab_strip->IsTabPinned(i));

      // Add group information if tab is in a group
      auto group_id = tab_strip->GetTabGroupForTab(i);
      if (group_id.has_value()) {
        tab_info.Set("group_id", group_id->ToString());
      }

      tabs.Append(std::move(tab_info));
    }

    window_info.Set("tabs", std::move(tabs));
    window_info.Set("groups", std::move(groups));
    window_info.Set("active_tab_index", tab_strip->active_index());

    windows.Append(std::move(window_info));
  }

  result.Set("windows", std::move(windows));
  return result;
}

void TabManagementTool::SendResultWithTabList(UseToolCallback callback,
                                              base::Value::Dict result) {
  base::Value::Dict tab_list = GenerateTabList();
  result.Set("windows", std::move(*tab_list.FindList("windows")));

  std::string json_output;
  base::JSONWriter::WriteWithOptions(
      result, base::JSONWriter::OPTIONS_PRETTY_PRINT, &json_output);
  std::move(callback).Run(CreateContentBlocksForText(json_output));
}

void TabManagementTool::PostTaskSendResultWithTabList(
    UseToolCallback callback,
    base::Value::Dict result,
    std::optional<tabs::TabHandle> active_moved_tab) {
  // If the operation determined we need to re-activate a tab and its window,
  // possibly due to moving the previously active tab to a new window,
  // then we can do it immediately as tab strip creation and movements
  // are immediately ready.
  if (active_moved_tab.has_value()) {
    if (tabs::TabInterface* tab = active_moved_tab->Get();
        BrowserWindowInterface* window = tab->GetBrowserWindowInterface()) {
      window->GetWindow()->Activate();
      window->GetTabStripModel()->ActivateTabAt(
          window->GetTabStripModel()->GetIndexOfTab(tab));
    }
  }
  // We do need to collect the result on the next task since any resulting
  // browser closure won't be reflected immediately.
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&TabManagementTool::SendResultWithTabList,
                                weak_ptr_factory_.GetWeakPtr(),
                                std::move(callback), std::move(result)));
}

void TabManagementTool::HandleListTabs(UseToolCallback callback) {
  base::Value::Dict result = GenerateTabList();

  std::string json_output;
  base::JSONWriter::WriteWithOptions(
      result, base::JSONWriter::OPTIONS_PRETTY_PRINT, &json_output);

  std::move(callback).Run(CreateContentBlocksForText(json_output));
}

void TabManagementTool::HandleMoveTabs(UseToolCallback callback,
                                       const base::Value::Dict& params) {
  const auto* tab_ids = params.FindList("tab_ids");
  const auto* group_to_move = params.FindString("move_group_id");

  // Check for mutual exclusivity first - if both fields are present, reject
  // regardless of content
  if (tab_ids && group_to_move) {
    std::move(callback).Run(CreateContentBlocksForText(
        "Cannot provide both 'tab_ids' and 'move_group_id' in the same "
        "request. "
        "Use separate requests to move individual tabs and groups."));
    return;
  }

  // Either tab_ids or move_group_id must be provided with valid content
  bool has_valid_tab_ids = tab_ids && !tab_ids->empty();
  bool has_valid_group_id = group_to_move && !group_to_move->empty();

  if (!has_valid_tab_ids && !has_valid_group_id) {
    std::move(callback).Run(CreateContentBlocksForText(
        "Missing 'tab_ids' array or 'move_group_id' for move operation. "
        "Provide either specific tab IDs or a group ID to move."));
    return;
  }

  // Extract common parameters
  const auto window_id = params.FindInt("window_id");
  const auto index = params.FindInt("index");

  // Dispatch to specialized handlers
  if (has_valid_group_id) {
    HandleMoveGroup(std::move(callback), *group_to_move, window_id, index);
  } else {
    // Convert tab_ids to vector<int>
    std::vector<int> tab_handles;
    for (const auto& tab_id_value : *tab_ids) {
      if (tab_id_value.is_int()) {
        tab_handles.push_back(tab_id_value.GetInt());
      }
    }

    const auto* group_id_str = params.FindString("group_id");
    const auto add_to_end = params.FindBool("add_to_end").value_or(false);

    HandleMoveIndividualTabs(std::move(callback), tab_handles, window_id, index,
                             group_id_str, add_to_end);
  }
}

BrowserWindowInterface* TabManagementTool::FindOrCreateTargetWindow(
    std::optional<int> window_id,
    std::string* error_out,
    bool* did_create_window_out) {
  if (!window_id.has_value()) {
    return nullptr;  // Use source window
  }

  if (*window_id < -1) {
    *error_out = "Invalid window ID";
    return nullptr;
  }

  if (*window_id == -1) {
    // Create a new window
    Browser::CreateParams create_params(profile_, true);
    BrowserWindowInterface* target_browser = Browser::Create(create_params);
    if (!target_browser) {
      *error_out = "Failed to create new browser window";
      return nullptr;
    }
    // Don't activate the window otherwise the user will lose
    // their active conversation tab.
    target_browser->GetWindow()->ShowInactive();
    *did_create_window_out = true;
    // Get BrowserWindowInterface from session ID of the new browser
    return BrowserWindowInterface::FromSessionID(
        target_browser->GetSessionID());
  }

  // Find existing window using BrowserWindowInterface
  SessionID target_session_id = SessionID::FromSerializedValue(*window_id);
  BrowserWindowInterface* target_window =
      BrowserWindowInterface::FromSessionID(target_session_id);

  if (!target_window) {
    *error_out =
        "Target window not found with ID: " + base::NumberToString(*window_id);
    return nullptr;
  }

  // Verify the profile matches
  if (target_window->GetProfile() != profile_) {
    *error_out = "Target window belongs to different profile";
    return nullptr;
  }

  if (target_window->GetType() != BrowserWindowInterface::Type::TYPE_NORMAL) {
    *error_out = "Target window is not a normal window";
    return nullptr;
  }

  return target_window;
}

BrowserWindowInterface* TabManagementTool::FindWindowWithGroup(
    const std::string& group_id,
    TabGroup** group_out) {
  for (BrowserWindowInterface* browser : GetAllBrowserWindowInterfaces()) {
    if (browser->GetProfile() != profile_ ||
        browser->GetType() != BrowserWindowInterface::Type::TYPE_NORMAL) {
      continue;
    }
    TabStripModel* tab_strip = browser->GetTabStripModel();
    if (!tab_strip->SupportsTabGroups() || !tab_strip->group_model()) {
      continue;
    }
    for (const auto& g : tab_strip->group_model()->ListTabGroups()) {
      if (g.ToString() == group_id) {
        *group_out = tab_strip->group_model()->GetTabGroup(g);
        return BrowserWindowInterface::FromSessionID(browser->GetSessionID());
      }
    }
  }
  return nullptr;
}

bool TabManagementTool::ValidateMoveTarget(
    BrowserWindowInterface* target_window,
    std::optional<int> index,
    std::string* error) const {
  CHECK(target_window) << "Don't call this function with a nullptr";

  TabStripModel* tab_strip = target_window->GetTabStripModel();
  if (!tab_strip) {
    *error = "Target window has no tab strip";
    return false;
  }

  if (target_window->GetType() != BrowserWindowInterface::Type::TYPE_NORMAL) {
    *error = "Target window is not a normal window";
    return false;
  }

  if (!tab_strip->SupportsTabGroups()) {
    *error = "Target window does not support tab groups";
    return false;
  }

  // Index validation is optional - if not provided, tabs go to end
  if (index.has_value()) {
    int target_index = *index;
    if (target_index < 0 || target_index > tab_strip->count()) {
      // This is allowed - will be clamped later
    }
  }

  return true;
}

void TabManagementTool::HandleMoveGroup(UseToolCallback callback,
                                        const std::string& group_id,
                                        std::optional<int> window_id,
                                        std::optional<int> index) {
  TabGroup* group;
  BrowserWindowInterface* source_window = FindWindowWithGroup(group_id, &group);

  if (!source_window || !group) {
    std::move(callback).Run(
        CreateContentBlocksForText("Group not found with ID: " + group_id));
    return;
  }

  TabStripModel* source_tab_strip = source_window->GetTabStripModel();

  std::string error;
  bool did_create_window = false;
  BrowserWindowInterface* target_window =
      FindOrCreateTargetWindow(window_id, &error, &did_create_window);
  if (!error.empty()) {
    std::move(callback).Run(CreateContentBlocksForText(error));
    return;
  }

  TabStripModel* target_tab_strip = nullptr;
  if (target_window) {
    target_tab_strip = target_window->GetTabStripModel();
    if (!ValidateMoveTarget(target_window, index, &error)) {
      std::move(callback).Run(CreateContentBlocksForText(error));
      return;
    }
  } else {
    // Use source tab strip
    target_tab_strip = source_tab_strip;
  }

  if (source_tab_strip == target_tab_strip) {
    // Same tab strip - use efficient MoveGroupTo
    // Follow the exact logic from the Extensions API for same-window group
    // moves
    gfx::Range tabs_in_group = group->ListTabs();
    const int start_index = tabs_in_group.start();
    int target_index = index.value_or(target_tab_strip->count());

    // The index should be clamped to positions in the tab strip the whole group
    // can occupy, i.e. count() - (num of tabs in group being moved).
    const int size_after_group_removed =
        source_tab_strip->count() - tabs_in_group.length();
    target_index = std::clamp(target_index, 0, size_after_group_removed);

    // Don't move if already at target position
    if (target_index == start_index) {
      // Group is already at target position, just return success immediately
      std::move(callback).Run(
          CreateContentBlocksForText("Group already at target position"));
      return;
    }

    source_tab_strip->MoveGroupTo(group->id(), target_index);

    base::Value::Dict result;
    result.Set("message", "Successfully moved group within same window");
    PostTaskSendResultWithTabList(std::move(callback), std::move(result));
  } else {
    // Cross-window move - use detach/attach
    int target_index = index.value_or(target_tab_strip->count());
    target_index = std::min(target_index, target_tab_strip->count());

    // See if we need to re-active the target window
    std::optional<tabs::TabHandle> tab_to_reactivate = std::nullopt;
    if (source_window->GetWindow()->IsActive()) {
      auto* active_tab = source_tab_strip->GetActiveTab();
      if (active_tab) {
        auto active_tab_group_id = active_tab->GetGroup();
        if (active_tab_group_id && active_tab_group_id == group->id()) {
          // We need to re-activate this tab and the target window
          tab_to_reactivate = active_tab->GetHandle();
        }
      }
    }

    std::unique_ptr<DetachedTabCollection> detached_group =
        source_tab_strip->DetachTabGroupForInsertion(group->id());
    target_tab_strip->InsertDetachedTabGroupAt(std::move(detached_group),
                                               target_index);

    base::Value::Dict result;
    result.Set("message", "Successfully moved group to different window");
    if (did_create_window) {
      result.Set("new_window_id", target_window->GetSessionID().id());
    }
    PostTaskSendResultWithTabList(std::move(callback), std::move(result),
                                  tab_to_reactivate);
  }
}

void TabManagementTool::HandleMoveIndividualTabs(
    UseToolCallback callback,
    const std::vector<int>& tab_handles,
    std::optional<int> window_id,
    std::optional<int> index,
    const std::string* group_id,
    bool add_to_end) {
  if (group_id && !group_id->empty() && window_id.has_value()) {
    std::move(callback).Run(CreateContentBlocksForText(
        "Cannot provide both a target 'group_id' and 'window_id' in the same "
        "request. 'group_id' implies a target window that the group is in, or "
        "use separate requests to move individual tabs and groups."));
    return;
  }

  std::string error;
  BrowserWindowInterface* target_window = nullptr;

  // Get tabs from handles
  std::vector<tabs::TabInterface*> tabs = GetTabsFromHandles(tab_handles);
  if (tabs.empty()) {
    std::move(callback).Run(
        CreateContentBlocksForText("No valid tabs found to move"));
    return;
  }

  bool did_create_window = false;
  if (window_id.has_value()) {
    target_window =
        FindOrCreateTargetWindow(window_id, &error, &did_create_window);
    if (!error.empty()) {
      std::move(callback).Run(CreateContentBlocksForText(error));
      return;
    }
  }

  // Parse target group if specified
  TabGroup* target_group = nullptr;
  if (group_id && !group_id->empty()) {
    target_window = FindWindowWithGroup(*group_id, &target_group);
    if (!target_window) {
      std::move(callback).Run(
          CreateContentBlocksForText("Group not found with ID: " + *group_id));
      return;
    }
  }

  // If no target specified, use the first tab's window
  if (!target_window) {
    target_window = tabs[0]->GetBrowserWindowInterface();
    if (!target_window) {
      std::move(callback).Run(CreateContentBlocksForText(
          "Could not determine target window for tab move"));
      return;
    }
  }

  if (!ValidateMoveTarget(target_window, index, &error)) {
    std::move(callback).Run(CreateContentBlocksForText(error));
    return;
  }

  TabStripModel* target_tab_strip = target_window->GetTabStripModel();
  int target_index = index.value_or(target_tab_strip->count());
  target_index = std::min(target_index, target_tab_strip->count());

  // Collect tabs to move with their source tab strips
  std::vector<std::pair<content::WebContents*, TabStripModel*>> tabs_to_move;
  std::optional<tabs::TabHandle> active_moved_tab = std::nullopt;

  for (auto* tab : tabs) {
    content::WebContents* web_contents = tab->GetContents();
    if (!web_contents) {
      continue;
    }

    if (auto* source_window = tab->GetBrowserWindowInterface()) {
      if (source_window->GetProfile() != profile_) {
        continue;
      }
      TabStripModel* source_tab_strip = source_window->GetTabStripModel();
      if (!source_tab_strip) {
        continue;
      }
      tabs_to_move.push_back({web_contents, source_tab_strip});
      // Keep a reference to the active tab in the active window so that
      // we can restore it after the move.
      if (source_window->GetWindow()->IsActive() &&
          source_tab_strip->GetActiveTab() &&
          source_tab_strip->GetActiveTab()->GetHandle() == tab->GetHandle()) {
        active_moved_tab = tab->GetHandle();
      }
    }
  }

  if (tabs_to_move.empty()) {
    std::move(callback).Run(
        CreateContentBlocksForText("No valid tabs found to move"));
    return;
  }

  // Perform the moves
  std::vector<int> moved_indices;
  int current_target_index = target_index;

  // Clamp the target index to a valid location
  int min_target_index = 0;
  int max_target_index = target_tab_strip->count();

  const bool has_group = target_group != nullptr;

  if (has_group) {
    gfx::Range group_indexes = target_group->ListTabs();
    min_target_index = group_indexes.start();
    max_target_index = group_indexes.end();
  }

  current_target_index =
      std::clamp(current_target_index, min_target_index, max_target_index);

  for (const auto& [web_contents, source_tab_strip] : tabs_to_move) {
    int source_index = source_tab_strip->GetIndexOfWebContents(web_contents);

    if (source_index == TabStripModel::kNoTab) {
      continue;
    }

    if (source_tab_strip != target_tab_strip) {
      // Cross-window move
      std::unique_ptr<tabs::TabModel> detached_tab =
          source_tab_strip->DetachTabAtForInsertion(source_index);

      if (detached_tab) {
        int inserted_index = target_tab_strip->InsertDetachedTabAt(
            current_target_index, std::move(detached_tab), ADD_NONE,
            has_group ? std::make_optional(target_group->id()) : std::nullopt);
        moved_indices.push_back(inserted_index);
      }
    } else {
      // Same tab strip move
      int new_index = target_tab_strip->MoveWebContentsAt(
          source_index, current_target_index, false,
          has_group ? std::make_optional(target_group->id()) : std::nullopt);
      moved_indices.push_back(new_index);
    }
  }

  // Add to existing group if specified
  if (has_group && !moved_indices.empty()) {
    std::vector<int> valid_indices =
        MakeSortedUniqueValidIndices(moved_indices, target_tab_strip);
    if (!valid_indices.empty()) {
      target_tab_strip->AddToExistingGroup(valid_indices, target_group->id(),
                                           add_to_end);
    }
  }

  const int moved_count = static_cast<int>(moved_indices.size());
  const int new_window_id = (window_id.has_value() && *window_id == -1)
                                ? target_window->GetSessionID().id()
                                : 0;

  base::Value::Dict result;
  result.Set("message", "Successfully moved " +
                            base::NumberToString(moved_count) + " tab(s)");
  if (new_window_id != 0) {
    result.Set("new_window_id", new_window_id);
  }

  PostTaskSendResultWithTabList(std::move(callback), std::move(result),
                                active_moved_tab);
}

void TabManagementTool::HandleCloseTabs(UseToolCallback callback,
                                        const base::Value::Dict& params) {
  const auto* tab_ids = params.FindList("tab_ids");

  if (!tab_ids || tab_ids->empty()) {
    std::move(callback).Run(CreateContentBlocksForText(
        "Missing or empty 'tab_ids' array for close operation"));
    return;
  }

  std::vector<std::pair<TabStripModel*, int>> tabs_to_close;
  std::vector<tabs::TabHandle> handles_to_wait_for;

  // Find all tabs to close using TabHandles
  for (const auto& tab_id_value : *tab_ids) {
    if (!tab_id_value.is_int()) {
      continue;
    }

    int handle_id = tab_id_value.GetInt();
    tabs::TabHandle handle = tabs::TabHandle(handle_id);

    if (auto* tab = handle.Get()) {
      content::WebContents* web_contents = tab->GetContents();
      if (web_contents) {
        // Must be the same profile
        if (web_contents->GetBrowserContext() != profile_) {
          continue;
        }
        // Get the browser window from the tab interface
        if (auto* browser_window = tab->GetBrowserWindowInterface()) {
          TabStripModel* tab_strip = browser_window->GetTabStripModel();
          int index = tab_strip->GetIndexOfWebContents(web_contents);
          if (index != TabStripModel::kNoTab) {
            tabs_to_close.push_back({tab_strip, index});
            handles_to_wait_for.push_back(handle);
          }
        }
      }
    }
  }

  // Sort by index in reverse order to avoid index shifting issues
  std::sort(tabs_to_close.begin(), tabs_to_close.end(),
            [](const auto& a, const auto& b) { return a.second > b.second; });

  // Close tabs
  int closed_count = 0;
  for (const auto& [tab_strip, index] : tabs_to_close) {
    // Validate index is still within bounds before closing
    if (index >= 0 && index < tab_strip->count()) {
      tab_strip->CloseWebContentsAt(index, CLOSE_USER_GESTURE);
      closed_count++;
    }
  }

  base::Value::Dict result;
  result.Set("message", "Successfully closed " +
                            base::NumberToString(closed_count) + " tab(s)");

  TabsClosedWaiter::Run(
      std::move(handles_to_wait_for),
      base::BindOnce(&TabManagementTool::PostTaskSendResultWithTabList,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback),
                     std::move(result), std::nullopt));
}

void TabManagementTool::HandleCreateGroup(UseToolCallback callback,
                                          const base::Value::Dict& params) {
  const auto* tab_ids = params.FindList("tab_ids");
  const auto* group_title = params.FindString("group_title");
  const auto* group_color = params.FindString("group_color");

  if (!tab_ids || tab_ids->empty()) {
    std::move(callback).Run(CreateContentBlocksForText(
        "Missing or empty 'tab_ids' array for create_group operation"));
    return;
  }

  // Find tabs and group them by browser using TabHandles
  std::map<TabStripModel*, std::vector<int>> browser_tabs;
  tabs::TabInterface* first_tab = nullptr;
  TabStripModel* target_tab_strip = nullptr;

  for (const auto& tab_id_value : *tab_ids) {
    if (!tab_id_value.is_int()) {
      continue;
    }

    int handle_id = tab_id_value.GetInt();
    tabs::TabHandle handle = tabs::TabHandle(handle_id);

    if (auto* tab = handle.Get()) {
      content::WebContents* web_contents = tab->GetContents();
      if (web_contents) {
        // Must be the same profile
        if (web_contents->GetBrowserContext() != profile_) {
          continue;
        }
        // Get the browser window from the tab interface
        if (auto* browser_window = tab->GetBrowserWindowInterface()) {
          TabStripModel* tab_strip = browser_window->GetTabStripModel();
          // Verify the tab is an entry in the tab strip
          int index = tab_strip->GetIndexOfWebContents(web_contents);
          if (index != TabStripModel::kNoTab) {
            // We use the window from the first tab to determine the target
            // window
            // for the group.
            if (!first_tab) {
              first_tab = tab;
              target_tab_strip = tab_strip;
            }
            browser_tabs[tab_strip].push_back(index);
          }
        }
      }
    }
  }

  if (!first_tab || !target_tab_strip || browser_tabs.empty()) {
    std::move(callback).Run(
        CreateContentBlocksForText("No valid tabs found to group"));
    return;
  }

  auto& first_indices = browser_tabs[target_tab_strip];
  std::vector<int> first_valid_indices =
      MakeSortedUniqueValidIndices(first_indices, target_tab_strip);
  // Create new group with tab(s) already in the window
  tab_groups::TabGroupId new_group_id =
      target_tab_strip->AddToNewGroup(first_valid_indices);

  // Set visual data if provided
  if (group_title || group_color) {
    const TabGroup* group =
        target_tab_strip->group_model()->GetTabGroup(new_group_id);
    if (group) {
      tab_groups::TabGroupVisualData visual_data = *group->visual_data();

      if (group_title) {
        visual_data = tab_groups::TabGroupVisualData(
            base::UTF8ToUTF16(*group_title), visual_data.color(),
            visual_data.is_collapsed());
      }

      if (group_color) {
        tab_groups::TabGroupColorId color = visual_data.color();
        if (GetTabGroupColorId(*group_color, color)) {
          visual_data = tab_groups::TabGroupVisualData(
              visual_data.title(), color, visual_data.is_collapsed());
        }
      }

      target_tab_strip->ChangeTabGroupVisuals(new_group_id, visual_data);
    }
  }

  std::vector<std::unique_ptr<tabs::TabModel>> tabs_moved_models;
  std::vector<tabs::TabHandle> tabs_moved;

  // Move all the tabs to the window with the new group, if there
  // are multiple browsers.
  if (browser_tabs.size() > 1) {
    // Move to the target browser
    for (const auto& [tab_strip, indices] : browser_tabs) {
      if (tab_strip == target_tab_strip) {
        continue;
      }
      std::vector<int> valid_indices =
          MakeSortedUniqueValidIndices(indices, tab_strip);

      if (valid_indices.empty()) {
        continue;
      }

      // Reverse the indices to avoid index shifting
      for (int index : base::Reversed(valid_indices)) {
        auto tab = tab_strip->DetachTabAtForInsertion(index);
        tabs_moved.push_back(tab->GetHandle());
        tabs_moved_models.push_back(std::move(tab));
      }
    }
  }

  // Insert tabs from other windows into the target window
  for (auto& tab_model : tabs_moved_models) {
    target_tab_strip->InsertDetachedTabAt(first_valid_indices.front(),
                                          std::move(tab_model), ADD_NONE,
                                          new_group_id);
  }

  base::Value::Dict result;
  result.Set("message", "Successfully created 1 group.");
  result.Set("created_group_id", new_group_id.ToString());

  PostTaskSendResultWithTabList(std::move(callback), std::move(result));
}

void TabManagementTool::HandleUpdateGroup(UseToolCallback callback,
                                          const base::Value::Dict& params) {
  const auto* group_id_str = params.FindString("group_id");
  const auto* group_title = params.FindString("group_title");
  const auto* group_color = params.FindString("group_color");

  if (!group_id_str || group_id_str->empty()) {
    std::move(callback).Run(CreateContentBlocksForText(
        "Missing 'group_id' for update_group operation"));
    return;
  }

  bool found = false;

  // Find and update the group
  TabGroup* group;
  if (BrowserWindowInterface* window =
          FindWindowWithGroup(*group_id_str, &group)) {
    tab_groups::TabGroupVisualData visual_data = *group->visual_data();
    if (group_title) {
      visual_data.SetTitle(base::UTF8ToUTF16(*group_title));
    }
    if (group_color) {
      tab_groups::TabGroupColorId color = visual_data.color();
      if (GetTabGroupColorId(*group_color, color)) {
        visual_data = tab_groups::TabGroupVisualData(
            visual_data.title(), color, visual_data.is_collapsed());
      }
    }
    window->GetTabStripModel()->ChangeTabGroupVisuals(group->id(), visual_data);
    found = true;
  }

  if (found) {
    base::Value::Dict result;
    result.Set("message", "Successfully updated group");
    PostTaskSendResultWithTabList(std::move(callback), std::move(result));
  } else {
    std::move(callback).Run(CreateContentBlocksForText(
        "Group not found with ID: " + *group_id_str));
  }
}

void TabManagementTool::HandleRemoveFromGroup(UseToolCallback callback,
                                              const base::Value::Dict& params) {
  const auto* tab_ids = params.FindList("tab_ids");

  if (!tab_ids || tab_ids->empty()) {
    std::move(callback).Run(CreateContentBlocksForText(
        "Missing or empty 'tab_ids' array for remove_from_group operation"));
    return;
  }

  // Find tabs and group them by browser using TabHandles
  std::map<TabStripModel*, std::vector<int>> browser_tabs;

  for (const auto& tab_id_value : *tab_ids) {
    if (!tab_id_value.is_int()) {
      continue;
    }

    int handle_id = tab_id_value.GetInt();
    tabs::TabHandle handle = tabs::TabHandle(handle_id);

    if (auto* tab = handle.Get()) {
      // Must be in a group
      if (!tab->GetGroup().has_value()) {
        continue;
      }
      content::WebContents* web_contents = tab->GetContents();
      if (web_contents) {
        // Must be the same profile
        if (web_contents->GetBrowserContext() != profile_) {
          continue;
        }
        // Get the browser window from the tab interface
        if (auto* browser_window = tab->GetBrowserWindowInterface()) {
          TabStripModel* tab_strip = browser_window->GetTabStripModel();
          int index = tab_strip->GetIndexOfWebContents(web_contents);
          if (index != TabStripModel::kNoTab) {
            browser_tabs[tab_strip].push_back(index);
          }
        }
      }
    }
  }

  if (browser_tabs.empty()) {
    std::move(callback).Run(CreateContentBlocksForText(
        "No valid tabs found to remove from groups"));
    return;
  }

  int removed_count = 0;

  // Remove tabs from groups in each browser
  for (const auto& [tab_strip, indices] : browser_tabs) {
    if (!tab_strip->SupportsTabGroups()) {
      continue;
    }

    // Validate, sort and de-duplicate indices before removing from group
    std::vector<int> valid_indices =
        MakeSortedUniqueValidIndices(indices, tab_strip);

    if (valid_indices.empty()) {
      continue;
    }

    tab_strip->RemoveFromGroup(valid_indices);
    removed_count += static_cast<int>(valid_indices.size());
  }

  base::Value::Dict result;
  result.Set("message", "Successfully removed " +
                            base::NumberToString(removed_count) +
                            " tab(s) from their groups");
  PostTaskSendResultWithTabList(std::move(callback), std::move(result));
}

}  // namespace ai_chat
