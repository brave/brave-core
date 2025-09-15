/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <map>

#include "base/barrier_callback.h"
#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/strcat.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/bind_post_task.h"
#include "base/task/thread_pool.h"
#include "brave/browser/tabs/tab_content_extractor.h"
#include "brave/components/local_ai/browser/local_models_updater.h"
#include "brave/components/local_ai/browser/text_embedder.h"
#include "brave/components/local_ai/common/features.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/views/tabs/dragging/tab_drag_controller.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/web_contents.h"
#include "url/gurl.h"

#define DraggingTabsSession DraggingTabsSessionChromium
#define ExecuteContextMenuCommand ExecuteContextMenuCommand_ChromiumImpl
#define IsContextMenuCommandEnabled IsContextMenuCommandEnabled_ChromiumImpl

// Suppress switch completeness warnings since our overrides handle the new enum
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-enum"
#pragma clang diagnostic ignored "-Wswitch"

#include <chrome/browser/ui/tabs/tab_strip_model.cc>  // IWYU pragma: export

#pragma clang diagnostic pop
#undef IsContextMenuCommandEnabled
#undef ExecuteContextMenuCommand
#undef DraggingTabsSession

void TabStripModel::CloseSelectedTabsWithSplitView() {
  NOTREACHED();
}

void TabStripModel::ExecuteContextMenuCommand(int context_index,
                                              ContextMenuCommand command_id) {
  if (command_id == CommandAddTabToSuggestedGroup) {
    HandleAddTabToSuggestedGroupCommand(context_index);
    return;
  }
  ExecuteContextMenuCommand_ChromiumImpl(context_index, command_id);
}

bool TabStripModel::IsContextMenuCommandEnabled(
    int context_index,
    ContextMenuCommand command_id) const {
  if (command_id == CommandAddTabToSuggestedGroup) {
    // Check if Local AI Tab Grouping feature is enabled
    if (!base::FeatureList::IsEnabled(
            local_ai::features::kLocalAITabGrouping)) {
      return false;
    }

    // Check if the text embedder model is available
    auto* state = local_ai::LocalModelsUpdaterState::GetInstance();
    if (state->GetInstallDir().empty()) {
      return false;
    }

    // Check if there are any ungrouped tabs in selection
    std::vector<int> indices = GetIndicesForCommand(context_index);
    for (int index : indices) {
      if (!GetTabGroupForTab(index).has_value()) {
        // Check if there are existing groups to suggest
        for (int i = 0; i < count(); ++i) {
          if (GetTabGroupForTab(i).has_value()) {
            return true;  // Found ungrouped tab and existing groups
          }
        }
      }
    }
    return false;
  }
  return IsContextMenuCommandEnabled_ChromiumImpl(context_index, command_id);
}

void TabStripModel::HandleAddTabToSuggestedGroupCommand(int context_index) {
  if (!base::FeatureList::IsEnabled(local_ai::features::kLocalAITabGrouping)) {
    return;
  }

  auto* state = local_ai::LocalModelsUpdaterState::GetInstance();
  if (state->GetInstallDir().empty()) {
    return;
  }

  std::vector<int> indices = GetIndicesForCommand(context_index);

  // Filter out tabs that are already in groups and convert to global handles
  std::vector<int> ungrouped_global_handles;
  for (int index : indices) {
    if (!GetTabGroupForTab(index).has_value()) {
      const tabs::TabInterface* tab = GetTabAtIndex(index);
      if (tab) {
        ungrouped_global_handles.push_back(tab->GetHandle().raw_value());
      }
    }
  }

  if (ungrouped_global_handles.empty()) {
    return;
  }

  // Create TextEmbedder
  auto embedder_task_runner = base::ThreadPool::CreateSequencedTaskRunner(
      {base::MayBlock(), base::TaskPriority::USER_BLOCKING});
  auto text_embedder = local_ai::TextEmbedder::Create(
      state->GetUniversalQAModel(), embedder_task_runner);

  if (!text_embedder) {
    return;
  }

  // Get the Browser and Profile to search across all windows
  content::WebContents* active_web_contents = GetActiveWebContents();
  if (!active_web_contents) {
    return;
  }

  Browser* browser = chrome::FindBrowserWithTab(active_web_contents);
  if (!browser) {
    return;
  }

  Profile* profile = browser->profile();
  if (!profile) {
    return;
  }

  // Collect all tabs that need content extraction from all windows in the
  // profile
  std::vector<BraveTabStripTabData> all_tabs_needing_content;
  CollectTabsFromAllWindows(profile, all_tabs_needing_content,
                            ungrouped_global_handles);

  if (all_tabs_needing_content.empty()) {
    return;
  }

  // Initialize TextEmbedder first
  text_embedder->Initialize(
      base::BindOnce(&TabStripModel::OnTextEmbedderInitializedForGroupCommand,
                     weak_factory_.GetWeakPtr(), std::move(text_embedder),
                     std::move(all_tabs_needing_content),
                     ungrouped_global_handles, context_index));
}

void TabStripModel::OnTextEmbedderInitializedForGroupCommand(
    std::unique_ptr<local_ai::TextEmbedder, base::OnTaskRunnerDeleter>
        text_embedder,
    std::vector<BraveTabStripTabData> all_tabs_needing_content,
    std::vector<int> ungrouped_global_handles,
    int context_index,
    bool success) {
  if (!success) {
    return;
  }

  // Now collect inner text for all tabs using barrier callback

  auto barrier_callback = base::BarrierCallback<
      std::pair<int, tab_content_extractor::ExtractedData>>(
      all_tabs_needing_content.size(),
      base::BindOnce(&TabStripModel::OnAllTabContentCollectedForGroupCommand,
                     weak_factory_.GetWeakPtr(), std::move(text_embedder),
                     all_tabs_needing_content, ungrouped_global_handles,
                     context_index));

  for (const auto& tab_data : all_tabs_needing_content) {
    VLOG(1) << "Starting content extraction for tab " << tab_data.index;
    tab_content_extractor::ExtractTextContent(
        tab_data.web_contents, tab_data.url, tab_data.index,
        base::BindOnce(
            [](base::RepeatingCallback<void(
                   std::pair<int, tab_content_extractor::ExtractedData>)>
                   callback,
               std::pair<int, tab_content_extractor::ExtractedData> result) {
              callback.Run(result);
            },
            barrier_callback));
  }
}

void TabStripModel::OnAllTabContentCollectedForGroupCommand(
    std::unique_ptr<local_ai::TextEmbedder, base::OnTaskRunnerDeleter>
        text_embedder,
    std::vector<BraveTabStripTabData> all_tabs_needing_content,
    std::vector<int> ungrouped_global_handles,
    int context_index,
    std::vector<std::pair<int, tab_content_extractor::ExtractedData>>
        content_results) {
  // Create a map of tab_index -> extracted data for quick lookup
  std::map<int, tab_content_extractor::ExtractedData> content_map;
  for (const auto& result : content_results) {
    content_map[result.first] = result.second;
  }

  // Build the group_tabs map with content
  std::map<tab_groups::TabGroupId, std::vector<local_ai::TextEmbedder::TabInfo>>
      group_tabs;

  for (const auto& tab_data : all_tabs_needing_content) {
    // Skip ungrouped tabs (they are candidates, not group tabs)
    if (!tab_data.group_id.has_value()) {
      continue;
    }

    const auto& extracted_data = content_map[tab_data.index];

    // Initialize the group if we haven't seen it yet
    if (group_tabs.find(tab_data.group_id.value()) == group_tabs.end()) {
      group_tabs[tab_data.group_id.value()] =
          std::vector<local_ai::TextEmbedder::TabInfo>();
    }

    group_tabs[tab_data.group_id.value()].push_back(
        {tab_data.title, tab_data.url, extracted_data.content,
         extracted_data.description});
  }

  if (group_tabs.empty()) {
    return;
  }

  // Process each ungrouped tab individually
  struct TabSuggestionResult {
    int tab_global_handle;
    absl::StatusOr<tab_groups::TabGroupId> suggested_group;
  };

  auto barrier_callback = base::BarrierCallback<TabSuggestionResult>(
      ungrouped_global_handles.size(),
      base::BindOnce(
          [](base::WeakPtr<TabStripModel> model, int context_index,
             std::vector<TabSuggestionResult> results) {
            if (!model) {
              return;
            }

            // Group tabs by their suggested group and add them
            std::map<tab_groups::TabGroupId, std::vector<int>> tabs_by_group;
            for (const auto& result : results) {
              if (result.suggested_group.ok()) {
                tab_groups::TabGroupId group_id =
                    result.suggested_group.value();
                tabs_by_group[group_id].push_back(result.tab_global_handle);
              } else {
              }
            }

            // Add tabs to their suggested groups
            for (const auto& [group_id, tab_global_handles] : tabs_by_group) {
              model->OnSuggestGroupForTabResult(
                  tab_global_handles, context_index,
                  absl::StatusOr<tab_groups::TabGroupId>(group_id));
            }
          },
          weak_factory_.GetWeakPtr(), context_index));

  // Process each ungrouped tab
  for (int tab_global_handle : ungrouped_global_handles) {
    auto it = std::find_if(
        all_tabs_needing_content.begin(), all_tabs_needing_content.end(),
        [tab_global_handle](const BraveTabStripTabData& data) {
          return data.index == tab_global_handle;
        });

    if (it == all_tabs_needing_content.end()) {
      barrier_callback.Run({tab_global_handle, absl::FailedPreconditionError(
                                                   "Tab data not found")});
      continue;
    }

    const auto& extracted_data = content_map[tab_global_handle];

    local_ai::TextEmbedder::CandidateTab candidate_tab;
    candidate_tab.index = tab_global_handle;
    candidate_tab.tab_info = {it->title, it->url, extracted_data.content,
                              extracted_data.description};

    text_embedder->SuggestGroupForTab(
        candidate_tab, group_tabs,
        base::BindOnce(
            [](base::RepeatingCallback<void(TabSuggestionResult)> callback,
               int tab_global_handle,
               absl::StatusOr<tab_groups::TabGroupId> result) {
              callback.Run({tab_global_handle, result});
            },
            barrier_callback, tab_global_handle));
  }
}

void TabStripModel::OnSuggestGroupForTabResult(
    std::vector<int> tab_global_handles,
    int context_index,
    absl::StatusOr<tab_groups::TabGroupId> result) {
  if (!result.ok()) {
    return;  // Error occurred
  }

  tab_groups::TabGroupId target_group = result.value();

  // First, find the TabStripModel that contains the target group
  TabStripModel* target_strip_model = nullptr;
  Browser* target_browser = nullptr;

  // Get the Profile to search across all windows
  content::WebContents* active_web_contents = GetActiveWebContents();
  if (!active_web_contents) {
    return;
  }
  Browser* current_browser = chrome::FindBrowserWithTab(active_web_contents);
  if (!current_browser) {
    return;
  }
  Profile* profile = current_browser->profile();
  if (!profile) {
    return;
  }

  // Search all browsers in the same profile to find the target group
  for (Browser* browser : *BrowserList::GetInstance()) {
    if (!ShouldTrackBrowser(browser, profile)) {
      continue;
    }
    TabStripModel* strip_model = browser->tab_strip_model();
    if (!strip_model) {
      continue;
    }

    // Check if this TabStripModel contains the target group
    for (int i = 0; i < strip_model->count(); ++i) {
      std::optional<tab_groups::TabGroupId> group =
          strip_model->GetTabGroupForTab(i);
      if (group.has_value() && group.value() == target_group) {
        target_strip_model = strip_model;
        target_browser = browser;
        break;
      }
    }
    if (target_strip_model) {
      break;  // Found the target group
    }
  }

  if (!target_strip_model || !target_browser) {
    return;  // Target group not found
  }

  // Now collect and move tabs to the target browser window
  std::vector<content::WebContents*> tabs_to_move;
  std::vector<int> same_window_indices;  // Tabs already in target window

  for (int global_handle : tab_global_handles) {
    const tabs::TabHandle handle = tabs::TabHandle(global_handle);
    tabs::TabInterface* const tab = handle.Get();
    if (!tab) {
      continue;  // Tab no longer exists
    }

    BrowserWindowInterface* browser_window = tab->GetBrowserWindowInterface();
    if (!browser_window) {
      continue;
    }

    TabStripModel* source_strip_model = browser_window->GetTabStripModel();
    if (!source_strip_model) {
      continue;
    }

    int local_index = source_strip_model->GetIndexOfTab(tab);
    if (local_index == TabStripModel::kNoTab) {
      continue;  // Tab not found in strip model
    }

    // Verify the tab is still ungrouped
    if (source_strip_model->GetTabGroupForTab(local_index).has_value()) {
      continue;  // Skip tabs that are already grouped
    }

    content::WebContents* web_contents = tab->GetContents();
    if (!web_contents) {
      continue;
    }

    if (source_strip_model == target_strip_model) {
      // Tab is already in the target window
      same_window_indices.push_back(local_index);
    } else {
      // Tab needs to be moved to target window
      tabs_to_move.push_back(web_contents);
    }
  }

  // Move cross-window tabs to the target browser
  std::vector<int> moved_tab_indices;
  for (content::WebContents* web_contents : tabs_to_move) {
    // Find the source TabStripModel for this WebContents
    TabStripModel* source_strip_model = nullptr;
    int source_index = -1;

    for (Browser* browser : *BrowserList::GetInstance()) {
      if (!ShouldTrackBrowser(browser, profile)) {
        continue;
      }
      TabStripModel* strip_model = browser->tab_strip_model();
      if (!strip_model) {
        continue;
      }
      int index = strip_model->GetIndexOfWebContents(web_contents);
      if (index != TabStripModel::kNoTab) {
        source_strip_model = strip_model;
        source_index = index;
        break;
      }
    }

    if (!source_strip_model || source_index == -1) {
      continue;  // Source not found
    }

    // Detach from source and attach to target
    std::unique_ptr<content::WebContents> detached_contents =
        source_strip_model->DetachWebContentsAtForInsertion(source_index);

    int insertion_index = target_strip_model->count();
    int new_index = target_strip_model->InsertWebContentsAt(
        insertion_index, std::move(detached_contents),
        AddTabTypes::ADD_ACTIVE | AddTabTypes::ADD_INHERIT_OPENER);

    if (new_index != TabStripModel::kNoTab) {
      moved_tab_indices.push_back(new_index);
    }
  }

  // Combine all tab indices (same-window + moved tabs)
  std::vector<int> all_target_indices = same_window_indices;
  all_target_indices.insert(all_target_indices.end(),
                           moved_tab_indices.begin(), moved_tab_indices.end());

  // Add all tabs to the target group
  if (!all_target_indices.empty()) {
    std::ranges::sort(all_target_indices);
    target_strip_model->AddToExistingGroup(all_target_indices, target_group);
  }
}

bool TabStripModel::ShouldTrackBrowser(Browser* browser,
                                       Profile* target_profile) {
  return browser && browser->profile() == target_profile &&
         browser->type() == Browser::TYPE_NORMAL;
}

void TabStripModel::CollectTabsFromAllWindows(
    Profile* profile,
    std::vector<BraveTabStripTabData>& all_tabs_needing_content,
    const std::vector<int>& ungrouped_indices) {
  // Collect tabs from all browsers in the same profile
  for (Browser* browser : *BrowserList::GetInstance()) {
    if (!ShouldTrackBrowser(browser, profile)) {
      continue;
    }

    TabStripModel* tab_strip_model = browser->tab_strip_model();
    if (!tab_strip_model) {
      continue;
    }

    // Collect group tabs from this browser
    for (int i = 0; i < tab_strip_model->count(); ++i) {
      auto group_id = tab_strip_model->GetTabGroupForTab(i);
      if (group_id.has_value()) {
        content::WebContents* web_contents =
            tab_strip_model->GetWebContentsAt(i);
        if (web_contents) {
          std::u16string title = web_contents->GetTitle();
          GURL url = web_contents->GetVisibleURL();
          // Use the tab's global TabHandle ID for uniqueness across windows
          const tabs::TabInterface* tab = tab_strip_model->GetTabAtIndex(i);
          int global_tab_index = tab ? tab->GetHandle().raw_value() : i;
          all_tabs_needing_content.push_back(
              {global_tab_index, title, url, group_id, web_contents});
        }
      }
    }
  }

  // Add ungrouped candidate tabs from the current window only
  // (since these are the ones selected by the user for grouping)
  // Note: ungrouped_indices now contains global handles, not local indices
  for (int global_handle : ungrouped_indices) {
    // Find the tab by global handle in the current window
    content::WebContents* web_contents = nullptr;
    for (int i = 0; i < count(); ++i) {
      const tabs::TabInterface* tab = GetTabAtIndex(i);
      if (tab && tab->GetHandle().raw_value() == global_handle) {
        web_contents = GetWebContentsAt(i);
        break;
      }
    }

    if (web_contents) {
      std::u16string title = web_contents->GetTitle();
      GURL url = web_contents->GetVisibleURL();
      all_tabs_needing_content.push_back(
          {global_handle, title, url, std::nullopt, web_contents});
    }
  }
}
