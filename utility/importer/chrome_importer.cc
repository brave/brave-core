/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/utility/importer/chrome_importer.h"

#include <memory>
#include <string>

#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "brave/utility/importer/brave_external_process_importer_bridge.h"
#include "build/build_config.h"
#include "chrome/common/importer/imported_bookmark_entry.h"
#include "chrome/common/importer/importer_bridge.h"
#include "chrome/common/importer/importer_url_row.h"
#include "chrome/utility/importer/favicon_reencode.h"
#include "components/autofill/core/common/password_form.h"
#include "components/os_crypt/os_crypt.h"
#include "components/cookie_config/cookie_store_util.h"
#include "net/extras/sqlite/cookie_crypto_delegate.h"
#include "components/password_manager/core/browser/login_database.h"
#include "components/password_manager/core/common/password_manager_pref_names.h"
#include "components/prefs/json_pref_store.h"
#include "components/prefs/pref_filter.h"
#include "net/cookies/canonical_cookie.h"
#include "net/cookies/cookie_constants.h"
#include "net/extras/sqlite/sqlite_persistent_cookie_store.h"
#include "sql/database.h"
#include "sql/statement.h"
#include "ui/base/page_transition_types.h"
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

base::nix::DesktopEnvironment ChromeImporter::GetDesktopEnvironment() {
  std::unique_ptr<base::Environment> env(base::Environment::Create());
  return base::nix::GetDesktopEnvironment(env.get());
}
#endif

using base::Time;

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
    ImportPasswords(base::FilePath(FILE_PATH_LITERAL("Preferences")));
    bridge_->NotifyItemEnded(importer::PASSWORDS);
  }

  if ((items & importer::COOKIES) && !cancelled()) {
    bridge_->NotifyItemStarted(importer::COOKIES);
    ImportCookies();
    bridge_->NotifyItemEnded(importer::COOKIES);
  }

  bridge_->NotifyEnded();
}

void ChromeImporter::ImportHistory() {
  base::FilePath history_path =
    source_path_.Append(
      base::FilePath::StringType(FILE_PATH_LITERAL("History")));
  if (!base::PathExists(history_path))
    return;

  sql::Database db;
  if (!db.Open(history_path))
    return;

  const char query[] =
    "SELECT u.url, u.title, v.visit_time, u.typed_count, u.visit_count "
    "FROM urls u JOIN visits v ON u.id = v.url "
    "WHERE hidden = 0 "
    "AND (transition & ?) != 0 "  // CHAIN_END
    "AND (transition & ?) NOT IN (?, ?, ?)";  // No SUBFRAME or
                                              // KEYWORD_GENERATED

  sql::Statement s(db.GetUniqueStatement(query));
  s.BindInt(0, ui::PAGE_TRANSITION_CHAIN_END);
  s.BindInt(1, ui::PAGE_TRANSITION_CORE_MASK);
  s.BindInt(2, ui::PAGE_TRANSITION_AUTO_SUBFRAME);
  s.BindInt(3, ui::PAGE_TRANSITION_MANUAL_SUBFRAME);
  s.BindInt(4, ui::PAGE_TRANSITION_KEYWORD_GENERATED);

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
  base::ReadFileToString(bookmarks_path, &bookmarks_content);
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

  sql::Database db;
  if (!db.Open(favicons_path))
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

void ChromeImporter::ImportPasswords(const base::FilePath& prefs_filename) {
  #if !defined(USE_X11)
    base::FilePath passwords_path =
      source_path_.Append(
        base::FilePath::StringType(FILE_PATH_LITERAL("Login Data")));

    password_manager::LoginDatabase database(passwords_path);
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
  #else
    base::FilePath prefs_path = source_path_.Append(prefs_filename);
    const base::Value *value;
    scoped_refptr<JsonPrefStore> prefs = new JsonPrefStore(prefs_path);
    int local_profile_id;
    if (prefs->ReadPrefs() != PersistentPrefStore::PREF_READ_ERROR_NONE) {
      return;
    }
    if (!prefs->GetValue(password_manager::prefs::kLocalProfileId, &value)) {
      return;
    }
    if (!value->GetAsInteger(&local_profile_id)) {
      return;
    }

    std::unique_ptr<PasswordStoreX::NativeBackend> backend;
    base::nix::DesktopEnvironment desktop_env = GetDesktopEnvironment();

    // WIP proper kEnableEncryptionSelection
    os_crypt::SelectedLinuxBackend selected_backend =
        os_crypt::SelectBackend(std::string(), true, desktop_env);
    if (!backend &&
        (selected_backend == os_crypt::SelectedLinuxBackend::KWALLET ||
        selected_backend == os_crypt::SelectedLinuxBackend::KWALLET5)) {
      base::nix::DesktopEnvironment used_desktop_env =
          selected_backend == os_crypt::SelectedLinuxBackend::KWALLET
              ? base::nix::DESKTOP_ENVIRONMENT_KDE4
              : base::nix::DESKTOP_ENVIRONMENT_KDE5;
      backend.reset(new NativeBackendKWallet(local_profile_id,
                                             used_desktop_env));
    } else if (selected_backend == os_crypt::SelectedLinuxBackend::GNOME_ANY ||
               selected_backend ==
                   os_crypt::SelectedLinuxBackend::GNOME_KEYRING ||
               selected_backend ==
                   os_crypt::SelectedLinuxBackend::GNOME_LIBSECRET) {
  #if defined(USE_LIBSECRET)
      if (!backend &&
          (selected_backend == os_crypt::SelectedLinuxBackend::GNOME_ANY ||
           selected_backend ==
               os_crypt::SelectedLinuxBackend::GNOME_LIBSECRET)) {
        backend.reset(new NativeBackendLibsecret(local_profile_id));
      }
  #endif
    }
    if (backend && backend->Init()) {
      std::vector<std::unique_ptr<autofill::PasswordForm>> forms;
      bool success = backend->GetAutofillableLogins(&forms);
      if (success) {
        for (size_t i = 0; i < forms.size(); ++i) {
          bridge_->SetPasswordForm(*forms[i].get());
        }
      }
      std::vector<std::unique_ptr<autofill::PasswordForm>> blacklist;
      success = backend->GetBlacklistLogins(&blacklist);
      if (success) {
        for (size_t i = 0; i < blacklist.size(); ++i) {
          bridge_->SetPasswordForm(*blacklist[i].get());
        }
      }
    }
  #endif
}

void ChromeImporter::ImportCookies() {
  base::FilePath cookies_path =
    source_path_.Append(
      base::FilePath::StringType(FILE_PATH_LITERAL("Cookies")));
  if (!base::PathExists(cookies_path))
    return;

  sql::Database db;
  if (!db.Open(cookies_path))
    return;

  const char query[] =
    "SELECT creation_utc, host_key, name, value, encrypted_value, path, "
    "expires_utc, is_secure, is_httponly, firstpartyonly, last_access_utc, "
    "has_expires, is_persistent, priority FROM cookies";

  sql::Statement s(db.GetUniqueStatement(query));

  net::CookieCryptoDelegate* delegate =
    cookie_config::GetCookieCryptoDelegate();
#if defined(OS_LINUX)
  OSCrypt::SetConfig(std::make_unique<os_crypt::Config>());
#endif

  std::vector<net::CanonicalCookie> cookies;
  while (s.Step() && !cancelled()) {
    std::string encrypted_value = s.ColumnString(4);
    std::string value;
    if (!encrypted_value.empty() && delegate) {
      if (!delegate->DecryptString(encrypted_value, &value)) {
        continue;
      }
    } else {
      value = s.ColumnString(3);
    }

    auto cookie = net::CanonicalCookie(
        s.ColumnString(2),                           // name
        value,                                       // value
        s.ColumnString(1),                           // domain
        s.ColumnString(5),                           // path
        Time::FromInternalValue(s.ColumnInt64(0)),   // creation_utc
        Time::FromInternalValue(s.ColumnInt64(6)),   // expires_utc
        Time::FromInternalValue(s.ColumnInt64(10)),  // last_access_utc
        s.ColumnBool(7),                             // secure
        s.ColumnBool(8),                             // http_only
        static_cast<net::CookieSameSite>(s.ColumnInt(9)),    // samesite
        static_cast<net::CookiePriority>(s.ColumnInt(13)));  // priority
    if (cookie.IsCanonical()) {
      cookies.push_back(cookie);
    }
  }

  if (!cookies.empty() && !cancelled()) {
    bridge_->SetCookies(cookies);
  }
}
