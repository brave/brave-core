/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/utility/importer/brave_importer.h"

#include <memory>
#include <string>
#include <vector>

#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"

#include "brave/common/importer/brave_ledger.h"
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

#include "brave/vendor/bat-native-ledger/src/static_values.h"

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
    // NOTE: RecoverWallet is async.
    // Its handler will call NotifyItemEnded/NotifyEnded
    ImportLedger();
    return;
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

// TODO: not used
std::vector<uint8_t> ParseWalletSeed(const base::Value& ledger_state_json) {
  const base::Value* wallet_seed_json = ledger_state_json.FindPathOfType(
      {"properties", "wallet", "keyinfo", "seed"},
      base::Value::Type::DICTIONARY);
  if (!wallet_seed_json) {
    LOG(ERROR) << "Failed to parse wallet seed from ledger state";
    return std::vector<uint8_t>();
  }

  // The wallet seed is a 32-byte Uint8Array, encoded in JSON as a
  // dictionary of integers.
  std::vector<uint8_t> wallet_seed;
  for (int i = 0; i < 32; i++) {
    const base::Value* value_json = wallet_seed_json->FindKey(std::to_string(i));
    if (!value_json) {
      LOG(ERROR) << "Expected seed byte not found: " << i;
      return std::vector<uint8_t>();
    }

    int value = value_json->GetInt();
    if (value < 0 || value > 255) {
      LOG(ERROR) << "Seed byte at index " << i << " out of range: " << value;
      return std::vector<uint8_t>();;
    }

    wallet_seed.push_back(static_cast<uint8_t>(value));
  }

  return wallet_seed;
}

bool ParseWalletPassphrase(BraveLedger& ledger, const base::Value& session_store_json) {
  const base::Value* wallet_passphrase_value = session_store_json.FindPathOfType(
      {"ledger", "info", "passphrase"},
      base::Value::Type::STRING);
  if (!wallet_passphrase_value) {
    LOG(ERROR) << "Wallet passphrase not found in session-store-1";
    return false;
  }

  ledger.passphrase = wallet_passphrase_value->GetString();
  return !ledger.passphrase.empty();
}

bool TryFindBoolKey(const base::Value* dict, const std::string key, bool& value_to_set) {
  auto* value_read = dict->FindKeyOfType(key, base::Value::Type::BOOLEAN);
  if (value_read) {
    value_to_set = value_read->GetBool();
    return true;
  }
  return false;
}

bool TryFindStringKey(const base::Value* dict, const std::string key, std::string& value_to_set) {
  auto* value_read = dict->FindKeyOfType(key, base::Value::Type::STRING);
  if (value_read) {
    value_to_set = value_read->GetString();
    return true;
  }
  return false;
}

bool TryFindIntKey(const base::Value* dict, const std::string key, int& value_to_set) {
  auto* value_read = dict->FindKeyOfType(key, base::Value::Type::INTEGER);
  if (value_read) {
    value_to_set = value_read->GetInt();
    return true;
  }
  return false;
}

bool ParsePaymentsPreferences(BraveLedger& ledger, const base::Value& session_store_json) {
  const base::Value* settings = session_store_json.FindKeyOfType(
    "settings",
    base::Value::Type::DICTIONARY);
  if (!settings) {
    LOG(ERROR) << "No entry \"settings\" found in session-store-1";
    return false;
  }

  auto* payments = &ledger.settings.payments;

  TryFindBoolKey(settings, "payments.enabled", payments->enabled);
  TryFindBoolKey(settings, "payments.allow-non-verified-publishers", payments->allow_non_verified);
  TryFindBoolKey(settings, "payments.allow-media-publishers", payments->allow_media_publishers);

  std::string contribution_amount = "";
  TryFindStringKey(settings, "payments.contribution-amount", contribution_amount);
  if (!contribution_amount.empty()) {
    if (!base::StringToDouble(contribution_amount, &payments->contribution_amount)) {
      LOG(ERROR) << "StringToDouble failed when converting \"settings.payments.contribution-amount\"; unable to convert value \"" << contribution_amount << "\"; defaulting value.";
    }
  }

  std::string minimum_visits = "";
  TryFindStringKey(settings, "payments.minimum-visits", minimum_visits);
  if (!minimum_visits.empty()) {
    if (!base::StringToUint(minimum_visits, &payments->min_visits)) {
      LOG(ERROR) << "StringToUint failed when converting \"settings.payments.minimum-visits\"; unable to convert value \"" << minimum_visits << "\"; defaulting value.";
    }
  }
  switch (payments->min_visits) {
    // allowed values
    case 1:
    case 5:
    case 10:
    break;

    default:
      payments->min_visits = 1u;
  }

  std::string minumum_visit_time = "";
  TryFindStringKey(settings, "payments.minimum-visit-time", minumum_visit_time);
  if (!minumum_visit_time.empty()) {
    if (!base::StringToUint64(minumum_visit_time, &payments->min_visit_time)) {
      LOG(ERROR) << "StringToUint64 failed when converting \"settings.payments.minimum-visit-time\"; unable to convert value \"" << minumum_visit_time << "\"; defaulting value.";
    }
  }
  switch (payments->min_visit_time) {
    // allowed values
    case 5000: payments->min_visit_time = 5; break;
    case 8000: payments->min_visit_time = 8; break;
    case 60000: payments->min_visit_time = 60; break;

    default:
      payments->min_visit_time = braveledger_ledger::_default_min_publisher_duration;
  }

  return true;
}

bool ParseExcludedSites(BraveLedger& ledger, const base::Value& session_store_json) {
  const base::Value* site_settings = session_store_json.FindKeyOfType(
    "siteSettings",
    base::Value::Type::DICTIONARY);
  if (!site_settings) {
    LOG(ERROR) << "No entry \"siteSettings\" found in session-store-1";
    return false;
  }

  ledger.excluded_publishers = std::vector<std::string>();
  bool host_pattern_included;

  for (const auto& item : site_settings->DictItems()) {
    const auto& host_pattern = item.first;
    const auto& settings = item.second;

    if (!settings.is_dict())
      continue;

    if (TryFindBoolKey(&settings, "ledgerPayments", host_pattern_included)) {
      if (!host_pattern_included) {
        // host pattern is in a format like: `https?://travis-ci.org`
        // The protocol part can be removed (to get publisher key)
        size_t protocol_index = host_pattern.find("//");
        if (protocol_index != std::string::npos) {
          ledger.excluded_publishers.push_back(host_pattern.substr(protocol_index + 2));
        }
      }
    }
  }

  return true;
}

bool ParsePinnedSites(BraveLedger& ledger, const base::Value& session_store_json) {
  const base::Value* publishers = session_store_json.FindPathOfType(
      {"ledger", "synopsis", "publishers"},
      base::Value::Type::DICTIONARY);
  if (!publishers) {
    LOG(ERROR) << "\"ledger\".\"synopsis\".\"publishers\" not found in session-store-1";
    return false;
  }

  ledger.pinned_publishers = std::map<std::string, unsigned int>();
  int pin_percentage;

  for (const auto& item : publishers->DictItems()) {
    const auto& domain = item.first;
    const auto& settings = item.second;

    if (!settings.is_dict())
      continue;

    if (TryFindIntKey(&settings, "pinPercentage", pin_percentage)) {
      if (pin_percentage > 0) {
        ledger.pinned_publishers.insert(
          std::pair<std::string, unsigned int>(domain, pin_percentage));
      }
    }
  }

  return true;
}

void BraveImporter::ImportLedger(bool clobber_wallet) {
  std::unique_ptr<base::Value> session_store_json = ParseBraveStateFile(
      "session-store-1");
  std::unique_ptr<base::Value> ledger_state_json = ParseBraveStateFile(
      "ledger-state.json");
  if (!(session_store_json && ledger_state_json))
    return;

  BraveLedger ledger;
  ledger.clobber_wallet = clobber_wallet;

  if (!ParseWalletPassphrase(ledger, *session_store_json)) {
    // TODO: ...
    LOG(ERROR) << "Failed to parse wallet passphrase";
  }

  if (!ParsePaymentsPreferences(ledger, *session_store_json)) {
    // TODO: ...
    LOG(ERROR) << "Failed to parse Brave Payment preferences";
  }

  if (!ParseExcludedSites(ledger, *session_store_json)) {
    // TODO: ...
    LOG(ERROR) << "Failed to parse list of excluded sites for Brave Payments";
  }

  if (!ParsePinnedSites(ledger, *session_store_json)) {
    // TODO: ...
    LOG(ERROR) << "Failed to parse list of pinned sites for Brave Payments";
  }

  bridge_->UpdateLedger(ledger);
}