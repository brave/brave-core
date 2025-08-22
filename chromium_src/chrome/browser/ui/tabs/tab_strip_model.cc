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
#include "brave/components/local_ai/browser/local_models_updater.h"
#include "brave/components/local_ai/browser/text_embedder.h"
#include "brave/components/local_ai/common/features.h"
#include "chrome/browser/ui/views/tabs/dragging/tab_drag_controller.h"
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

  // Collect existing groups first (shared across all tab processing)
  std::vector<std::vector<std::string>> group_tabs;
  std::set<tab_groups::TabGroupId> processed_groups;

  for (int i = 0; i < count(); ++i) {
    auto group_id = GetTabGroupForTab(i);
    if (group_id.has_value() &&
        processed_groups.find(group_id.value()) == processed_groups.end()) {
      processed_groups.insert(group_id.value());

      std::vector<std::string> current_group_tabs;
      for (int j = 0; j < count(); ++j) {
        auto tab_group = GetTabGroupForTab(j);
        if (tab_group.has_value() && tab_group.value() == group_id.value()) {
          content::WebContents* group_web_contents = GetWebContentsAt(j);
          if (group_web_contents) {
            std::u16string group_title = group_web_contents->GetTitle();
            GURL group_url = group_web_contents->GetVisibleURL();
            std::string group_tab_description = base::StrCat(
                {base::UTF16ToUTF8(group_title), " | ", group_url.spec()});
            current_group_tabs.push_back(group_tab_description);
          }
        }
      }
      if (!current_group_tabs.empty()) {
        group_tabs.push_back(std::move(current_group_tabs));
      }
    }
  }

  if (group_tabs.empty()) {
    return;  // No existing groups
  }

  // Initialize the TextEmbedder first
  text_embedder->Initialize(base::BindOnce(
      [](base::WeakPtr<TabStripModel> model,
         std::unique_ptr<local_ai::TextEmbedder, base::OnTaskRunnerDeleter>
             embedder,
         std::vector<std::vector<std::string>> group_tabs,
         std::vector<int> ungrouped_indices, int context_index, bool success) {
        if (!model || !success) {
          return;
        }

        // Use barrier callback to wait for all tab suggestions to complete
        struct TabSuggestionResult {
          int tab_index;
          absl::StatusOr<int> suggested_group;
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
                  std::map<int, std::vector<int>> tabs_by_group;
                  for (const auto& result : results) {
                    if (result.suggested_group.ok()) {
                      int group_index = result.suggested_group.value();
                      tabs_by_group[group_index].push_back(result.tab_index);
                      VLOG(1) << "Tab " << result.tab_index << " -> Group "
                              << group_index;
                    } else {
                      VLOG(1)
                          << "No suggestion for tab " << result.tab_index
                          << ": " << result.suggested_group.status().message();
                    }
                  }

                  // Add tabs to their suggested groups
                  for (const auto& [group_index, tab_indices] : tabs_by_group) {
                    model->OnSuggestGroupForTabResult(
                        tab_indices, context_index,
                        absl::StatusOr<int>(group_index));
                  }
                },
                model, context_index));

        // Process each tab individually
        for (size_t i = 0; i < ungrouped_indices.size(); ++i) {
          int tab_index = ungrouped_indices[i];
          content::WebContents* web_contents =
              model->GetWebContentsAt(tab_index);
          if (!web_contents) {
            barrier_callback.Run(
                {tab_index, absl::FailedPreconditionError("No web contents")});
            continue;
          }

          std::u16string title = web_contents->GetTitle();
          GURL url = web_contents->GetVisibleURL();
          std::string tab_description =
              base::StrCat({base::UTF16ToUTF8(title), " | ", url.spec()});

          embedder->SuggestGroupForTab(
              std::make_pair(tab_index, tab_description), group_tabs,
              base::BindOnce(
                  [](base::RepeatingCallback<void(TabSuggestionResult)>
                         callback,
                     int tab_index, absl::StatusOr<int> result) {
                    callback.Run({tab_index, result});
                  },
                  barrier_callback, tab_index));
        }
      },
      weak_factory_.GetWeakPtr(), std::move(text_embedder),
      std::move(group_tabs), ungrouped_indices, context_index));
}

void TabStripModel::OnSuggestGroupForTabResult(std::vector<int> tab_indices,
                                               int context_index,
                                               absl::StatusOr<int> result) {
  if (!result.ok()) {
    return;  // Error occurred
  }

  int suggested_group_index = result.value();

  // Find the actual tab group ID for the suggested group index
  std::vector<tab_groups::TabGroupId> existing_groups;
  std::set<tab_groups::TabGroupId> processed_groups;

  for (int i = 0; i < count(); ++i) {
    auto group_id = GetTabGroupForTab(i);
    if (group_id.has_value() &&
        processed_groups.find(group_id.value()) == processed_groups.end()) {
      processed_groups.insert(group_id.value());
      existing_groups.push_back(group_id.value());
    }
  }

  if (suggested_group_index < 0 ||
      suggested_group_index >= static_cast<int>(existing_groups.size())) {
    return;  // Invalid group index
  }

  tab_groups::TabGroupId target_group = existing_groups[suggested_group_index];

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
