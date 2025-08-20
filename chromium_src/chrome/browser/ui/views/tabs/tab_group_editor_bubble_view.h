/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_GROUP_EDITOR_BUBBLE_VIEW_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_GROUP_EDITOR_BUBBLE_VIEW_H_

#include <memory>

#include "base/task/sequenced_task_runner.h"
#include "brave/components/local_ai/browser/text_embedder.h"

// Inject our new member methods into the upstream class
#define RebuildMenuContents                                             \
  RebuildMenuContentsUnsed();                                           \
  std::unique_ptr<views::LabelButton> BuildSuggestedTabInGroupButton(); \
  void SuggestedTabsPressed();                                          \
                                                                        \
 public:                                                                \
  void MaybeAddSuggestedTabsButton();                                   \
                                                                        \
 private:                                                               \
  /* AI-powered tab suggestion functionality */                         \
  void OnTextEmbedderInitialized(bool initialized);                     \
  void ProcessTabSuggestion();                                          \
  void OnTabSuggestionResult(absl::StatusOr<std::vector<int>> result);  \
  void ShowSuggestionDialog(std::vector<int> suggested_tab_indices);    \
  void CleanupTextEmbedder();                                           \
  void CleanupAndClose();                                               \
                                                                        \
  /* Member variables for TextEmbedder functionality */                 \
  std::unique_ptr<local_ai::TextEmbedder, base::OnTaskRunnerDeleter>    \
      text_embedder_{nullptr, base::OnTaskRunnerDeleter(nullptr)};      \
  bool suggestion_in_progress_ = false;                                 \
                                                                        \
  void RebuildMenuContents

#define TAB_GROUP_HEADER_CXMENU_RECENT_ACTIVITY    \
  TAB_GROUP_HEADER_CXMENU_ADD_SUGGESTED_TABS = 11; \
  static constexpr int TAB_GROUP_HEADER_CXMENU_RECENT_ACTIVITY

#include <chrome/browser/ui/views/tabs/tab_group_editor_bubble_view.h>  // IWYU pragma: export

#undef TAB_GROUP_HEADER_CXMENU_RECENT_ACTIVITY
#undef RebuildMenuContents

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_GROUP_EDITOR_BUBBLE_VIEW_H_
