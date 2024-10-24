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
#include "base/memory/ref_counted.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/common/importer/scoped_copy_file.h"
#include "brave/grit/brave_generated_resources.h"
#include "brave/utility/importer/brave_external_process_importer_bridge.h"
#include "build/build_config.h"
#include "chrome/common/importer/imported_bookmark_entry.h"
#include "chrome/common/importer/importer_bridge.h"
#include "chrome/common/importer/importer_data_types.h"
#include "chrome/common/importer/importer_url_row.h"
#include "chrome/utility/importer/favicon_reencode.h"
#include "components/os_crypt/sync/os_crypt.h"
#include "components/password_manager/core/browser/password_form.h"
#include "components/password_manager/core/browser/password_store/login_database.h"
#include "components/password_manager/core/common/password_manager_pref_names.h"
#include "components/prefs/json_pref_store.h"
#include "components/prefs/pref_filter.h"
#include "components/webdata/common/webdata_constants.h"
#include "sql/database.h"
#include "sql/statement.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"

#if BUILDFLAG(IS_LINUX)
#include "chrome/grit/branded_strings.h"
#include "components/os_crypt/sync/key_storage_config_linux.h"
#endif  // BUILDFLAG(IS_LINUX)

#if BUILDFLAG(IS_WIN)
#include "base/base64.h"
#include "base/win/wincrypt_shim.h"
#endif

using base::Time;

namespace {

// Most of below code is copied from os_crypt_win.cc
#if BUILDFLAG(IS_WIN)
// Contains base64 random key encrypted with DPAPI.
constexpr char kOsCryptEncryptedKeyPrefName[] = "os_crypt.encrypted_key";

// Key prefix for a key encrypted with DPAPI.
constexpr char kDPAPIKeyPrefix[] = "DPAPI";

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
  std::optional<base::Value> local_state =
      base::JSONReader::Read(local_state_content);
  if (!local_state || !local_state->is_dict()) {
    return false;
  }

  if (auto* base64_encrypted_key =
          local_state->GetDict().FindStringByDottedPath(
              kOsCryptEncryptedKeyPrefName)) {
    std::string encrypted_key_with_header;

    base::Base64Decode(*base64_encrypted_key, &encrypted_key_with_header);

    if (!encrypted_key_with_header.starts_with(kDPAPIKeyPrefix)) {
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

bool SetEncryptionKey(const base::FilePath& source_path) {
#if BUILDFLAG(IS_LINUX)
  // Set up crypt config.
  std::unique_ptr<os_crypt::Config> config(new os_crypt::Config());
  config->product_name = l10n_util::GetStringUTF8(IDS_PRODUCT_NAME);
  config->should_use_preference = false;
  config->user_data_path = source_path;
  OSCrypt::SetConfig(std::move(config));
  return true;
#elif BUILDFLAG(IS_WIN)
  base::FilePath local_state_path = source_path.Append(
      base::FilePath::StringType(FILE_PATH_LITERAL("Local State")));
  if (!base::PathExists(local_state_path))
    return false;
  if (!SetEncryptionKeyForPasswordImporting(local_state_path))
    return false;
  return true;
#else
  return true;
#endif
}

std::u16string DecryptedCardFromColumn(sql::Statement* s, int column_index) {
  std::u16string credit_card_number;
  std::string encrypted_number;
  s->ColumnBlobAsString(column_index, &encrypted_number);
  if (!encrypted_number.empty()) {
    OSCrypt::DecryptString16(encrypted_number, &credit_card_number);
  }
  return credit_card_number;
}

bool PasswordFormToImportedPasswordForm(
    const password_manager::PasswordForm& form,
    importer::ImportedPasswordForm& imported_form) {
  if (form.scheme != password_manager::PasswordForm::Scheme::kHtml &&
      form.scheme != password_manager::PasswordForm::Scheme::kBasic) {
    return false;
  }

  if (form.scheme == password_manager::PasswordForm::Scheme::kHtml) {
    imported_form.scheme = importer::ImportedPasswordForm::Scheme::kHtml;
  } else {
    imported_form.scheme = importer::ImportedPasswordForm::Scheme::kBasic;
  }

  imported_form.signon_realm = form.signon_realm;
  imported_form.url = form.url;
  imported_form.action = form.action;
  imported_form.username_element = form.username_element;
  imported_form.username_value = form.username_value;
  imported_form.password_element = form.password_element;
  imported_form.password_value = form.password_value;
  imported_form.blocked_by_user = form.blocked_by_user;
  return true;
}

}  // namespace

ChromeImporter::ChromeImporter() = default;

ChromeImporter::~ChromeImporter() = default;

void ChromeImporter::StartImport(const importer::SourceProfile& source_profile,
                                 uint16_t items,
                                 ImporterBridge* bridge) {
  bridge_ = bridge;
  source_path_ = source_profile.source_path;
  importer_name_ = source_profile.importer_name;
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

#if BUILDFLAG(IS_WIN)
  auto source_path = source_profile.importer_type == importer::TYPE_OPERA
                         ? source_path_
                         : source_path_.DirName();
#else
  auto source_path = source_path_;
#endif
  const bool set_encryption_key = SetEncryptionKey(source_path);
  if ((items & importer::PASSWORDS) && !cancelled() && set_encryption_key) {
    bridge_->NotifyItemStarted(importer::PASSWORDS);
    ImportPasswords(base::FilePath(FILE_PATH_LITERAL("Login Data")));
    ImportPasswords(
        base::FilePath(FILE_PATH_LITERAL("Login Data For Account")));
    bridge_->NotifyItemEnded(importer::PASSWORDS);
  }

  if ((items & importer::PAYMENTS) && !cancelled() && set_encryption_key) {
    bridge_->NotifyItemStarted(importer::PAYMENTS);
    ImportPayments();
    bridge_->NotifyItemEnded(importer::PAYMENTS);
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

  sql::Database db;
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

  std::vector<ImporterURLRow> rows;
  while (s.Step() && !cancelled()) {
    GURL url(s.ColumnString(0));

    ImporterURLRow row(url);
    row.title = s.ColumnString16(1);
    row.last_visit = base::Time::FromSecondsSinceUnixEpoch(
        chromeTimeToDouble((s.ColumnInt64(2))));
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
  base::FilePath bookmarks_path = source_path_.Append(
      base::FilePath::StringType(FILE_PATH_LITERAL("Bookmarks")));
  ScopedCopyFile copy_bookmark_file(bookmarks_path);
  if (!copy_bookmark_file.copy_success())
    return;

  base::ReadFileToString(copy_bookmark_file.copied_file_path(),
                         &bookmarks_content);
  std::optional<base::Value> bookmarks_json =
      base::JSONReader::Read(bookmarks_content);
  if (!bookmarks_json)
    return;
  const base::Value::Dict* bookmark_dict = bookmarks_json->GetIfDict();
  if (!bookmark_dict)
    return;

  std::vector<ImportedBookmarkEntry> bookmarks;
  const base::Value::Dict* roots = bookmark_dict->FindDict("roots");
  if (roots) {
    // Importing bookmark bar items
    const base::Value::Dict* bookmark_bar = roots->FindDict("bookmark_bar");
    if (bookmark_bar) {
      std::vector<std::u16string> path;
      const auto* name = bookmark_bar->FindString("name");

      path.push_back(base::UTF8ToUTF16(name ? *name : std::string()));
      RecursiveReadBookmarksFolder(bookmark_bar, path, true, &bookmarks);
    }
    // Importing other items
    const base::Value::Dict* other = roots->FindDict("other");
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

      std::vector<uint8_t> data;
      s.ColumnBlobAsVector(1, &data);
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
    const base::Value::Dict* folder,
    const std::vector<std::u16string>& parent_path,
    bool is_in_toolbar,
    std::vector<ImportedBookmarkEntry>* bookmarks) {
  const base::Value::List* children = folder->FindList("children");
  if (children) {
    for (const auto& value : *children) {
      const base::Value::Dict* dict = value.GetIfDict();
      if (!dict)
        continue;
      const auto* date_added = dict->FindString("date_added");
      const auto* name_found = dict->FindString("name");
      auto name = base::UTF8ToUTF16(name_found ? *name_found : std::string());
      const auto* type = dict->FindString("type");
      const auto* url = dict->FindString("url");
      ImportedBookmarkEntry entry;
      if (type && *type == "folder") {
        // Folders are added implicitly on adding children, so we only
        // explicitly add empty folders.
        const base::Value::List* inner_children = dict->FindList("children");
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

double ChromeImporter::chromeTimeToDouble(int64_t time) {
  return ((time * 10 - 0x19DB1DED53E8000) / 10000) / 1000;
}

void ChromeImporter::ImportPasswords(
    const base::FilePath& passwords_file_name) {
  base::FilePath passwords_path = source_path_.Append(passwords_file_name);

  if (!base::PathExists(passwords_path))
    return;

  ScopedCopyFile copy_password_file(passwords_path);
  if (!copy_password_file.copy_success())
    return;

  password_manager::LoginDatabase database(
      copy_password_file.copied_file_path(),
      password_manager::IsAccountStore(false));
  if (!database.Init(
          /*on_undecryptable_passwords_removed=*/base::NullCallback(),
          /*encryptor=*/nullptr)) {
    LOG(ERROR) << "LoginDatabase Init() failed";
    return;
  }

  std::vector<password_manager::PasswordForm> forms;
  bool success = database.GetAutofillableLogins(&forms);
  if (success) {
    for (auto& entry : forms) {
      importer::ImportedPasswordForm form;
      if (PasswordFormToImportedPasswordForm(entry, form)) {
        bridge_->SetPasswordForm(form);
      }
    }
  }
  std::vector<password_manager::PasswordForm> blocklist;
  success = database.GetBlocklistLogins(&blocklist);
  if (success) {
    for (auto& entry : blocklist) {
      importer::ImportedPasswordForm form;
      if (PasswordFormToImportedPasswordForm(entry, form)) {
        bridge_->SetPasswordForm(form);
      }
    }
  }
}

void ChromeImporter::ImportPayments() {
  const base::FilePath payments_path = source_path_.Append(kWebDataFilename);

  if (!base::PathExists(payments_path))
    return;

  ScopedCopyFile copy_payments_file(payments_path);
  if (!copy_payments_file.copy_success())
    return;

  sql::Database db;
  if (!db.Open(copy_payments_file.copied_file_path())) {
    return;
  }

  const char query[] =
      "SELECT name_on_card, expiration_month, expiration_year, "
      "card_number_encrypted, origin "
      "FROM credit_cards;";
  sql::Statement s(db.GetUniqueStatement(query));
  auto* brave_bridge =
      static_cast<BraveExternalProcessImporterBridge*>(bridge_.get());
  while (s.Step()) {
    const std::u16string card_number = DecryptedCardFromColumn(&s, 3);
    // Empty means decryption is failed. Or chrome's data is invalid.
    // Skip it.
    if (card_number.empty())
      continue;
    brave_bridge->SetCreditCard(s.ColumnString16(0), s.ColumnString16(1),
                                s.ColumnString16(2), card_number,
                                s.ColumnString(4));
  }
}
