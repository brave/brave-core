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
#include "brave/grit/generated_resources.h"
#include "chrome/common/importer/importer_bridge.h"
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
#include "sql/connection.h"
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

base::nix::DesktopEnvironment BraveImporter::GetDesktopEnvironment() {
  std::unique_ptr<base::Environment> env(base::Environment::Create());
  return base::nix::GetDesktopEnvironment(env.get());
}
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
    ImportPasswords();
    bridge_->NotifyItemEnded(importer::PASSWORDS);
  }

  if ((items & importer::COOKIES) && !cancelled()) {
    bridge_->NotifyItemStarted(importer::COOKIES);
    ImportCookies();
    bridge_->NotifyItemEnded(importer::COOKIES);
  }

  bridge_->NotifyEnded();
}

// Returns true if |url| has a valid scheme that we allow to import. We
// filter out the URL with a unsupported scheme.
bool CanImportURL(const GURL& url) {
  // The URL is not valid.
  if (!url.is_valid())
    return false;

  // Filter out the URLs with unsupported schemes.
  const char* const kInvalidSchemes[] = {"chrome-extension"};
  for (size_t i = 0; i < arraysize(kInvalidSchemes); ++i) {
    if (url.SchemeIs(kInvalidSchemes[i]))
      return false;
  }

  return true;
}

void BraveImporter::ImportHistory() {
  base::FilePath history_path =
    source_path_.Append(
      base::FilePath::StringType(FILE_PATH_LITERAL("History")));
  if (!base::PathExists(history_path))
    return;

  sql::Connection db;
  if (!db.Open(history_path))
    return;

  const char query[] =
    "SELECT url, title, last_visit_time, typed_count, visit_count "
    "FROM urls WHERE hidden = 0";

  sql::Statement s(db.GetUniqueStatement(query));

  std::vector<ImporterURLRow> rows;
  while (s.Step() && !cancelled()) {
    GURL url(s.ColumnString(0));

    // Filter out unwanted URLs.
    if (!CanImportURL(url))
      continue;

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
    bridge_->SetHistoryItems(rows, importer::VISIT_SOURCE_BRAVE_IMPORTED);
}

void BraveImporter::ParseBookmarks(
    std::vector<ImportedBookmarkEntry>* bookmarks) {
  base::FilePath session_store_path =
    source_path_.Append(
      base::FilePath::StringType(FILE_PATH_LITERAL("session-store-1")));
  std::string session_store_content;
  if (!ReadFileToString(session_store_path, &session_store_content)) {
    LOG(ERROR) << "Reading Brave session store file failed";
    return;
  }

  std::unique_ptr<base::Value> session_store_json =
    base::JSONReader::Read(session_store_content);
  if (!session_store_json) {
    LOG(ERROR) << "Parsing Brave session store JSON failed";
    return;
  }

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

double BraveImporter::chromeTimeToDouble(int64_t time) {
  return ((time * 10 - 0x19DB1DED53E8000) / 10000) / 1000;
}

void BraveImporter::ImportPasswords() {
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
  base::FilePath prefs_path =
    source_path_.Append(
      base::FilePath::StringType(FILE_PATH_LITERAL("UserPrefs")));
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
	selected_backend == os_crypt::SelectedLinuxBackend::GNOME_LIBSECRET)) {
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

void BraveImporter::ImportCookies() {
  base::FilePath cookies_path =
    source_path_.Append(
      base::FilePath::StringType(FILE_PATH_LITERAL("Cookies")));
  if (!base::PathExists(cookies_path))
    return;

  sql::Connection db;
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
        static_cast<net::CookieSameSite>(s.ColumnInt(9)),  // samesite
        static_cast<net::CookiePriority>(s.ColumnInt(13)));  // priority
    if (cookie.IsCanonical()) {
      cookies.push_back(cookie);
    }
  }

  if (!cookies.empty() && !cancelled()) {
    bridge_->SetCookies(cookies);
  }
}
