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
#include "brave/browser/local_ai/fast_vlm_tab_content_extractor.h"
#include "brave/components/local_ai/browser/local_models_updater.h"
#include "brave/components/local_ai/browser/text_embedder.h"
#include "brave/components/local_ai/common/features.h"
#include "chrome/browser/ui/views/tabs/dragging/tab_drag_controller.h"
#include "components/content_extraction/content/browser/inner_text.h"
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

  // Filter out tabs that are already in groups
  std::vector<int> ungrouped_indices;
  for (int index : indices) {
    if (!GetTabGroupForTab(index).has_value()) {
      ungrouped_indices.push_back(index);
    }
  }

  if (ungrouped_indices.empty()) {
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

  // Collect all tabs that need content extraction (both group tabs and
  // candidate tabs)
  std::vector<BraveTabStripTabData> all_tabs_needing_content;

  // Collect all group tabs
  for (int i = 0; i < count(); ++i) {
    auto group_id = GetTabGroupForTab(i);
    if (group_id.has_value()) {
      content::WebContents* web_contents = GetWebContentsAt(i);
      if (web_contents) {
        std::u16string title = web_contents->GetTitle();
        GURL url = web_contents->GetVisibleURL();
        all_tabs_needing_content.push_back(
            {i, title, url, group_id, web_contents});
      }
    }
  }

  // Add ungrouped candidate tabs (use empty group_id for these)
  for (int index : ungrouped_indices) {
    content::WebContents* web_contents = GetWebContentsAt(index);
    if (web_contents) {
      std::u16string title = web_contents->GetTitle();
      GURL url = web_contents->GetVisibleURL();
      all_tabs_needing_content.push_back(
          {index, title, url, std::nullopt, web_contents});
    }
  }

  if (all_tabs_needing_content.empty()) {
    return;
  }

  // Initialize TextEmbedder first
  text_embedder->Initialize(base::BindOnce(
      &TabStripModel::OnTextEmbedderInitializedForGroupCommand,
      weak_factory_.GetWeakPtr(), std::move(text_embedder),
      std::move(all_tabs_needing_content), ungrouped_indices, context_index));
}

void TabStripModel::OnTextEmbedderInitializedForGroupCommand(
    std::unique_ptr<local_ai::TextEmbedder, base::OnTaskRunnerDeleter>
        text_embedder,
    std::vector<BraveTabStripTabData> all_tabs_needing_content,
    std::vector<int> ungrouped_indices,
    int context_index,
    bool success) {
  if (!success) {
    return;
  }

  // Now collect inner text for all tabs using barrier callback

  auto barrier_callback = base::BarrierCallback<std::pair<int, std::string>>(
      all_tabs_needing_content.size(),
      base::BindOnce(&TabStripModel::OnAllTabContentCollectedForGroupCommand,
                     weak_factory_.GetWeakPtr(), std::move(text_embedder),
                     all_tabs_needing_content, ungrouped_indices,
                     context_index));

  for (const auto& tab_data : all_tabs_needing_content) {
    local_ai::FastVLMTabContentExtractor::ExtractVisualContent(
        tab_data.web_contents, tab_data.index,
        base::BindOnce(
            [](base::RepeatingCallback<void(std::pair<int, std::string>)>
                   callback,
               int tab_index, const std::string& visual_description) {
              callback.Run(std::make_pair(tab_index, visual_description));
            },
            barrier_callback));
  }
}

void TabStripModel::OnAllTabContentCollectedForGroupCommand(
    std::unique_ptr<local_ai::TextEmbedder, base::OnTaskRunnerDeleter>
        text_embedder,
    std::vector<BraveTabStripTabData> all_tabs_needing_content,
    std::vector<int> ungrouped_indices,
    int context_index,
    std::vector<std::pair<int, std::string>> content_results) {
  // Create a map of tab_index -> content for quick lookup
  std::map<int, std::string> content_map;
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

    std::string content = content_map[tab_data.index];

    // Initialize the group if we haven't seen it yet
    if (group_tabs.find(tab_data.group_id.value()) == group_tabs.end()) {
      group_tabs[tab_data.group_id.value()] =
          std::vector<local_ai::TextEmbedder::TabInfo>();
    }

    group_tabs[tab_data.group_id.value()].push_back(
        {tab_data.title, tab_data.url, content});
  }

  if (group_tabs.empty()) {
    return;
  }

  // Process each ungrouped tab individually
  struct TabSuggestionResult {
    int tab_index;
    absl::StatusOr<tab_groups::TabGroupId> suggested_group;
  };

  auto barrier_callback = base::BarrierCallback<TabSuggestionResult>(
      ungrouped_indices.size(),
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
                tabs_by_group[group_id].push_back(result.tab_index);
              } else {
              }
            }

            // Add tabs to their suggested groups
            for (const auto& [group_id, tab_indices] : tabs_by_group) {
              model->OnSuggestGroupForTabResult(
                  tab_indices, context_index,
                  absl::StatusOr<tab_groups::TabGroupId>(group_id));
            }
          },
          weak_factory_.GetWeakPtr(), context_index));

  // Process each ungrouped tab
  for (int tab_index : ungrouped_indices) {
    auto it = std::find_if(all_tabs_needing_content.begin(),
                           all_tabs_needing_content.end(),
                           [tab_index](const BraveTabStripTabData& data) {
                             return data.index == tab_index;
                           });

    if (it == all_tabs_needing_content.end()) {
      barrier_callback.Run(
          {tab_index, absl::FailedPreconditionError("Tab data not found")});
      continue;
    }

    std::string content = content_map[tab_index];

    local_ai::TextEmbedder::CandidateTab candidate_tab;
    candidate_tab.index = tab_index;
    candidate_tab.tab_info = {it->title, it->url, content};

    text_embedder->SuggestGroupForTab(
        candidate_tab, group_tabs,
        base::BindOnce(
            [](base::RepeatingCallback<void(TabSuggestionResult)> callback,
               int tab_index, absl::StatusOr<tab_groups::TabGroupId> result) {
              callback.Run({tab_index, result});
            },
            barrier_callback, tab_index));
  }
}

void TabStripModel::OnSuggestGroupForTabResult(
    std::vector<int> tab_indices,
    int context_index,
    absl::StatusOr<tab_groups::TabGroupId> result) {
  if (!result.ok()) {
    return;  // Error occurred
  }

  tab_groups::TabGroupId target_group = result.value();

  // Filter and validate tab indices
  std::vector<int> valid_indices;
  for (int tab_index : tab_indices) {
    if (tab_index >= 0 && tab_index < count()) {
      if (!GetTabGroupForTab(tab_index).has_value()) {
        valid_indices.push_back(tab_index);
      }
    }
  }

  // Add tabs to the suggested group
  if (!valid_indices.empty()) {
    std::ranges::sort(valid_indices);
    AddToExistingGroup(valid_indices, target_group);
  }
}
