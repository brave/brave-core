/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/utility/importer/chrome_importer.h"

#include <memory>
#include <string>
#include <utility>

#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "build/build_config.h"
#include "chrome/common/importer/imported_bookmark_entry.h"
#include "chrome/common/importer/importer_bridge.h"
#include "chrome/common/importer/importer_url_row.h"
#include "chrome/utility/importer/favicon_reencode.h"
#include "components/autofill/core/common/password_form.h"
#include "components/os_crypt/os_crypt.h"
#include "components/password_manager/core/browser/login_database.h"
#include "components/password_manager/core/common/password_manager_pref_names.h"
#include "components/prefs/json_pref_store.h"
#include "components/prefs/pref_filter.h"
#include "sql/database.h"
#include "sql/statement.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"

#if defined(OS_LINUX)
#include "components/os_crypt/key_storage_config_linux.h"
#include "chrome/grit/chromium_strings.h"
#include "ui/base/l10n/l10n_util.h"
#endif  // defined(OS_LINUX)

#if defined(OS_WIN)
#include "base/base64.h"
#include "base/win/wincrypt_shim.h"
#endif

using base::Time;

namespace {

// Most of below code is copied from os_crypt_win.cc
#if defined(OS_WIN)
// Contains base64 random key encrypted with DPAPI.
const char kOsCryptEncryptedKeyPrefName[] = "os_crypt.encrypted_key";

// Key prefix for a key encrypted with DPAPI.
const char kDPAPIKeyPrefix[] = "DPAPI";

bool DecryptStringWithDPAPI(const std::string& ciphertext,
                            std::string* plaintext) {
  DATA_BLOB input;
  input.pbData =
      const_cast<BYTE*>(reinterpret_cast<const BYTE*>(ciphertext.data()));
  input.cbData = static_cast<DWORD>(ciphertext.length());

  DATA_BLOB output;
  BOOL result = CryptUnprotectData(&input, nullptr, nullptr, nullptr, nullptr,
                                   0, &output);
  if (!result) {
    PLOG(ERROR) << "Failed to decrypt";
    return false;
  }

  plaintext->assign(reinterpret_cast<char*>(output.pbData), output.cbData);
  LocalFree(output.pbData);
  return true;
}

// Return false if encryption key setting is failed.
// Fetch chrome's raw encryption key and use it to get chrome's password data.
bool SetEncryptionKeyForPasswordImporting(
    const base::FilePath& local_state_path) {
  std::string local_state_content;
  base::ReadFileToString(local_state_path, &local_state_content);
  base::Optional<base::Value> local_state =
      base::JSONReader::Read(local_state_content);
  if (auto* base64_encrypted_key =
          local_state->FindStringPath(kOsCryptEncryptedKeyPrefName)) {
    std::string encrypted_key_with_header;

    base::Base64Decode(*base64_encrypted_key, &encrypted_key_with_header);

    if (!base::StartsWith(encrypted_key_with_header, kDPAPIKeyPrefix,
                          base::CompareCase::SENSITIVE)) {
      return false;
    }
    std::string encrypted_key =
        encrypted_key_with_header.substr(sizeof(kDPAPIKeyPrefix) - 1);
    std::string key;
    // This DPAPI decryption can fail if the user's password has been reset
    // by an Administrator.
    if (DecryptStringWithDPAPI(encrypted_key, &key)) {
      OSCrypt::SetRawEncryptionKey(key);
      return true;
    }
  }

  return false;
}
#endif

class CreateCopyFile {
 public:
  explicit CreateCopyFile(const base::FilePath& original_file_path) {
    DCHECK(base::PathExists(original_file_path));
    if (base::CreateTemporaryFile(&copied_file_path_))
      base::CopyFile(original_file_path, copied_file_path_);
  }

  ~CreateCopyFile() {
    base::DeleteFile(copied_file_path_, false);
  }

  CreateCopyFile(const CreateCopyFile&) = delete;
  CreateCopyFile& operator=(const CreateCopyFile&) = delete;

  base::FilePath copied_file_path() const { return copied_file_path_; }

 private:
  base::FilePath copied_file_path_;
};

}  // namespace

ChromeImporter::ChromeImporter() {
}

ChromeImporter::~ChromeImporter() {
}

void ChromeImporter::StartImport(const importer::SourceProfile& source_profile,
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
    ImportPasswords();
    bridge_->NotifyItemEnded(importer::PASSWORDS);
  }

  bridge_->NotifyEnded();
}

void ChromeImporter::ImportHistory() {
  base::FilePath history_path = source_path_.Append(
      base::FilePath::StringType(FILE_PATH_LITERAL("History")));
  if (!base::PathExists(history_path))
    return;

  CreateCopyFile copy_history_file(history_path);
  sql::Database db;
  if (!db.Open(copy_history_file.copied_file_path())) {
    return;
  }

  const char query[] =
    "SELECT u.url, u.title, v.visit_time, u.typed_count, u.visit_count "
    "FROM urls u JOIN visits v ON u.id = v.url "
    "WHERE hidden = 0 "
    "AND (transition & ?) != 0 "  // CHAIN_END
    "AND (transition & ?) NOT IN (?, ?, ?)";  // No SUBFRAME or
                                              // KEYWORD_GENERATED

  sql::Statement s(db.GetUniqueStatement(query));
  s.BindInt64(0, ui::PAGE_TRANSITION_CHAIN_END);
  s.BindInt64(1, ui::PAGE_TRANSITION_CORE_MASK);
  s.BindInt64(2, ui::PAGE_TRANSITION_AUTO_SUBFRAME);
  s.BindInt64(3, ui::PAGE_TRANSITION_MANUAL_SUBFRAME);
  s.BindInt64(4, ui::PAGE_TRANSITION_KEYWORD_GENERATED);

  std::vector<ImporterURLRow> rows;
  while (s.Step() && !cancelled()) {
    GURL url(s.ColumnString(0));

    ImporterURLRow row(url);
    row.title = s.ColumnString16(1);
    row.last_visit =
      base::Time::FromDoubleT(chromeTimeToDouble((s.ColumnInt64(2))));
    row.hidden = false;
    row.typed_count = s.ColumnInt(3);
    row.visit_count = s.ColumnInt(4);

    rows.push_back(row);
  }

  if (!rows.empty() && !cancelled())
    bridge_->SetHistoryItems(rows, importer::VISIT_SOURCE_CHROME_IMPORTED);
}

void ChromeImporter::ImportBookmarks() {
  std::string bookmarks_content;
  base::FilePath bookmarks_path =
    source_path_.Append(
      base::FilePath::StringType(FILE_PATH_LITERAL("Bookmarks")));
  CreateCopyFile copy_bookmark_file(bookmarks_path);
  base::ReadFileToString(copy_bookmark_file.copied_file_path(),
                         &bookmarks_content);
  base::Optional<base::Value> bookmarks_json =
    base::JSONReader::Read(bookmarks_content);
  const base::DictionaryValue* bookmark_dict;
  if (!bookmarks_json || !bookmarks_json->GetAsDictionary(&bookmark_dict))
    return;
  std::vector<ImportedBookmarkEntry> bookmarks;
  const base::DictionaryValue* roots;
  const base::DictionaryValue* bookmark_bar;
  const base::DictionaryValue* other;
  if (bookmark_dict->GetDictionary("roots", &roots)) {
    // Importing bookmark bar items
    if (roots->GetDictionary("bookmark_bar", &bookmark_bar)) {
      std::vector<base::string16> path;
      base::string16 name;
      bookmark_bar->GetString("name", &name);

      path.push_back(name);
      RecursiveReadBookmarksFolder(bookmark_bar, path, true, &bookmarks);
    }
    // Importing other items
    if (roots->GetDictionary("other", &other)) {
      std::vector<base::string16> path;
      base::string16 name;
      other->GetString("name", &name);

      path.push_back(name);
      RecursiveReadBookmarksFolder(other, path, false, &bookmarks);
    }
  }
  // Write into profile.
  if (!bookmarks.empty() && !cancelled()) {
    const base::string16& first_folder_name =
      base::UTF8ToUTF16("Imported from Chrome");
    bridge_->AddBookmarks(bookmarks, first_folder_name);
  }

  // Import favicons.
  base::FilePath favicons_path =
    source_path_.Append(
      base::FilePath::StringType(FILE_PATH_LITERAL("Favicons")));
  if (!base::PathExists(favicons_path))
    return;

  CreateCopyFile copy_favicon_file(favicons_path);

  sql::Database db;
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

void ChromeImporter::ImportFaviconURLs(
  sql::Database* db,
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
  const char query[] = "SELECT f.url, fb.image_data "
                       "FROM favicons f "
                       "JOIN favicon_bitmaps fb "
                       "ON f.id = fb.icon_id "
                       "WHERE f.id = ?;";
  sql::Statement s(db->GetUniqueStatement(query));

  if (!s.is_valid())
    return;

  for (FaviconMap::const_iterator i = favicon_map.begin();
       i != favicon_map.end(); ++i) {
    s.BindInt64(0, i->first);
    if (s.Step()) {
      favicon_base::FaviconUsageData usage;

      usage.favicon_url = GURL(s.ColumnString(0));
      if (!usage.favicon_url.is_valid())
        continue;  // Don't bother importing favicons with invalid URLs.

      std::vector<unsigned char> data;
      s.ColumnBlobAsVector(1, &data);
      if (data.empty())
        continue;  // Data definitely invalid.

      if (!importer::ReencodeFavicon(&data[0], data.size(), &usage.png_data))
        continue;  // Unable to decode.

      usage.urls = i->second;
      favicons->push_back(usage);
    }
    s.Reset(true);
  }
}

void ChromeImporter::RecursiveReadBookmarksFolder(
  const base::DictionaryValue* folder,
  const std::vector<base::string16>& parent_path,
  bool is_in_toolbar,
  std::vector<ImportedBookmarkEntry>* bookmarks) {
  const base::ListValue* children;
  if (folder->GetList("children", &children)) {
    for (const auto& value : *children) {
      const base::DictionaryValue* dict;
      if (!value.GetAsDictionary(&dict))
        continue;
      std::string date_added, type, url;
      base::string16 name;
      dict->GetString("date_added", &date_added);
      dict->GetString("name", &name);
      dict->GetString("type", &type);
      dict->GetString("url", &url);
      ImportedBookmarkEntry entry;
      if (type == "folder") {
        // Folders are added implicitly on adding children, so we only
        // explicitly add empty folders.
        const base::ListValue* children;
        if (dict->GetList("children", &children) && children->empty()) {
          entry.in_toolbar = is_in_toolbar;
          entry.is_folder = true;
          entry.url = GURL();
          entry.path = parent_path;
          entry.title = name;
          entry.creation_time =
            base::Time::FromDoubleT(chromeTimeToDouble(std::stoll(date_added)));
          bookmarks->push_back(entry);
        }

        std::vector<base::string16> path = parent_path;
        path.push_back(name);
        RecursiveReadBookmarksFolder(dict, path, is_in_toolbar, bookmarks);
      } else if (type == "url") {
        entry.in_toolbar = is_in_toolbar;
        entry.is_folder = false;
        entry.url = GURL(url);
        entry.path = parent_path;
        entry.title = name;
        entry.creation_time =
          base::Time::FromDoubleT(chromeTimeToDouble(std::stoll(date_added)));
        bookmarks->push_back(entry);
      }
    }
  }
}

double ChromeImporter::chromeTimeToDouble(int64_t time) {
  return ((time * 10 - 0x19DB1DED53E8000) / 10000) / 1000;
}

void ChromeImporter::ImportPasswords() {
#if defined(OS_LINUX)
  // Set up crypt config.
  std::unique_ptr<os_crypt::Config> config(new os_crypt::Config());
  config->product_name = l10n_util::GetStringUTF8(IDS_PRODUCT_NAME);
  config->should_use_preference = false;
  config->user_data_path = source_path_;
  OSCrypt::SetConfig(std::move(config));
#endif

#if defined(OS_WIN)
  base::FilePath local_state_path = source_path_.DirName().Append(
      base::FilePath::StringType(FILE_PATH_LITERAL("Local State")));
  if (!base::PathExists(local_state_path))
    return;
  if (!SetEncryptionKeyForPasswordImporting(local_state_path))
    return;
#endif

  base::FilePath passwords_path = source_path_.Append(
      base::FilePath::StringType(FILE_PATH_LITERAL("Login Data")));

  if (!base::PathExists(passwords_path))
    return;

  CreateCopyFile copy_password_file(passwords_path);
  password_manager::LoginDatabase database(
      copy_password_file.copied_file_path(),
      password_manager::IsAccountStore(false));
  if (!database.Init()) {
    LOG(ERROR) << "LoginDatabase Init() failed";
    return;
  }

  std::vector<std::unique_ptr<autofill::PasswordForm>> forms;
  bool success = database.GetAutofillableLogins(&forms);
  if (success) {
    for (size_t i = 0; i < forms.size(); ++i) {
      bridge_->SetPasswordForm(*forms[i].get());
    }
  }
  std::vector<std::unique_ptr<autofill::PasswordForm>> blacklist;
  success = database.GetBlacklistLogins(&blacklist);
  if (success) {
    for (size_t i = 0; i < blacklist.size(); ++i) {
      bridge_->SetPasswordForm(*blacklist[i].get());
    }
  }
}
