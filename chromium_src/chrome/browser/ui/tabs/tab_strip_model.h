/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_TAB_STRIP_MODEL_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_TAB_STRIP_MODEL_H_

#include "base/memory/raw_ptr.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/browser/tabs/tab_content_extractor.h"
#include "components/tab_groups/tab_group_id.h"
#include "content/public/browser/web_contents.h"
#include "third_party/abseil-cpp/absl/status/statusor.h"
#include "url/gurl.h"

class Browser;
class Profile;

namespace local_ai {
class TextEmbedder;
}

#define CommandCommerceProductSpecifications \
  CommandAddTabToSuggestedGroup, CommandCommerceProductSpecifications

#define ExecuteContextMenuCommand                                        \
  ExecuteContextMenuCommand_ChromiumImpl(int context_index,              \
                                         ContextMenuCommand command_id); \
                                                                         \
 private:                                                                \
  struct BraveTabStripTabData {                                          \
    int index;                                                           \
    std::u16string title;                                                \
    GURL url;                                                            \
    std::optional<tab_groups::TabGroupId> group_id;                      \
    raw_ptr<content::WebContents> web_contents;                          \
  };                                                                     \
                                                                         \
 public:                                                                 \
  void HandleAddTabToSuggestedGroupCommand(int context_index);           \
  void OnTextEmbedderInitializedForGroupCommand(                         \
      std::unique_ptr<local_ai::TextEmbedder, base::OnTaskRunnerDeleter> \
          text_embedder,                                                 \
      std::vector<BraveTabStripTabData> all_tabs_needing_content,        \
      std::vector<int> ungrouped_global_handles, int context_index,      \
      bool success);                                                     \
  void OnAllTabContentCollectedForGroupCommand(                          \
      std::unique_ptr<local_ai::TextEmbedder, base::OnTaskRunnerDeleter> \
          text_embedder,                                                 \
      std::vector<BraveTabStripTabData> all_tabs_needing_content,        \
      std::vector<int> ungrouped_global_handles, int context_index,      \
      std::vector<std::pair<int, tab_content_extractor::ExtractedData>>  \
          content_results);                                              \
  void OnSuggestGroupForTabResult(                                       \
      std::vector<int> tab_global_handles, int context_index,            \
      absl::StatusOr<tab_groups::TabGroupId> result);                    \
                                                                         \
 private:                                                                \
  bool ShouldTrackBrowser(Browser* browser, Profile* target_profile);    \
  void CollectTabsFromAllWindows(                                        \
      Profile* profile,                                                  \
      std::vector<BraveTabStripTabData>& all_tabs_needing_content,       \
      const std::vector<int>& ungrouped_global_handles);                 \
                                                                         \
 public:                                                                 \
  void ExecuteContextMenuCommand

#define IsContextMenuCommandEnabled                            \
  IsContextMenuCommandEnabled_ChromiumImpl(                    \
      int context_index, ContextMenuCommand command_id) const; \
  bool IsContextMenuCommandEnabled

#define SelectRelativeTab(...)            \
  virtual SelectRelativeTab(__VA_ARGS__); \
  friend class BraveTabStripModel

// Added another closing selected tabs method to handle close activ tab
// in split tab. We only want to close active tab from split tab.
#define CloseSelectedTabs(...)    \
  CloseSelectedTabs(__VA_ARGS__); \
  virtual void CloseSelectedTabsWithSplitView()

#define DraggingTabsSession DraggingTabsSessionChromium
#define IsReadLaterSupportedForAny virtual IsReadLaterSupportedForAny
#define UpdateWebContentsStateAt virtual UpdateWebContentsStateAt

#include <chrome/browser/ui/tabs/tab_strip_model.h>  // IWYU pragma: export

#undef UpdateWebContentsStateAt
#undef IsReadLaterSupportedForAny
#undef DraggingTabsSession
#undef CloseSelectedTabs
#undef SelectRelativeTab
#undef IsContextMenuCommandEnabled
#undef ExecuteContextMenuCommand
#undef CommandCommerceProductSpecifications

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_TAB_STRIP_MODEL_H_
