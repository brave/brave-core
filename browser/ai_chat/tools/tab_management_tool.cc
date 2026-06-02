// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tools/tab_management_tool.h"

#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/containers/adapters.h"
#include "base/containers/flat_map.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/memory/raw_ptr.h"
#include "base/notreached.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/sequenced_task_runner.h"
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
#include "chrome/browser/ui/tabs/tab_group_model.h"
#include "chrome/browser/ui/tabs/tab_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/tab_groups/tab_group_color.h"
#include "components/tab_groups/tab_group_id.h"
#include "components/tab_groups/tab_group_visual_data.h"
#include "components/tabs/public/tab_group.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/base_window.h"

static_assert(BUILDFLAG(ENABLE_AI_CHAT_TAB_MANAGEMENT_TOOL));
static_assert(!BUILDFLAG(IS_ANDROID));

namespace ai_chat {

namespace {

// Returns a sorted, de-duplicated list of indices that are valid for the
// provided TabStripModel. Any indices outside of the current tab bounds are
// dropped.
std::vector<int> MakeSortedUniqueValidIndices(const std::vector<int>& indices,
                                              const TabStripModel* tab_strip) {
  CHECK(tab_strip);
  const int tab_count = tab_strip->count();

  // The incoming values are positions within |tab_strip|, so they are bounded
  // by [0, tab_count). Marking the valid ones in a bitmap keyed on the index
  // and then reading them back in order filters, de-duplicates, and sorts in a
  // single linear pass, with no comparison sort.
  std::vector<bool> is_valid_index(tab_count, false);
  for (int index : indices) {
    if (index >= 0 && index < tab_count) {
      is_valid_index[index] = true;
    }
  }

  std::vector<int> result;
  for (int i = 0; i < tab_count; ++i) {
    if (is_valid_index[i]) {
      result.push_back(i);
    }
  }
  return result;
}

std::optional<tab_groups::TabGroupColorId> GetTabGroupColorId(
    std::string_view group_color) {
  if (group_color == "grey") {
    return tab_groups::TabGroupColorId::kGrey;
  } else if (group_color == "blue") {
    return tab_groups::TabGroupColorId::kBlue;
  } else if (group_color == "red") {
    return tab_groups::TabGroupColorId::kRed;
  } else if (group_color == "yellow") {
    return tab_groups::TabGroupColorId::kYellow;
  } else if (group_color == "green") {
    return tab_groups::TabGroupColorId::kGreen;
  } else if (group_color == "pink") {
    return tab_groups::TabGroupColorId::kPink;
  } else if (group_color == "purple") {
    return tab_groups::TabGroupColorId::kPurple;
  } else if (group_color == "cyan") {
    return tab_groups::TabGroupColorId::kCyan;
  } else if (group_color == "orange") {
    return tab_groups::TabGroupColorId::kOrange;
  }
  return std::nullopt;
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
    case tab_groups::TabGroupColorId::kNumEntries:
      NOTREACHED();
  }
}

// Returns a copy of |visual_data| with the optional |title| and |color|
// overrides applied. A null override leaves that field unchanged, and an
// unrecognized color string is ignored.
tab_groups::TabGroupVisualData ApplyGroupVisualOverrides(
    tab_groups::TabGroupVisualData visual_data,
    const std::string* title,
    const std::string* color) {
  if (title) {
    visual_data.SetTitle(base::UTF8ToUTF16(*title));
  }
  if (color) {
    if (std::optional<tab_groups::TabGroupColorId> color_id =
            GetTabGroupColorId(*color)) {
      visual_data = tab_groups::TabGroupVisualData(
          visual_data.title(), *color_id, visual_data.is_collapsed());
    }
  }
  return visual_data;
}

// A tab resolved from a tab-handle ID, together with its current location.
struct ResolvedTab {
  raw_ptr<tabs::TabInterface> tab;
  raw_ptr<content::WebContents> contents;
  raw_ptr<TabStripModel> tab_strip;
  int index;
};

// Returns the integer values of a list, skipping any non-integer entries.
std::vector<int> HandleIdsFromList(const base::ListValue& list) {
  std::vector<int> ids;
  ids.reserve(list.size());
  for (const auto& value : list) {
    if (value.is_int()) {
      ids.push_back(value.GetInt());
    }
  }
  return ids;
}

// Resolves tab-handle IDs to their live tabs, in input order. A tab is skipped
// when its handle is stale, it belongs to a different |profile|, it is not an
// entry in a tab strip, or (when |require_grouped|) it is not in a tab group.
std::vector<ResolvedTab> ResolveTabs(const std::vector<int>& tab_ids,
                                     Profile* profile,
                                     bool require_grouped) {
  std::vector<ResolvedTab> resolved;
  resolved.reserve(tab_ids.size());
  for (int handle_id : tab_ids) {
    auto* tab = tabs::TabHandle(handle_id).Get();
    if (!tab) {
      // A handle that resolves to no live tab usually means the model is
      // operating on a stale tab list. Surface it for diagnostics; we still
      // proceed with whatever tabs did resolve rather than failing the whole
      // call, since tabs can legitimately close between list and action.
      DVLOG(2) << "TabManagementTool: tab id " << handle_id
               << " did not resolve to a live tab (stale or invalid handle)";
      continue;
    }
    if (require_grouped && !tab->GetGroup().has_value()) {
      continue;
    }
    content::WebContents* contents = tab->GetContents();
    // Must belong to the same profile as the tool.
    if (!contents || contents->GetBrowserContext() != profile) {
      continue;
    }
    auto* browser_window = tab->GetBrowserWindowInterface();
    if (!browser_window) {
      continue;
    }
    TabStripModel* tab_strip = browser_window->GetTabStripModel();
    int index = tab_strip->GetIndexOfWebContents(contents);
    if (index == TabStripModel::kNoTab) {
      continue;
    }
    resolved.push_back({tab, contents, tab_strip, index});
  }
  return resolved;
}

// Buckets the tabs identified by |tab_ids| into a map of source TabStripModel
// to the indices of the matching tabs within it, applying the |profile| and
// |require_grouped| filters of ResolveTabs(). If |first_strip_out| is non-null,
// it receives the TabStripModel of the first valid tab in input order, which
// callers use to choose a destination window.
base::flat_map<TabStripModel*, std::vector<int>> ResolveTabIdsToStripIndices(
    const base::ListValue& tab_ids,
    Profile* profile,
    bool require_grouped,
    TabStripModel** first_strip_out) {
  base::flat_map<TabStripModel*, std::vector<int>> by_strip;
  for (const ResolvedTab& resolved :
       ResolveTabs(HandleIdsFromList(tab_ids), profile, require_grouped)) {
    if (first_strip_out && !*first_strip_out) {
      *first_strip_out = resolved.tab_strip;
    }
    by_strip[resolved.tab_strip].push_back(resolved.index);
  }
  return by_strip;
}

// Returns the handle of the tab among |tabs_to_move| that is currently the
// active tab of an active window, if any. Moving such a tab can change which
// window and tab are active, so the caller restores activation afterward.
std::optional<tabs::TabHandle> FindActiveTabToRestore(
    const std::vector<ResolvedTab>& tabs_to_move) {
  for (const ResolvedTab& moved : tabs_to_move) {
    auto* source_window = moved.tab->GetBrowserWindowInterface();
    if (source_window && source_window->GetWindow()->IsActive() &&
        moved.tab_strip->GetActiveTab() == moved.tab) {
      return moved.tab->GetHandle();
    }
  }
  return std::nullopt;
}

// Moves |tabs_to_move| into |target_tab_strip| starting at |index|, in request
// order so they land contiguously in that order. The insertion point is clamped
// to a valid position; when |target_group| is non-null it is clamped within the
// group's range and the moved tabs are added to that group. Returns the number
// of tabs actually moved.
int MoveResolvedTabsToStrip(const std::vector<ResolvedTab>& tabs_to_move,
                            TabStripModel* target_tab_strip,
                            TabGroup* target_group,
                            std::optional<int> index,
                            bool add_to_end) {
  const bool has_group = target_group != nullptr;

  // Without a group the insertion point can be anywhere in the strip; with a
  // group it is constrained to the group's current range.
  int min_target_index = 0;
  int max_target_index = target_tab_strip->count();
  if (has_group) {
    gfx::Range group_indexes = target_group->ListTabs();
    min_target_index = group_indexes.start();
    max_target_index = group_indexes.end();
  }
  int current_target_index =
      std::clamp(index.value_or(target_tab_strip->count()), min_target_index,
                 max_target_index);

  const std::optional<tab_groups::TabGroupId> group_id =
      has_group ? std::make_optional(target_group->id()) : std::nullopt;

  std::vector<int> moved_indices;
  for (const ResolvedTab& moved : tabs_to_move) {
    int source_index = moved.tab_strip->GetIndexOfWebContents(moved.contents);
    if (source_index == TabStripModel::kNoTab) {
      continue;
    }

    int landed_index = TabStripModel::kNoTab;
    if (moved.tab_strip != target_tab_strip) {
      // Cross-window move: detach from the source strip, insert into target.
      std::unique_ptr<tabs::TabModel> detached_tab =
          moved.tab_strip->DetachTabAtForInsertion(source_index);
      if (!detached_tab) {
        continue;
      }
      landed_index = target_tab_strip->InsertDetachedTabAt(
          current_target_index, std::move(detached_tab), ADD_NONE, group_id);
    } else {
      // Same-strip move.
      landed_index = target_tab_strip->MoveWebContentsAt(
          source_index, current_target_index, false, group_id);
    }
    moved_indices.push_back(landed_index);

    // Advance the insertion point so the next tab lands immediately after this
    // one, preserving the request order of the moved tabs. All moves run
    // synchronously on the tab strip here, so reading back the landed index is
    // safe with no timing concern.
    current_target_index =
        std::min(landed_index + 1, target_tab_strip->count());
  }

  // Ensure the moved tabs are members of the destination group. Cross-window
  // inserts above already join the group, but same-strip moves and |add_to_end|
  // ordering still need this pass.
  if (has_group && !moved_indices.empty()) {
    std::vector<int> valid_indices =
        MakeSortedUniqueValidIndices(moved_indices, target_tab_strip);
    if (!valid_indices.empty()) {
      target_tab_strip->AddToExistingGroup(valid_indices, target_group->id(),
                                           add_to_end);
    }
  }

  return static_cast<int>(moved_indices.size());
}

}  // namespace

TabManagementTool::TabManagementTool(Profile* profile) : profile_(profile) {}

TabManagementTool::~TabManagementTool() = default;

std::string_view TabManagementTool::Name() const {
  return mojom::kTabManagementToolName;
}

std::string_view TabManagementTool::Description() const {
  return "Manage browser tabs - list and move tabs, move groups, close tabs "
         "and manage tab groups. Only use this tool when the user explicitly "
         "requests tab "
         "organization, grouping, moving, or closing tabs. Do not use this "
         "tool proactively or when the user's request can be answered without "
         "modifying their tabs. "
         "This tool can list all open tabs with their window, group, URL and "
         "title information, "
         "move individual tabs (move_tabs) or an entire group (move_group) "
         "between windows or positions, "
         "close tabs, "
         "and create or modify tab groups. Groups are per-window, so make any "
         "moves before grouping. Use window_id=-1 to move tabs/groups to a new "
         "window. "
         "Use move_tabs to move individual tabs and move_group to move an "
         "entire group at once. "
         "When moving tabs, the active tab state is preserved - if you move "
         "the active tab, "
         "it remains active in its new location. "
         "After each operation, the updated tab list is returned. "
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

std::optional<base::DictValue> TabManagementTool::InputProperties() const {
  return CreateInputProperties(
      {{"action", StringProperty("The action to perform",
                                 std::vector<std::string>{
                                     "list", "move_tabs", "move_group", "close",
                                     "create_group", "update_group",
                                     "remove_from_group"})},
       {"plan",
        StringProperty("Human readable plan of what the assistant intends to "
                       "do with the list of tabs and with the tab management "
                       "tool. This should be provided during the very first "
                       "list operation in a conversation and allows the user "
                       "to approve or deny the tab management operations.")},
       {"tab_ids",
        ArrayProperty("List of tab IDs to operate on (for move_tabs, "
                      "close, create_group, remove_from_group).",
                      IntegerProperty("Tab ID"))},
       {"window_id",
        IntegerProperty(
            "Destination window ID (for move_tabs and move_group). Use -1 to "
            "create a new window. For move_tabs this is mutually exclusive "
            "with "
            "destination_group_id.")},
       {"group_id",
        StringProperty("The existing group to act on: the group to move "
                       "(move_group) or the group to update (update_group).")},
       {"destination_group_id",
        StringProperty(
            "Destination group to move tabs into (for move_tabs). Mutually "
            "exclusive with window_id; the target window is inferred from this "
            "group's window.")},
       {"index",
        IntegerProperty(
            "Destination index position (for move_tabs and move_group).")},
       {"group_title", StringProperty("Title for new or updated group")},
       {"group_color",
        StringProperty(
            "Color for new or updated group",
            std::vector<std::string>{"grey", "blue", "red", "yellow", "green",
                                     "pink", "purple", "cyan", "orange"})},
       {"add_to_end",
        BooleanProperty("Add tabs to the end of the destination group instead "
                        "of the beginning (for move_tabs into a group).")}});
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
  // TODO (https://github.com/brave/brave-browser/issues/56024): keep sidebar
  // open to a AI Chat conversation if it's open in the active window so that
  // user can follow along. Or open in a full page if we're going to remove the
  // active window with the active conversation.
  auto input = base::JSONReader::ReadDict(input_json,
                                          base::JSON_PARSE_CHROMIUM_EXTENSIONS);

  if (!input.has_value()) {
    DVLOG(4) << "Failed to parse TabManagementTool input JSON: " << input_json;
    std::move(callback).Run(
        CreateContentBlocksForText("Failed to parse input JSON. Please provide "
                                   "valid JSON with an 'action' field."),
        {});
    return;
  }

  const auto& dict = input.value();

  // Verify we have permission
  if (!user_has_granted_permission_) {
    // Report to LLM as to why we couldn't grant permission yet
    auto* plan = dict.FindString("plan");
    if (!plan || plan->empty()) {
      // No plan provided, so we can't grant permission
      std::move(callback).Run(
          CreateContentBlocksForText("No plan provided which the user will be "
                                     "asked to approve. Provide a "
                                     "plan for the first use of this tool."),
          {});
      return;
    }
    // We shouldn't get here since we expect callers to call
    // RequiresUserInteractionBeforeHandling first and only call UseTool if
    // permission was granted but, just in case, still provide output so the
    // conversation can proceed without running this tool.
    std::move(callback).Run(CreateContentBlocksForText("Unknown error"), {});
    return;
  }

  CHECK(user_has_granted_permission_);

  const auto* action = dict.FindString("action");

  if (!action) {
    std::move(callback).Run(
        CreateContentBlocksForText(
            "Missing required 'action' field. Must be one of: list, move_tabs, "
            "move_group, close, create_group, update_group, "
            "remove_from_group"),
        {});
    return;
  }

  if (*action == "list") {
    HandleListTabs(std::move(callback));
  } else if (*action == "move_tabs") {
    HandleMoveTabs(std::move(callback), dict);
  } else if (*action == "move_group") {
    HandleMoveGroup(std::move(callback), dict);
  } else if (*action == "create_group") {
    HandleCreateGroup(std::move(callback), dict);
  } else if (*action == "update_group") {
    HandleUpdateGroup(std::move(callback), dict);
  } else if (*action == "remove_from_group") {
    HandleRemoveFromGroup(std::move(callback), dict);
  } else {
    std::move(callback).Run(
        CreateContentBlocksForText(
            "Invalid action. Must be one of: list, move_tabs, move_group, "
            "close, create_group, update_group, remove_from_group"),
        {});
  }
}

base::DictValue TabManagementTool::GenerateTabList() const {
  base::DictValue result;
  base::ListValue windows;

  // Iterate through all browser windows for this profile
  for (BrowserWindowInterface* browser : GetAllBrowserWindowInterfaces()) {
    if (browser->GetProfile() != profile_) {
      continue;
    }

    base::DictValue window_info;
    window_info.Set("window_id", browser->GetSessionID().id());
    window_info.Set("is_active", browser->IsActive());

    TabStripModel* tab_strip = browser->GetTabStripModel();
    if (!tab_strip) {
      continue;
    }

    base::ListValue tabs;
    base::DictValue groups;

    // First, collect group information
    if (tab_strip->SupportsTabGroups() && tab_strip->group_model()) {
      for (const auto& group_id : tab_strip->group_model()->ListTabGroups()) {
        const TabGroup* group = tab_strip->group_model()->GetTabGroup(group_id);
        if (group) {
          base::DictValue group_info;
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

      base::DictValue tab_info;
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
                                              base::DictValue result) {
  base::DictValue tab_list = GenerateTabList();
  result.Set("windows", std::move(*tab_list.FindList("windows")));

  std::string json_output;
  base::JSONWriter::WriteWithOptions(
      result, base::JSONWriter::OPTIONS_PRETTY_PRINT, &json_output);
  std::move(callback).Run(CreateContentBlocksForText(json_output), {});
}

void TabManagementTool::PostTaskSendResultWithTabList(
    UseToolCallback callback,
    base::DictValue result,
    std::optional<tabs::TabHandle> active_moved_tab) {
  // If the operation determined we need to re-activate a tab and its window,
  // possibly due to moving the previously active tab to a new window,
  // then we can do it immediately as tab strip creation and movements
  // are immediately ready.
  if (active_moved_tab.has_value()) {
    if (tabs::TabInterface* tab = active_moved_tab->Get()) {
      if (BrowserWindowInterface* window = tab->GetBrowserWindowInterface()) {
        window->GetWindow()->Activate();
        window->GetTabStripModel()->ActivateTabAt(
            window->GetTabStripModel()->GetIndexOfTab(tab));
      }
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
  base::DictValue result = GenerateTabList();

  std::string json_output;
  base::JSONWriter::WriteWithOptions(
      result, base::JSONWriter::OPTIONS_PRETTY_PRINT, &json_output);

  std::move(callback).Run(CreateContentBlocksForText(json_output), {});
}

void TabManagementTool::HandleMoveTabs(UseToolCallback callback,
                                       const base::DictValue& params) {
  const auto* tab_ids = params.FindList("tab_ids");
  if (!tab_ids || tab_ids->empty()) {
    std::move(callback).Run(
        CreateContentBlocksForText(
            "Missing or empty 'tab_ids' array for move_tabs operation"),
        {});
    return;
  }

  const auto window_id = params.FindInt("window_id");
  const auto index = params.FindInt("index");
  const auto* destination_group_id = params.FindString("destination_group_id");
  const bool add_to_end = params.FindBool("add_to_end").value_or(false);

  const bool has_destination_group =
      destination_group_id && !destination_group_id->empty();
  if (has_destination_group && window_id.has_value()) {
    std::move(callback).Run(
        CreateContentBlocksForText(
            "Cannot provide both 'destination_group_id' and 'window_id' in the "
            "same request. 'destination_group_id' implies the target window "
            "(the group's window); to move tabs to a plain window use "
            "'window_id' instead."),
        {});
    return;
  }

  // Resolve the requested tabs in request order, restricted to this profile.
  std::vector<ResolvedTab> tabs_to_move = ResolveTabs(
      HandleIdsFromList(*tab_ids), profile_, /*require_grouped=*/false);
  if (tabs_to_move.empty()) {
    std::move(callback).Run(
        CreateContentBlocksForText("No valid tabs found to move"), {});
    return;
  }

  // Resolve the destination window, and group if any. In priority order the
  // destination is: an explicit window_id, the window owning
  // destination_group_id, or the first moved tab's current window. window_id
  // and destination_group_id are mutually exclusive (checked above).
  std::string error;
  bool did_create_window = false;
  BrowserWindowInterface* target_window = nullptr;
  TabGroup* target_group = nullptr;
  if (window_id.has_value()) {
    target_window =
        FindOrCreateTargetWindow(window_id, &error, &did_create_window);
    if (!error.empty()) {
      std::move(callback).Run(CreateContentBlocksForText(error), {});
      return;
    }
  } else if (has_destination_group) {
    target_window = FindWindowWithGroup(*destination_group_id, &target_group);
    if (!target_window) {
      std::move(callback).Run(
          CreateContentBlocksForText("Group not found with ID: " +
                                     *destination_group_id),
          {});
      return;
    }
  } else {
    target_window = tabs_to_move.front().tab->GetBrowserWindowInterface();
    if (!target_window) {
      std::move(callback).Run(
          CreateContentBlocksForText(
              "Could not determine target window for tab move"),
          {});
      return;
    }
  }

  if (!ValidateMoveTarget(target_window, index, &error)) {
    std::move(callback).Run(CreateContentBlocksForText(error), {});
    return;
  }

  // Moving the active tab out of its window can change activation, so remember
  // it and let PostTaskSendResultWithTabList restore it afterward.
  std::optional<tabs::TabHandle> active_moved_tab =
      FindActiveTabToRestore(tabs_to_move);

  const int moved_count =
      MoveResolvedTabsToStrip(tabs_to_move, target_window->GetTabStripModel(),
                              target_group, index, add_to_end);

  base::DictValue result;
  result.Set("message", "Successfully moved " +
                            base::NumberToString(moved_count) + " tab(s)");
  if (did_create_window) {
    result.Set("new_window_id", target_window->GetSessionID().id());
  }

  PostTaskSendResultWithTabList(std::move(callback), std::move(result),
                                active_moved_tab);
}

BrowserWindowInterface* TabManagementTool::FindOrCreateTargetWindow(
    std::optional<int> window_id,
    std::string* error_out,
    bool* did_create_window_out) {
  *did_create_window_out = false;

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
  *group_out = nullptr;
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
  // All paths that produce a target window (FindOrCreateTargetWindow,
  // FindWindowWithGroup, and a moved tab's own window) restrict to this
  // profile, so a cross-profile target is a programming error.
  CHECK_EQ(target_window->GetProfile(), profile_.get());

  TabStripModel* tab_strip = target_window->GetTabStripModel();
  if (!tab_strip) {
    *error = "Target window has no tab strip";
    return false;
  }

  if (target_window->GetType() != BrowserWindowInterface::Type::TYPE_NORMAL) {
    *error = "Target window is not a normal window";
    return false;
  }

  // Move targets may involve placing tabs into a group, so the destination
  // must support tab groups. All normal desktop browser windows do.
  if (!tab_strip->SupportsTabGroups()) {
    *error = "Target window does not support tab groups";
    return false;
  }

  // An out-of-range index is intentionally not an error: it is clamped to the
  // valid range when the move is performed, and an absent index appends to the
  // end of the tab strip.
  return true;
}

void TabManagementTool::HandleMoveGroup(UseToolCallback callback,
                                        const base::DictValue& params) {
  const auto* group_id = params.FindString("group_id");
  if (!group_id || group_id->empty()) {
    std::move(callback).Run(CreateContentBlocksForText(
                                "Missing 'group_id' for move_group operation"),
                            {});
    return;
  }

  const auto window_id = params.FindInt("window_id");
  const auto index = params.FindInt("index");

  TabGroup* group = nullptr;
  BrowserWindowInterface* source_window =
      FindWindowWithGroup(*group_id, &group);

  if (!source_window || !group) {
    std::move(callback).Run(
        CreateContentBlocksForText("Group not found with ID: " + *group_id),
        {});
    return;
  }

  TabStripModel* source_tab_strip = source_window->GetTabStripModel();

  std::string error;
  bool did_create_window = false;
  BrowserWindowInterface* target_window =
      FindOrCreateTargetWindow(window_id, &error, &did_create_window);
  if (!error.empty()) {
    std::move(callback).Run(CreateContentBlocksForText(error), {});
    return;
  }

  TabStripModel* target_tab_strip = nullptr;
  if (target_window) {
    target_tab_strip = target_window->GetTabStripModel();
    if (!ValidateMoveTarget(target_window, index, &error)) {
      std::move(callback).Run(CreateContentBlocksForText(error), {});
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
          CreateContentBlocksForText("Group already at target position"), {});
      return;
    }

    source_tab_strip->MoveGroupTo(group->id(), target_index);

    base::DictValue result;
    result.Set("message", "Successfully moved group within same window");
    PostTaskSendResultWithTabList(std::move(callback), std::move(result));
  } else {
    // Cross-window move - use detach/attach
    int target_index = index.value_or(target_tab_strip->count());
    target_index = std::min(target_index, target_tab_strip->count());

    // See if we need to re-activate the target window
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

    base::DictValue result;
    result.Set("message", "Successfully moved group to different window");
    if (did_create_window) {
      result.Set("new_window_id", target_window->GetSessionID().id());
    }
    PostTaskSendResultWithTabList(std::move(callback), std::move(result),
                                  tab_to_reactivate);
  }
}

void TabManagementTool::HandleCreateGroup(UseToolCallback callback,
                                          const base::DictValue& params) {
  const auto* tab_ids = params.FindList("tab_ids");
  const auto* group_title = params.FindString("group_title");
  const auto* group_color = params.FindString("group_color");

  if (!tab_ids || tab_ids->empty()) {
    std::move(callback).Run(
        CreateContentBlocksForText(
            "Missing or empty 'tab_ids' array for create_group operation"),
        {});
    return;
  }

  // Find tabs and bucket them by browser using TabHandles. The window of the
  // first valid tab in the request determines the target window for the new
  // group.
  TabStripModel* target_tab_strip = nullptr;
  base::flat_map<TabStripModel*, std::vector<int>> browser_tabs =
      ResolveTabIdsToStripIndices(*tab_ids, profile_,
                                  /*require_grouped=*/false, &target_tab_strip);

  if (!target_tab_strip || browser_tabs.empty()) {
    std::move(callback).Run(
        CreateContentBlocksForText("No valid tabs found to group"), {});
    return;
  }

  // The target window's tab strip must support tab groups: AddToNewGroup()
  // CHECKs SupportsTabGroups().
  if (!target_tab_strip->SupportsTabGroups()) {
    std::move(callback).Run(
        CreateContentBlocksForText("Target window does not support tab groups"),
        {});
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
      target_tab_strip->ChangeTabGroupVisuals(
          new_group_id, ApplyGroupVisualOverrides(*group->visual_data(),
                                                  group_title, group_color));
    }
  }

  std::vector<std::unique_ptr<tabs::TabModel>> tabs_moved_models;

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
        tabs_moved_models.push_back(tab_strip->DetachTabAtForInsertion(index));
      }
    }
  }

  // Insert tabs from other windows into the target window
  for (auto& tab_model : tabs_moved_models) {
    target_tab_strip->InsertDetachedTabAt(first_valid_indices.front(),
                                          std::move(tab_model), ADD_NONE,
                                          new_group_id);
  }

  base::DictValue result;
  result.Set("message", "Successfully created 1 group.");
  result.Set("created_group_id", new_group_id.ToString());

  PostTaskSendResultWithTabList(std::move(callback), std::move(result));
}

void TabManagementTool::HandleUpdateGroup(UseToolCallback callback,
                                          const base::DictValue& params) {
  const auto* group_id_str = params.FindString("group_id");
  const auto* group_title = params.FindString("group_title");
  const auto* group_color = params.FindString("group_color");

  if (!group_id_str || group_id_str->empty()) {
    std::move(callback).Run(
        CreateContentBlocksForText(
            "Missing 'group_id' for update_group operation"),
        {});
    return;
  }

  bool found = false;

  // Find and update the group
  TabGroup* group = nullptr;
  if (BrowserWindowInterface* window =
          FindWindowWithGroup(*group_id_str, &group)) {
    tab_groups::TabGroupVisualData visual_data = ApplyGroupVisualOverrides(
        *group->visual_data(), group_title, group_color);
    window->GetTabStripModel()->ChangeTabGroupVisuals(group->id(), visual_data);
    found = true;
  }

  if (found) {
    base::DictValue result;
    result.Set("message", "Successfully updated group");
    PostTaskSendResultWithTabList(std::move(callback), std::move(result));
  } else {
    std::move(callback).Run(
        CreateContentBlocksForText("Group not found with ID: " + *group_id_str),
        {});
  }
}

void TabManagementTool::HandleRemoveFromGroup(UseToolCallback callback,
                                              const base::DictValue& params) {
  const auto* tab_ids = params.FindList("tab_ids");

  if (!tab_ids || tab_ids->empty()) {
    std::move(callback).Run(
        CreateContentBlocksForText(
            "Missing or empty 'tab_ids' array for remove_from_group operation"),
        {});
    return;
  }

  // Find the grouped tabs and bucket them by browser using TabHandles.
  base::flat_map<TabStripModel*, std::vector<int>> browser_tabs =
      ResolveTabIdsToStripIndices(*tab_ids, profile_, /*require_grouped=*/true,
                                  /*first_strip_out=*/nullptr);

  if (browser_tabs.empty()) {
    std::move(callback).Run(
        CreateContentBlocksForText("No valid tabs found to remove from groups"),
        {});
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

  base::DictValue result;
  result.Set("message", "Successfully removed " +
                            base::NumberToString(removed_count) +
                            " tab(s) from their groups");
  PostTaskSendResultWithTabList(std::move(callback), std::move(result));
}

}  // namespace ai_chat
