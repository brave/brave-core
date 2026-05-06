/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ephemeral_storage/browsing_history_cleaner.h"

#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sync/sync_service_factory.h"

namespace ephemeral_storage {

BrowsingHistoryCleaner::BrowsingHistoryCleaner(
    Profile* profile,
    history::HistoryService* history_service,
    syncer::SyncService* sync_service)
    : profile_(profile) {
  CHECK(history_service);
  CHECK(sync_service);
  browsing_history_service_ = std::make_unique<history::BrowsingHistoryService>(
      this, history_service, sync_service);
}

BrowsingHistoryCleaner::~BrowsingHistoryCleaner() = default;

void BrowsingHistoryCleaner::CleanupBrowsingHistoryForDomain(
    const std::string& domain) {
  if (domain.empty()) {
    return;
  }

  history::QueryOptions options;
  options.max_count = 0;
  options.host_only = false;
  options.duplicate_policy = history::QueryOptions::KEEP_ALL_DUPLICATES;
  browsing_history_service_->QueryHistory(base::UTF8ToUTF16(domain), options);
}

void BrowsingHistoryCleaner::OnQueryComplete(
    const std::vector<history::BrowsingHistoryService::HistoryEntry>&
        query_results,
    const history::BrowsingHistoryService::QueryResultsInfo& query_results_info,
    base::OnceClosure continuation_closure) {
  browsing_history_service_->RemoveVisits(query_results);
}

Profile* BrowsingHistoryCleaner::GetProfile() {
  return profile_;
}

}  // namespace ephemeral_storage
