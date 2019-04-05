/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/utility/importer/brave_importer.h"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/stl_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"

#include "brave/common/importer/brave_ledger.h"
#include "brave/common/importer/brave_stats.h"
#include "brave/common/importer/brave_referral.h"
#include "brave/common/importer/imported_browser_window.h"
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

#include "bat/ledger/internal/static_values.h"

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
    uint16_t items, ImporterBridge* bridge) {
  bridge_ = bridge;
  source_path_ = source_profile.source_path;

  // The order here is important!
  bridge_->NotifyStarted();

  // NOTE: Some data is always imported (not configurable by user)
  // If data isn't found, settings are cleared or defaulted.
  ImportRequiredItems();

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

  if ((items & importer::WINDOWS) && !cancelled()) {
    bridge_->NotifyItemStarted(importer::WINDOWS);
    ImportWindows();
    bridge_->NotifyItemEnded(importer::WINDOWS);
  }

  if ((items & importer::LEDGER) && !cancelled()) {
    // `ImportLedger` returns true if "importable"
    if (ImportLedger()) {
      // NOTE: RecoverWallet is async.
      // Its handler will call NotifyItemEnded/NotifyEnded
      bridge_->NotifyItemStarted(importer::LEDGER);
      return;
    }
  }

  bridge_->NotifyEnded();
}

// Called before user-toggleable import items.
// These import types don't need a distinct checkbox in the import screen.
void BraveImporter::ImportRequiredItems() {
  ImportReferral();
  ImportSettings();
}

void BraveImporter::ImportHistory() {
  base::Optional<base::Value> session_store_json = ParseBraveStateFile(
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
    // Brave browser-laptop doesn't store the typed count anywhere
    // so default to 0.
    row.typed_count = 0;

    rows.push_back(row);
  }

  if (!rows.empty() && !cancelled())
    bridge_->SetHistoryItems(rows, importer::VISIT_SOURCE_BRAVE_IMPORTED);
}

void BraveImporter::ParseBookmarks(
    std::vector<ImportedBookmarkEntry>* bookmarks) {
  base::Optional<base::Value> session_store_json = ParseBraveStateFile(
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

  for (auto& entry : bookmark_order->GetList()) {
    base::Value* typeValue = entry.FindKeyOfType("type",
        base::Value::Type::STRING);
    base::Value* keyValue = entry.FindKeyOfType("key",
        base::Value::Type::STRING);
    if (!(typeValue && keyValue))
      continue;

    auto type = typeValue->GetString();
    auto key = keyValue->GetString();

    if (type == "bookmark-folder") {
      base::Value* bookmark_folder =
        bookmark_folders_dict->FindKeyOfType(key,
          base::Value::Type::DICTIONARY);
      if (!bookmark_folder)
        continue;

      base::Value* titleValue = bookmark_folder->FindKeyOfType("title",
          base::Value::Type::STRING);
      if (!titleValue)
        continue;

      auto title = titleValue->GetString();

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
      if (!bookmark)
        continue;

      base::Value* titleValue = bookmark->FindKeyOfType("title",
          base::Value::Type::STRING);
      base::Value* locationValue = bookmark->FindKeyOfType("location",
          base::Value::Type::STRING);
      if (!(titleValue && locationValue))
        continue;

      auto title = titleValue->GetString();
      auto location = locationValue->GetString();

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

base::Optional<base::Value> BraveImporter::ParseBraveStateFile(
    const std::string& filename) {
  base::FilePath session_store_path = source_path_.AppendASCII(filename);
  std::string session_store_content;
  if (!ReadFileToString(session_store_path, &session_store_content)) {
    LOG(ERROR) << "Could not read file: " << session_store_path;
    return base::nullopt;
  }

  base::Optional<base::Value> session_store_json =
    base::JSONReader::Read(session_store_content);
  if (!session_store_json) {
    LOG(ERROR) << "Could not parse JSON from file: " << session_store_path;
  }

  return session_store_json;
}

void BraveImporter::ImportStats() {
  base::Optional<base::Value> session_store_json = ParseBraveStateFile(
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

bool ParseWalletPassphrase(BraveLedger* ledger,
  const base::Value& session_store_json) {
  const base::Value* wallet_passphrase_value =
    session_store_json.FindPathOfType(
      {"ledger", "info", "passphrase"},
      base::Value::Type::STRING);
  if (!wallet_passphrase_value) {
    LOG(ERROR) << "Wallet passphrase not found in session-store-1";
    return false;
  }

  ledger->passphrase = wallet_passphrase_value->GetString();
  return !ledger->passphrase.empty();
}

bool TryFindBoolKey(const base::Value* dict,
  const std::string key, bool& value_to_set) {  // NOLINT
  auto* value_read = dict->FindKeyOfType(key, base::Value::Type::BOOLEAN);
  if (value_read) {
    value_to_set = value_read->GetBool();
    return true;
  }
  return false;
}

bool TryFindStringKey(const base::Value* dict,
  const std::string key, std::string& value_to_set) {  // NOLINT
  auto* value_read = dict->FindKeyOfType(key, base::Value::Type::STRING);
  if (value_read) {
    value_to_set = value_read->GetString();
    return true;
  }
  return false;
}

bool TryFindIntKey(const base::Value* dict, const std::string key,
  int& value_to_set) {  // NOLINT
  auto* value_read = dict->FindKeyOfType(key, base::Value::Type::INTEGER);
  if (value_read) {
    value_to_set = value_read->GetInt();
    return true;
  }
  return false;
}

bool TryFindUInt64Key(const base::Value* dict,
    const std::string key, uint64_t& value_to_set) {  // NOLINT
  auto* value_read = dict->FindKeyOfType(key, base::Value::Type::DOUBLE);
  if (value_read) {
    value_to_set = (uint64_t)value_read->GetDouble();
    return true;
  }
  return false;
}

bool ParsePaymentsPreferences(BraveLedger* ledger,
  const base::Value& session_store_json) {
  const base::Value* settings = session_store_json.FindKeyOfType(
    "settings",
    base::Value::Type::DICTIONARY);
  if (!settings) {
    LOG(ERROR) << "No entry \"settings\" found in session-store-1";
    return false;
  }

  auto* payments = &ledger->settings.payments;

  // Boolean prefs. If any of these settings are missing,
  // let's fall back to the default value from browser-laptop.
  // (see browser-laptop/js/constants/appConfig.js for more info)
  if (!TryFindBoolKey(settings, "payments.enabled", payments->enabled)) {
    payments->enabled = false;
  }

  if (!TryFindBoolKey(settings, "payments.allow-non-verified-publishers",
    payments->allow_non_verified)) {
    payments->allow_non_verified = true;
  }

  if (!TryFindBoolKey(settings, "payments.allow-media-publishers",
    payments->allow_media_publishers)) {
    payments->allow_media_publishers = true;
  }

  // Contribution amount
  // TODO(brave): Get default amount from rewards service
  const int default_monthly_contribution = 20;
  std::string contribution_amount = "";
  payments->contribution_amount = -1;
  TryFindStringKey(settings, "payments.contribution-amount",
    contribution_amount);
  if (!contribution_amount.empty()) {
    if (!base::StringToDouble(contribution_amount,
      &payments->contribution_amount)) {
      LOG(ERROR) << "StringToDouble failed when converting "
        << "\"settings.payments.contribution-amount\"; unable to convert "
        << "value \"" << contribution_amount << "\"; defaulting value.";
    }
  }

  // Fall back to default value if contribution amount is missing/out of range.
  // If user never modified (using the UI) the contribution amount, it won't
  // be present in the session-store-1. This was intended so that we can change
  // the default amount. Once user changes it, value was then locked in.
  if (payments->contribution_amount < 1 ||
    payments->contribution_amount > 500) {
    payments->contribution_amount = default_monthly_contribution;
  }

  // Minimum number of visits for a site to be considered relevant
  std::string minimum_visits = "";
  TryFindStringKey(settings, "payments.minimum-visits", minimum_visits);
  if (!minimum_visits.empty()) {
    if (!base::StringToUint(minimum_visits, &payments->min_visits)) {
      LOG(ERROR) << "StringToUint failed when converting "
        << "\"settings.payments.minimum-visits\"; unable to convert "
        << "value \"" << minimum_visits << "\"; defaulting value.";
    }
  }

  if (payments->min_visits != 1 &&
      payments->min_visits != 5 &&
      payments->min_visits != 10) {
    payments->min_visits = 1u;
  }

  // Minimum visit time at a site to be considered relevant
  std::string minumum_visit_time = "";
  TryFindStringKey(settings, "payments.minimum-visit-time", minumum_visit_time);
  if (!minumum_visit_time.empty()) {
    if (!base::StringToUint64(minumum_visit_time, &payments->min_visit_time)) {
      LOG(ERROR) << "StringToUint64 failed when converting "
        << "\"settings.payments.minimum-visit-time\"; unable to convert "
        << "value \"" << minumum_visit_time << "\"; defaulting value.";
    }
  }
  switch (payments->min_visit_time) {
    // allowed values
    case 5000:
    case 8000:
    case 60000:
      payments->min_visit_time /= 1000;
      break;

    default:
      payments->min_visit_time =
        braveledger_ledger::_default_min_publisher_duration;
  }

  return true;
}

bool ParseExcludedSites(BraveLedger* ledger,
  const base::Value& session_store_json) {
  const base::Value* site_settings = session_store_json.FindKeyOfType(
    "siteSettings",
    base::Value::Type::DICTIONARY);
  if (!site_settings) {
    LOG(ERROR) << "No entry \"siteSettings\" found in session-store-1";
    return false;
  }

  ledger->excluded_publishers = std::vector<std::string>();
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
          ledger->excluded_publishers.push_back(
            host_pattern.substr(protocol_index + 2));
        }
      }
    }
  }

  return true;
}

// implemented in C++20
bool ends_with(const std::string &input, const std::string &test) {
  if (input.length() >= test.length()) {
    return (0 == input.compare (input.length() - test.length(), test.length(),
          test));
  }
  return false;
}

bool ParsePinnedSites(BraveLedger* ledger,
  const base::Value& session_store_json) {
  const base::Value* publishers = session_store_json.FindPathOfType(
      {"ledger", "about", "synopsis"}, base::Value::Type::LIST);
  if (!publishers) {
    LOG(ERROR)
      << "\"ledger\".\"about\".\"synopsis\" not found in session-store-1";
    return false;
  }

  ledger->pinned_publishers = std::vector<BravePublisher>();

  for (const auto& item : publishers->GetList()) {
    BravePublisher publisher;

    // Publisher key is required; if not present, skip this object.
    if (!TryFindStringKey(&item, "publisherKey", publisher.key)) {
      continue;
    }

    // Read any entries with pinPercentage > 0
    if (TryFindIntKey(&item, "pinPercentage", publisher.pin_percentage)) {
      if (publisher.pin_percentage > 0) {
        // Read publisher fields from synopsis; provide default values on error
        if (!TryFindBoolKey(&item, "verified", publisher.verified)) {
          publisher.verified = false;
        }
        if (!TryFindStringKey(&item, "siteName", publisher.name)) {
          publisher.name = publisher.key;
        }
        if (!TryFindStringKey(&item, "providerName", publisher.provider)) {
          publisher.provider = "";
        }

        // Publisher URL is required; if found, persist this object.
        if (TryFindStringKey(&item, "publisherURL", publisher.url)) {
          if (!ends_with(publisher.url, "/")) {
            publisher.url += "/";
          }
          ledger->pinned_publishers.push_back(publisher);
        }
      }
    }
  }

  return true;
}

bool BraveImporter::ImportLedger() {
  base::Optional<base::Value> session_store_json = ParseBraveStateFile(
      "session-store-1");
  base::Optional<base::Value> ledger_state_json = ParseBraveStateFile(
      "ledger-state.json");
  if (!(session_store_json && ledger_state_json)) {
    return false;
  }

  BraveLedger ledger;

  if (!ParsePaymentsPreferences(&ledger, *session_store_json)) {
    LOG(ERROR) << "Failed to parse preferences for Brave Payments";
    return false;
  }

  // It should be considered fatal if an error occurs while
  // parsing any of the below expected fields. This could
  // indicate a corrupt session-store-1
  if (!ParseWalletPassphrase(&ledger, *session_store_json)) {
    LOG(ERROR) << "Failed to parse wallet passphrase";
    return false;
  }

  if (!ledger.settings.payments.enabled) {
    LOG(INFO) << "Skipping `Brave Payments` import (feature was disabled)";
    return false;
  }

  // only do the import if Brave Payments is enabled
  if (!ParseExcludedSites(&ledger, *session_store_json)) {
    LOG(ERROR) << "Failed to parse list of excluded sites for Brave Payments";
    return false;
  }

  if (!ParsePinnedSites(&ledger, *session_store_json)) {
    LOG(ERROR) << "Failed to parse list of pinned sites for Brave Payments";
    return false;
  }

  bridge_->UpdateLedger(ledger);
  return true;
}

void BraveImporter::ImportReferral() {
  base::Optional<base::Value> session_store_json = ParseBraveStateFile(
      "session-store-1");
  if (!session_store_json) {
    return;
  }

  const base::Value* updates = session_store_json->FindKeyOfType(
      "updates",
      base::Value::Type::DICTIONARY);
  if (!updates) {
    LOG(ERROR) << "No entry \"updates\" found in session-store-1";
    return;
  }

  BraveReferral referral;

  // Read as many values as possible (defaulting to "" or 0)
  // After 90 days, the `promoCode` field is erased (so it's not
  // always there). `referralTimestamp` is only present after those
  // 90 days elapse. Week of installation should always be present
  // but if missing, it shouldn't cancel the import.
  if (!TryFindStringKey(updates, "referralPromoCode", referral.promo_code)) {
    referral.promo_code = "";
  }

  if (!TryFindStringKey(updates, "referralDownloadId", referral.download_id)) {
    referral.download_id = "";
  }

  if (!TryFindUInt64Key(updates, "referralTimestamp",
        referral.finalize_timestamp)) {
    referral.finalize_timestamp = 0;
  }

  if (!TryFindStringKey(updates, "weekOfInstallation",
        referral.week_of_installation)) {
    referral.week_of_installation = "";
  }

  bridge_->UpdateReferral(referral);
}

bool CanImportURL(const GURL& url) {
  if (!url.is_valid())
    return false;

  const char* const kInvalidSchemes[] = {"about", "chrome-extension"};
  for (size_t i = 0; i < base::size(kInvalidSchemes); i++) {
    if (url.SchemeIs(kInvalidSchemes[i]))
      return false;
  }

  return true;
}

std::vector<ImportedBrowserTab> ParseTabs(const base::Value* frames) {
  std::vector<ImportedBrowserTab> tabs;

  for (const auto& frame : frames->GetList()) {
    auto* key = frame.FindKeyOfType("key",
        base::Value::Type::INTEGER);
    auto* location = frame.FindKeyOfType("location",
        base::Value::Type::STRING);

    if (!(key && location))
      continue;

    auto url = GURL(location->GetString());
    // Filter internal URLs from Muon that won't resolve correctly in brave-core
    if (!CanImportURL(url))
      continue;

    ImportedBrowserTab tab;
    tab.key = key->GetInt();
    tab.location = url;
    tabs.push_back(tab);
  }

  return tabs;
}

std::vector<ImportedBrowserWindow> ParseWindows(
    const base::Value* perWindowState) {
  std::vector<ImportedBrowserWindow> windows;

  for (const auto& entry : perWindowState->GetList()) {
    ImportedBrowserWindow window;

    auto* windowInfo = entry.FindKeyOfType("windowInfo",
        base::Value::Type::DICTIONARY);
    auto* activeFrameKey = entry.FindKeyOfType("activeFrameKey",
        base::Value::Type::INTEGER);
    auto* frames = entry.FindKeyOfType("frames",
        base::Value::Type::LIST);

    if (!(frames && activeFrameKey && windowInfo))
      continue;

    // Window info
    auto* top = windowInfo->FindKeyOfType("top",
        base::Value::Type::INTEGER);
    auto* left = windowInfo->FindKeyOfType("left",
        base::Value::Type::INTEGER);
    auto* width = windowInfo->FindKeyOfType("width",
        base::Value::Type::INTEGER);
    auto* height = windowInfo->FindKeyOfType("height",
        base::Value::Type::INTEGER);
    auto* focused = windowInfo->FindKeyOfType("focused",
        base::Value::Type::BOOLEAN);
    auto* type = windowInfo->FindKeyOfType("type",
        base::Value::Type::STRING);
    auto* state = windowInfo->FindKeyOfType("state",
        base::Value::Type::STRING);

    if (!(top && left && width && height && focused && type && state)) {
      LOG(WARNING) << "windowInfo failed validation, skipping window";
      continue;
    }

    // "type" is one of: "normal", "popup", or "devtools"
    if (type->GetString() != "normal") {
      LOG(INFO) << "windowInfo type not normal, skipping window";
      continue;
    }

    auto tabs = ParseTabs(frames);
    if (tabs.size() == 0)
      continue;

    window.top = top->GetInt();
    window.left = left->GetInt();
    window.width = width->GetInt();
    window.height = height->GetInt();
    window.focused = focused->GetBool();
    window.state = state->GetString();
    window.activeFrameKey = activeFrameKey->GetInt();
    window.tabs = tabs;

    windows.push_back(window);
  }

  return windows;
}

std::vector<ImportedBrowserTab> ParsePinnedTabs(
    const base::Value* pinnedSites) {
  std::vector<ImportedBrowserTab> pinnedTabs;

  for (const auto& item : pinnedSites->DictItems()) {
    const auto& value = item.second;
    if (!value.is_dict())
      continue;

    const base::Value* location =
        value.FindKeyOfType("location",
                            base::Value::Type::STRING);
    const base::Value* order =
        value.FindKeyOfType("order",
                            base::Value::Type::INTEGER);

    if (!(location && order))
      continue;

    auto url = GURL(location->GetString());
    // Filter internal URLs from Muon that won't resolve correctly in brave-core
    if (!CanImportURL(url))
      continue;

    ImportedBrowserTab tab;
    tab.key = order->GetInt();
    tab.location = url;
    pinnedTabs.push_back(tab);
  }

  // Sort pinned tabs by key, which corresponds to a 0-indexed ordering from
  // left to right.
  std::sort(std::begin(pinnedTabs), std::end(pinnedTabs),
      [](auto a, auto b) { return a.key < b.key; });

  return pinnedTabs;
}

void BraveImporter::ImportWindows() {
  base::Optional<base::Value> session_store_json = ParseBraveStateFile(
      "session-store-1");
  if (!session_store_json)
    return;

  base::Value* perWindowState =
    session_store_json->FindKeyOfType("perWindowState",
                                      base::Value::Type::LIST);
  base::Value* pinnedSites =
    session_store_json->FindKeyOfType("pinnedSites",
                                      base::Value::Type::DICTIONARY);
  if (!(perWindowState && pinnedSites)) {
    LOG(ERROR) << "perWindowState and/or pinnedSites not found";
    return;
  }

  std::vector<ImportedBrowserWindow> windows = ParseWindows(perWindowState);

  // Pinned tabs are global in browser-laptop, while they are per-tab in
  // brave-core. To manage this transition, import all pinned tabs into the
  // first imported window only.
  std::vector<ImportedBrowserTab> pinnedTabs = ParsePinnedTabs(pinnedSites);

  if (!windows.empty() && !cancelled()) {
    ImportedWindowState windowState;
    windowState.windows = windows;
    windowState.pinnedTabs = pinnedTabs;
    bridge_->UpdateWindows(windowState);
  }
}

void BraveImporter::ImportSettings() {
  base::Optional<base::Value> session_store_json = ParseBraveStateFile(
      "session-store-1");
  if (!session_store_json) {
    return;
  }

  const base::Value* settings = session_store_json->FindKeyOfType(
      "settings",
      base::Value::Type::DICTIONARY);
  if (!settings) {
    LOG(ERROR) << "No entry \"settings\" found in session-store-1";
    return;
  }

  SessionStoreSettings user_settings;

  // Search related settings
  TryFindStringKey(settings, "search.default-search-engine",
      user_settings.default_search_engine);

  if (!TryFindBoolKey(settings, "search.use-alternate-private-search-engine",
      user_settings.use_alternate_private_search_engine)) {
    user_settings.use_alternate_private_search_engine = false;
  }

  if (!TryFindBoolKey(settings,
        "search.use-alternate-private-search-engine-tor",
      user_settings.use_alternate_private_search_engine_tor)) {
    user_settings.use_alternate_private_search_engine_tor = true;
  }

  bridge_->UpdateSettings(user_settings);
}
