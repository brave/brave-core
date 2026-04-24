/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/utility/importer/chrome_importer.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "brave/common/importer/scoped_copy_file.h"
#include "brave/grit/brave_generated_resources.h"
#include "brave/utility/importer/brave_external_process_importer_bridge.h"
#include "build/build_config.h"
#include "chrome/common/importer/importer_bridge.h"
#include "components/prefs/json_pref_store.h"
#include "components/prefs/pref_filter.h"
#include "components/user_data_importer/common/imported_bookmark_entry.h"
#include "components/user_data_importer/common/importer_data_types.h"
#include "components/user_data_importer/common/importer_url_row.h"
#include "components/user_data_importer/content/favicon_reencode.h"
#include "sql/database.h"
#include "sql/statement.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"

#if BUILDFLAG(IS_LINUX)
#include "chrome/grit/branded_strings.h"
#endif  // BUILDFLAG(IS_LINUX)

using base::Time;

namespace {

// Converts time expressed in microseconds since the Windows epoch (01-01-1601)
// to time expressed in seconds since the Unix epoch (01-01-1970).
double chromeTimeToDouble(int64_t time) {
  // Divide by 1'000'000.0 to convert to seconds,
  // then adjust by the number of seconds between the epochs.
  return time / 1'000'000.0 - 11644473600.0;
}

}  // namespace

ChromeImporter::ChromeImporter() = default;

ChromeImporter::~ChromeImporter() = default;

void ChromeImporter::StartImport(
    const user_data_importer::SourceProfile& source_profile,
    uint16_t items,
    ImporterBridge* bridge) {
  bridge_ = bridge;
  source_path_ = source_profile.source_path;
  importer_name_ = source_profile.importer_name;
  // The order here is important!
  bridge_->NotifyStarted();

  if ((items & user_data_importer::HISTORY) && !cancelled()) {
    bridge_->NotifyItemStarted(user_data_importer::HISTORY);
    ImportHistory();
    bridge_->NotifyItemEnded(user_data_importer::HISTORY);
  }

  if ((items & user_data_importer::FAVORITES) && !cancelled()) {
    bridge_->NotifyItemStarted(user_data_importer::FAVORITES);
    ImportBookmarks();
    bridge_->NotifyItemEnded(user_data_importer::FAVORITES);
  }

  bridge_->NotifyEnded();
}

void ChromeImporter::ImportHistory() {
  base::FilePath history_path = source_path_.Append(
      base::FilePath::StringType(FILE_PATH_LITERAL("History")));
  if (!base::PathExists(history_path))
    return;

  ScopedCopyFile copy_history_file(history_path);
  if (!copy_history_file.copy_success())
    return;

  sql::Database db(sql::Database::Tag("History"));
  if (!db.Open(copy_history_file.copied_file_path())) {
    return;
  }

  const char query[] =
      "SELECT u.url, u.title, v.visit_time, u.typed_count, u.visit_count "
      "FROM urls u JOIN visits v ON u.id = v.url "
      "WHERE hidden = 0 "
      "AND (transition & ?) != 0 "              // CHAIN_END
      "AND (transition & ?) NOT IN (?, ?, ?)";  // No SUBFRAME or
                                                // KEYWORD_GENERATED

  sql::Statement s(db.GetUniqueStatement(query));
  s.BindInt64(0, ui::PAGE_TRANSITION_CHAIN_END);
  s.BindInt64(1, ui::PAGE_TRANSITION_CORE_MASK);
  s.BindInt64(2, ui::PAGE_TRANSITION_AUTO_SUBFRAME);
  s.BindInt64(3, ui::PAGE_TRANSITION_MANUAL_SUBFRAME);
  s.BindInt64(4, ui::PAGE_TRANSITION_KEYWORD_GENERATED);

  std::vector<user_data_importer::ImporterURLRow> rows;
  while (s.Step() && !cancelled()) {
    GURL url(s.ColumnString(0));

    user_data_importer::ImporterURLRow row(url);
    row.title = s.ColumnString16(1);
    row.last_visit = base::Time::FromSecondsSinceUnixEpoch(
        chromeTimeToDouble((s.ColumnInt64(2))));
    row.hidden = false;
    row.typed_count = s.ColumnInt(3);
    row.visit_count = s.ColumnInt(4);

    rows.push_back(row);
  }

  if (!rows.empty() && !cancelled())
    bridge_->SetHistoryItems(rows,
                             user_data_importer::VISIT_SOURCE_CHROME_IMPORTED);
}

void ChromeImporter::ImportBookmarks() {
  std::string bookmarks_content;
  base::FilePath bookmarks_path = source_path_.Append(
      base::FilePath::StringType(FILE_PATH_LITERAL("Bookmarks")));
  ScopedCopyFile copy_bookmark_file(bookmarks_path);
  if (!copy_bookmark_file.copy_success())
    return;

  base::ReadFileToString(copy_bookmark_file.copied_file_path(),
                         &bookmarks_content);
  std::optional<base::DictValue> bookmark_dict = base::JSONReader::ReadDict(
      bookmarks_content, base::JSON_PARSE_CHROMIUM_EXTENSIONS);
  if (!bookmark_dict)
    return;

  std::vector<user_data_importer::ImportedBookmarkEntry> bookmarks;
  const base::DictValue* roots = bookmark_dict->FindDict("roots");
  if (roots) {
    // Importing bookmark bar items
    const base::DictValue* bookmark_bar = roots->FindDict("bookmark_bar");
    if (bookmark_bar) {
      std::vector<std::u16string> path;
      const auto* name = bookmark_bar->FindString("name");

      path.push_back(base::UTF8ToUTF16(name ? *name : std::string()));
      RecursiveReadBookmarksFolder(bookmark_bar, path, true, &bookmarks);
    }
    // Importing other items
    const base::DictValue* other = roots->FindDict("other");
    if (other) {
      std::vector<std::u16string> path;
      const auto* name = other->FindString("name");

      path.push_back(base::UTF8ToUTF16(name ? *name : std::string()));
      RecursiveReadBookmarksFolder(other, path, false, &bookmarks);
    }
  }
  // Write into profile.
  if (!bookmarks.empty() && !cancelled()) {
    bridge_->AddBookmarks(
        bookmarks, l10n_util::GetStringFUTF16(IDS_IMPORTED_FROM_BOOKMARK_FOLDER,
                                              importer_name_));
  }

  // Import favicons.
  base::FilePath favicons_path = source_path_.Append(
      base::FilePath::StringType(FILE_PATH_LITERAL("Favicons")));
  if (!base::PathExists(favicons_path))
    return;

  ScopedCopyFile copy_favicon_file(favicons_path);
  if (!copy_favicon_file.copy_success())
    return;

  sql::Database db(sql::Database::Tag("Favicons"));
  if (!db.Open(copy_favicon_file.copied_file_path()))
    return;

  FaviconMap favicon_map;
  ImportFaviconURLs(&db, &favicon_map);
  // Write favicons into profile.
  if (!favicon_map.empty() && !cancelled()) {
    favicon_base::FaviconUsageDataList favicons;
    LoadFaviconData(&db, favicon_map, &favicons);
    bridge_->SetFavicons(favicons);
  }
}

void ChromeImporter::ImportFaviconURLs(sql::Database* db,
                                       FaviconMap* favicon_map) {
  const char query[] = "SELECT icon_id, page_url FROM icon_mapping;";
  sql::Statement s(db->GetUniqueStatement(query));

  while (s.Step() && !cancelled()) {
    int64_t icon_id = s.ColumnInt64(0);
    GURL url = GURL(s.ColumnString(1));
    (*favicon_map)[icon_id].insert(url);
  }
}

void ChromeImporter::LoadFaviconData(
    sql::Database* db,
    const FaviconMap& favicon_map,
    favicon_base::FaviconUsageDataList* favicons) {
  const char query[] =
      "SELECT f.url, fb.image_data "
      "FROM favicons f "
      "JOIN favicon_bitmaps fb "
      "ON f.id = fb.icon_id "
      "WHERE f.id = ?;";
  sql::Statement s(db->GetUniqueStatement(query));

  if (!s.is_valid())
    return;

  for (const auto& entry : favicon_map) {
    s.BindInt64(0, entry.first);
    if (s.Step()) {
      favicon_base::FaviconUsageData usage;

      usage.favicon_url = GURL(s.ColumnString(0));
      if (!usage.favicon_url.is_valid())
        continue;  // Don't bother importing favicons with invalid URLs.

      std::vector<uint8_t> data = s.ColumnBlobAsVector(1);
      if (data.empty())
        continue;  // Data definitely invalid.

      auto decoded_data = importer::ReencodeFavicon(base::span(data));
      if (!decoded_data) {
        continue;  // Unable to decode.
      }

      usage.urls = entry.second;
      usage.png_data = std::move(decoded_data).value();
      favicons->push_back(usage);
    }
    s.Reset(true);
  }
}

void ChromeImporter::RecursiveReadBookmarksFolder(
    const base::DictValue* folder,
    const std::vector<std::u16string>& parent_path,
    bool is_in_toolbar,
    std::vector<user_data_importer::ImportedBookmarkEntry>* bookmarks) {
  const base::ListValue* children = folder->FindList("children");
  if (children) {
    for (const auto& value : *children) {
      const base::DictValue* dict = value.GetIfDict();
      if (!dict)
        continue;
      const auto* date_added = dict->FindString("date_added");
      const auto* name_found = dict->FindString("name");
      auto name = base::UTF8ToUTF16(name_found ? *name_found : std::string());
      const auto* type = dict->FindString("type");
      const auto* url = dict->FindString("url");
      user_data_importer::ImportedBookmarkEntry entry;
      if (type && *type == "folder") {
        // Folders are added implicitly on adding children, so we only
        // explicitly add empty folders.
        const base::ListValue* inner_children = dict->FindList("children");
        if (inner_children && inner_children->empty()) {
          entry.in_toolbar = is_in_toolbar;
          entry.is_folder = true;
          entry.url = GURL();
          entry.path = parent_path;
          entry.title = name;
          entry.creation_time = base::Time::FromSecondsSinceUnixEpoch(
              chromeTimeToDouble(std::stoll(*date_added)));
          bookmarks->push_back(entry);
        }

        std::vector<std::u16string> path = parent_path;
        path.push_back(name);
        RecursiveReadBookmarksFolder(dict, path, is_in_toolbar, bookmarks);
      } else if (type && *type == "url") {
        entry.in_toolbar = is_in_toolbar;
        entry.is_folder = false;
        entry.url = GURL(*url);
        entry.path = parent_path;
        entry.title = name;
        entry.creation_time = base::Time::FromSecondsSinceUnixEpoch(
            chromeTimeToDouble(std::stoll(*date_added)));
        bookmarks->push_back(entry);
      }
    }
  }
}
