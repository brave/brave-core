/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/utility/importer/brave_importer.h"

#include <memory>
#include <vector>

#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "brave/common/importer/brave_stats.h"
#include "chrome/common/importer/importer_bridge.h"
#include "chrome/grit/generated_resources.h"
#include "components/autofill/core/common/password_form.h"
#include "components/cookie_config/cookie_store_util.h"
#include "components/os_crypt/os_crypt.h"
#include "components/password_manager/core/browser/login_database.h"
#include "components/password_manager/core/common/password_manager_pref_names.h"
#include "components/prefs/json_pref_store.h"
#include "components/prefs/pref_filter.h"
#include "net/cookies/canonical_cookie.h"
#include "net/cookies/cookie_constants.h"
#include "net/extras/sqlite/cookie_crypto_delegate.h"
#include "net/extras/sqlite/sqlite_persistent_cookie_store.h"
#include "sql/database.h"
#include "sql/statement.h"
#include "url/gurl.h"

#if defined(OS_LINUX)
#include "components/os_crypt/key_storage_config_linux.h"
#endif

#if defined(USE_X11)
#if defined(USE_LIBSECRET)
#include "chrome/browser/password_manager/native_backend_libsecret.h"
#endif
#include "chrome/browser/password_manager/native_backend_kwallet_x.h"
#include "chrome/browser/password_manager/password_store_x.h"
#include "components/os_crypt/key_storage_util_linux.h"
#endif

using base::Time;

BraveImporter::BraveImporter() {
}

BraveImporter::~BraveImporter() {
}

void BraveImporter::StartImport(const importer::SourceProfile& source_profile,
				uint16_t items,
				ImporterBridge* bridge) {
  bridge_ = bridge;
  source_path_ = source_profile.source_path;

  // The order here is important!
  bridge_->NotifyStarted();

  if ((items & importer::HISTORY) && !cancelled()) {
    bridge_->NotifyItemStarted(importer::HISTORY);
    ImportHistory();
    bridge_->NotifyItemEnded(importer::HISTORY);
  }

  if ((items & importer::FAVORITES) && !cancelled()) {
    bridge_->NotifyItemStarted(importer::FAVORITES);
    ImportBookmarks();
    bridge_->NotifyItemEnded(importer::FAVORITES);
  }

  if ((items & importer::PASSWORDS) && !cancelled()) {
    bridge_->NotifyItemStarted(importer::PASSWORDS);
    ImportPasswords(base::FilePath(FILE_PATH_LITERAL("UserPrefs")));
    bridge_->NotifyItemEnded(importer::PASSWORDS);
  }

  if ((items & importer::COOKIES) && !cancelled()) {
    bridge_->NotifyItemStarted(importer::COOKIES);
    ImportCookies();
    bridge_->NotifyItemEnded(importer::COOKIES);
  }

  if ((items & importer::STATS) && !cancelled()) {
    bridge_->NotifyItemStarted(importer::STATS);
    ImportStats();
    bridge_->NotifyItemEnded(importer::STATS);
  }

  if ((items & importer::LEDGER) && !cancelled()) {
    bridge_->NotifyItemStarted(importer::LEDGER);
    ImportLedger();
    bridge_->NotifyItemEnded(importer::LEDGER);
  }

  bridge_->NotifyEnded();
}

void BraveImporter::ImportHistory() {
  std::unique_ptr<base::Value> session_store_json = ParseBraveStateFile(
      "session-store-1");
  if (!session_store_json)
    return;

  const base::Value* history_sites =
      session_store_json->FindKeyOfType("historySites",
                                        base::Value::Type::DICTIONARY);
  if (!history_sites)
    return;

  std::vector<ImporterURLRow> rows;
  for (const auto& item : history_sites->DictItems()) {
    const auto& value = item.second;
    if (!value.is_dict())
      continue;

    const base::Value* location =
        value.FindKeyOfType("location",
                            base::Value::Type::STRING);
    const base::Value* title =
        value.FindKeyOfType("title",
                            base::Value::Type::STRING);
    const base::Value* lastAccessedTime =
        value.FindKeyOfType("lastAccessedTime",
                            base::Value::Type::DOUBLE);
    const base::Value* count =
        value.FindKeyOfType("count",
                            base::Value::Type::INTEGER);
    if (!(location && title && lastAccessedTime && count))
      continue;

    ImporterURLRow row;
    row.url = GURL(location->GetString());
    row.title = base::UTF8ToUTF16(title->GetString());
    row.last_visit = base::Time::FromJsTime(lastAccessedTime->GetDouble());
    row.visit_count = count->GetInt();
    // Only visible URLs are stored in historySites
    row.hidden = false;
    // Brave browser-laptop doesn't store the typed count anywhere, so default to 0.
    row.typed_count = 0;

    rows.push_back(row);
  }

  if (!rows.empty() && !cancelled())
    bridge_->SetHistoryItems(rows, importer::VISIT_SOURCE_BRAVE_IMPORTED);
}

void BraveImporter::ParseBookmarks(
    std::vector<ImportedBookmarkEntry>* bookmarks) {
  std::unique_ptr<base::Value> session_store_json = ParseBraveStateFile(
      "session-store-1");
  if (!session_store_json)
    return;

  base::Value* bookmark_folders_dict =
    session_store_json->FindKeyOfType("bookmarkFolders",
                                      base::Value::Type::DICTIONARY);
  base::Value* bookmarks_dict =
    session_store_json->FindKeyOfType("bookmarks",
                                      base::Value::Type::DICTIONARY);
  base::Value* bookmark_order_dict =
    session_store_json->FindPathOfType({"cache", "bookmarkOrder"},
				       base::Value::Type::DICTIONARY);
  if (!(bookmark_folders_dict && bookmarks_dict && bookmark_order_dict))
    return;

  // Recursively load bookmarks from each of the top-level bookmarks
  // folders: "Bookmarks Toolbar" and "Other Bookmarks"
  std::vector<base::string16> path;
  RecursiveReadBookmarksFolder(base::UTF8ToUTF16("Bookmarks Toolbar"),
                               "0",
                               path,
                               true,
                               bookmark_folders_dict,
                               bookmarks_dict,
                               bookmark_order_dict,
                               bookmarks);

  RecursiveReadBookmarksFolder(base::UTF8ToUTF16("Other Bookmarks"),
                               "-1",
                               path,
                               false,
                               bookmark_folders_dict,
                               bookmarks_dict,
                               bookmark_order_dict,
                               bookmarks);
}

void BraveImporter::RecursiveReadBookmarksFolder(
  const base::string16 name,
  const std::string key,
  std::vector<base::string16> path,
  const bool in_toolbar,
  base::Value* bookmark_folders_dict,
  base::Value* bookmarks_dict,
  base::Value* bookmark_order_dict,
  std::vector<ImportedBookmarkEntry>* bookmarks) {
  // Add the name of the current folder to the path
  path.push_back(name);

  base::Value* bookmark_order =
    bookmark_order_dict->FindKeyOfType(key, base::Value::Type::LIST);
  if (!bookmark_order)
    return;

  for (const auto& entry : bookmark_order->GetList()) {
    auto& type = entry.FindKeyOfType("type", base::Value::Type::STRING)->GetString();
    auto& key = entry.FindKeyOfType("key", base::Value::Type::STRING)->GetString();

     if (type == "bookmark-folder") {
      base::Value* bookmark_folder =
        bookmark_folders_dict->FindKeyOfType(key, base::Value::Type::DICTIONARY);
      auto& title =
        bookmark_folder->FindKeyOfType("title", base::Value::Type::STRING)->GetString();

      // Empty folders don't have a corresponding entry in bookmark_order_dict,
      // which provides an easy way to test whether a folder is empty.
      base::Value* bookmark_order_entry =
        bookmark_order_dict->FindKeyOfType(key, base::Value::Type::LIST);

      if (bookmark_order_entry) {
        // Recurse into non-empty folder.
        RecursiveReadBookmarksFolder(base::UTF8ToUTF16(title),
                                     key,
                                     path,
                                     in_toolbar,
                                     bookmark_folders_dict,
                                     bookmarks_dict,
                                     bookmark_order_dict,
                                     bookmarks);
      } else {
        // Add ImportedBookmarkEntry for empty folder.
         ImportedBookmarkEntry imported_bookmark_folder;
        imported_bookmark_folder.is_folder = true;
        imported_bookmark_folder.in_toolbar = in_toolbar;
        imported_bookmark_folder.url = GURL();
        imported_bookmark_folder.path = path;
        imported_bookmark_folder.title = base::UTF8ToUTF16(title);
        // Brave doesn't specify a creation time for the folder.
        imported_bookmark_folder.creation_time = base::Time::Now();
        bookmarks->push_back(imported_bookmark_folder);
      }
    } else if (type == "bookmark") {
      base::Value* bookmark =
        bookmarks_dict->FindKeyOfType(key, base::Value::Type::DICTIONARY);
      auto& title =
        bookmark->FindKeyOfType("title", base::Value::Type::STRING)->GetString();
      auto& location =
        bookmark->FindKeyOfType("location", base::Value::Type::STRING)->GetString();

      ImportedBookmarkEntry imported_bookmark;
      imported_bookmark.is_folder = false;
      imported_bookmark.in_toolbar = in_toolbar;
      imported_bookmark.url = GURL(location);
      imported_bookmark.path = path;
      imported_bookmark.title = base::UTF8ToUTF16(title);
      // Brave doesn't specify a creation time for the bookmark.
      imported_bookmark.creation_time = base::Time::Now();
      bookmarks->push_back(imported_bookmark);
    }
  }
}

void BraveImporter::ImportBookmarks() {
  std::vector<ImportedBookmarkEntry> bookmarks;
  ParseBookmarks(&bookmarks);

  if (!bookmarks.empty() && !cancelled()) {
    const base::string16& first_folder_name =
      bridge_->GetLocalizedString(IDS_BOOKMARK_GROUP_FROM_BRAVE);
    bridge_->AddBookmarks(bookmarks, first_folder_name);
  }
}

std::unique_ptr<base::Value> BraveImporter::ParseBraveStateFile(
    const std::string& filename) {
  base::FilePath session_store_path = source_path_.AppendASCII(filename);
  std::string session_store_content;
  if (!ReadFileToString(session_store_path, &session_store_content)) {
    LOG(ERROR) << "Could not read file: " << session_store_path;
    return nullptr;
  }

  std::unique_ptr<base::Value> session_store_json =
    base::JSONReader::Read(session_store_content);
  if (!session_store_json) {
    LOG(ERROR) << "Could not parse JSON from file: " << session_store_path;
  }

  return session_store_json;
}

void BraveImporter::ImportStats() {
  std::unique_ptr<base::Value> session_store_json = ParseBraveStateFile(
      "session-store-1");
  if (!session_store_json)
    return;

  base::Value* adblock_count =
    session_store_json->FindPathOfType({"adblock", "count"},
                                       base::Value::Type::INTEGER);
  base::Value* trackingProtection_count =
    session_store_json->FindPathOfType({"trackingProtection", "count"},
                                       base::Value::Type::INTEGER);
  base::Value* httpsEverywhere_count =
    session_store_json->FindPathOfType({"httpsEverywhere", "count"},
                                       base::Value::Type::INTEGER);

  BraveStats stats;
  if (adblock_count) {
    stats.adblock_count = adblock_count->GetInt();
  }
  if (trackingProtection_count) {
    stats.trackingProtection_count = trackingProtection_count->GetInt();
  }
  if (httpsEverywhere_count) {
    stats.httpsEverywhere_count = httpsEverywhere_count->GetInt();
  }

  bridge_->UpdateStats(stats);
}

void BraveImporter::ImportLedger() {
  std::unique_ptr<base::Value> ledger_state_json = ParseBraveStateFile(
      "ledger-state.json");
  if (!ledger_state_json)
    return;

  LOG(ERROR) << "Successfully parsed Brave ledger-state.json in BraveImporter::ImportLedger";

  //bridge_->UpdateLedger(ledger);
}
