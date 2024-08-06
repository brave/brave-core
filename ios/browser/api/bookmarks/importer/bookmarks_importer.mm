/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/bookmarks/importer/bookmarks_importer.h"

#include <set>

#include "base/base_paths.h"
#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/path_service.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/ios/browser/api/bookmarks/importer/imported_bookmark_entry.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_node.h"
#include "components/bookmarks/browser/bookmark_utils.h"
#include "components/bookmarks/common/bookmark_pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/strings/grit/components_strings.h"
#include "components/sync/base/features.h"
#include "components/user_prefs/user_prefs.h"
#include "ios/chrome/browser/bookmarks/model/bookmark_model_factory.h"
#include "ios/chrome/browser/shared/model/application_context/application_context.h"
#include "ios/chrome/browser/shared/model/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/shared/model/browser_state/chrome_browser_state_manager.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/gurl.h"

using bookmarks::BookmarkModel;
using bookmarks::BookmarkNode;

namespace {

// Generates a unique folder name. If |folder_name| is not unique, then this
// repeatedly tests for '|folder_name| + (i)' until a unique name is found.
std::u16string GenerateUniqueFolderName(BookmarkModel* model,
                                        const std::u16string& folder_name) {
  // Build a set containing the bookmark bar folder names.
  std::set<std::u16string> existing_folder_names;
  const BookmarkNode* bookmark_bar = model->bookmark_bar_node();
  for (const auto& node : bookmark_bar->children()) {
    if (node->is_folder())
      existing_folder_names.insert(node->GetTitle());
  }

  // If the given name is unique, use it.
  if (existing_folder_names.find(folder_name) == existing_folder_names.end())
    return folder_name;

  // Otherwise iterate until we find a unique name.
  for (size_t i = 1; i <= existing_folder_names.size(); ++i) {
    std::u16string name =
        folder_name + u" (" + base::NumberToString16(i) + u")";
    if (existing_folder_names.find(name) == existing_folder_names.end())
      return name;
  }

  NOTREACHED_IN_MIGRATION();
  return folder_name;
}

// Shows the bookmarks toolbar.
void ShowBookmarkBar(ChromeBrowserState* browser_state) {
  browser_state->GetPrefs()->SetBoolean(bookmarks::prefs::kShowBookmarkBar,
                                        true);
}

}  // namespace

void BookmarksImporter::AddBookmarks(
    const std::u16string& top_level_folder_name,
    const std::vector<ImportedBookmarkEntry>& bookmarks) {
  if (bookmarks.empty())
    return;

  ChromeBrowserStateManager* browser_state_manager =
      GetApplicationContext()->GetChromeBrowserStateManager();
  ChromeBrowserState* browser_state =
      browser_state_manager->GetLastUsedBrowserStateDeprecatedDoNotUse();
  bookmarks::BookmarkModel* model =
      ios::BookmarkModelFactory::GetForBrowserState(browser_state);
  DCHECK(model->loaded());

  // If the bookmark bar is currently empty, we should import directly to it.
  // Otherwise, we should import everything to a subfolder.
  // For iOS, import into the Mobile Bookmarks Node - Brandon T.
  const BookmarkNode* bookmark_bar = model->mobile_node();
  bool import_to_top_level = bookmark_bar->children().empty();

  // Reorder bookmarks so that the toolbar entries come first.
  std::vector<ImportedBookmarkEntry> toolbar_bookmarks;
  std::vector<ImportedBookmarkEntry> reordered_bookmarks;
  for (auto it = bookmarks.begin(); it != bookmarks.end(); ++it) {
    if (it->in_toolbar)
      toolbar_bookmarks.push_back(*it);
    else
      reordered_bookmarks.push_back(*it);
  }
  reordered_bookmarks.insert(reordered_bookmarks.begin(),
                             toolbar_bookmarks.begin(),
                             toolbar_bookmarks.end());

  // If the user currently has no bookmarks in the bookmark bar, make sure that
  // at least some of the imported bookmarks end up there.  Otherwise, we'll end
  // up with just a single folder containing the imported bookmarks, which makes
  // for unnecessary nesting.
  bool add_all_to_top_level = import_to_top_level && toolbar_bookmarks.empty();

  model->BeginExtensiveChanges();

  std::set<const BookmarkNode*> folders_added_to;
  const BookmarkNode* top_level_folder = NULL;
  for (std::vector<ImportedBookmarkEntry>::const_iterator bookmark =
           reordered_bookmarks.begin();
       bookmark != reordered_bookmarks.end(); ++bookmark) {
    // Disregard any bookmarks with invalid urls.
    if (!bookmark->is_folder && !bookmark->url.is_valid())
      continue;

    const BookmarkNode* parent = NULL;
    if (import_to_top_level && (add_all_to_top_level || bookmark->in_toolbar)) {
      // Add directly to the bookmarks bar.
      parent = bookmark_bar;
    } else {
      // Add to a folder that will contain all the imported bookmarks not added
      // to the bar.  The first time we do so, create the folder.
      if (!top_level_folder) {
        std::u16string name =
            GenerateUniqueFolderName(model, top_level_folder_name);
        top_level_folder = model->AddFolder(
            bookmark_bar, bookmark_bar->children().size(), name);
      }
      parent = top_level_folder;
    }

    // Ensure any enclosing folders are present in the model.  The bookmark's
    // enclosing folder structure should be
    //   path[0] > path[1] > ... > path[size() - 1]
    for (auto folder_name = bookmark->path.begin();
         folder_name != bookmark->path.end(); ++folder_name) {
      if (bookmark->in_toolbar && parent == bookmark_bar &&
          folder_name == bookmark->path.begin()) {
        // If we're importing directly to the bookmarks bar, skip over the
        // folder named "Bookmarks Toolbar" (or any non-Firefox equivalent).
        continue;
      }

      const auto it = base::ranges::find_if(
          parent->children(), [folder_name](const auto& node) {
            return node->is_folder() && node->GetTitle() == *folder_name;
          });
      parent = (it == parent->children().cend())
                   ? model->AddFolder(parent, parent->children().size(),
                                      *folder_name)
                   : it->get();
    }

    folders_added_to.insert(parent);
    if (bookmark->is_folder) {
      model->AddFolder(parent, parent->children().size(), bookmark->title);
    } else {
      model->AddURL(parent, parent->children().size(), bookmark->title,
                    bookmark->url, nullptr, bookmark->creation_time);
    }
  }

  // In order to keep the imported-to folders from appearing in the 'recently
  // added to' combobox, reset their modified times.
  for (auto i = folders_added_to.begin(); i != folders_added_to.end(); ++i) {
    model->ResetDateFolderModified(*i);
  }

  model->EndExtensiveChanges();

  // If the user was previously using a toolbar, we should show the bar.
  if (import_to_top_level && !add_all_to_top_level)
    ShowBookmarkBar(browser_state);
}
