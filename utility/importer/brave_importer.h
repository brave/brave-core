/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_UTILITY_IMPORTER_BRAVE_IMPORTER_H_
#define BRAVE_UTILITY_IMPORTER_BRAVE_IMPORTER_H_

#include <stdint.h>

#include <map>
#include <vector>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/nix/xdg_util.h"
#include "base/values.h"
#include "brave/utility/importer/chrome_importer.h"
#include "build/build_config.h"
#include "chrome/common/importer/imported_bookmark_entry.h"

class BraveImporter : public ChromeImporter {
 public:
  BraveImporter();

  // Importer:
  void StartImport(const importer::SourceProfile& source_profile,
                   uint16_t items,
                   ImporterBridge* bridge) override;

 private:
  ~BraveImporter() override;

  void ImportBookmarks() override;
  void ImportHistory() override;
  void ImportStats();
  void ImportLedger();
  
  std::unique_ptr<base::Value> ParseBraveSessionStore();

  void ParseBookmarks(std::vector<ImportedBookmarkEntry>* bookmarks);
  void RecursiveReadBookmarksFolder(
    const base::string16 name,
    const std::string key,
    std::vector<base::string16> path,
    const bool in_toolbar,
    base::Value* bookmark_folders_dict,
    base::Value* bookmarks_dict,
    base::Value* bookmark_order_dict,
    std::vector<ImportedBookmarkEntry>* bookmarks);

  DISALLOW_COPY_AND_ASSIGN(BraveImporter);
};

#endif  // BRAVE_UTILITY_IMPORTER_BRAVE_IMPORTER_H_
