/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ephemeral_storage/browsing_history_cleaner.h"

#include <algorithm>

#include "base/functional/bind.h"
#include "base/functional/callback.h"
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

  // Queue the query
  queries_.emplace_back(base::BindOnce(&BrowsingHistoryCleaner::CleanupQuery,
                                       weak_factory_.GetWeakPtr(), domain));
  if (queries_.size() > 1) {
    return;
  }

  std::move(queries_.back()).Run();
}

void BrowsingHistoryCleaner::CleanupQuery(const std::string& domain) {
  history::QueryOptions options;
  options.max_count = 0;
  options.host_only = true;
  options.duplicate_policy = history::QueryOptions::KEEP_ALL_DUPLICATES;
  browsing_history_service_->QueryHistory(base::UTF8ToUTF16(domain), options);
}

void BrowsingHistoryCleaner::OnRemoveRequestCompleted() {
  // Clean the queue from the completed queries
  queries_.erase(
      std::remove_if(queries_.begin(), queries_.end(),
                     [](const auto& query) { return query.is_null(); }),
      queries_.end());
  if (!queries_.empty()) {
    // Run the next query in the queue.
    std::move(queries_.back()).Run();
    return;
  }

  if (on_query_complete_callback_for_testing_) {
    std::move(on_query_complete_callback_for_testing_).Run();
  }
}

void BrowsingHistoryCleaner::OnQueryComplete(
    const HistoryEntryRequests& query_results,
    const history::BrowsingHistoryService::QueryResultsInfo& query_results_info,
    base::OnceClosure continuation_closure) {
  browsing_history_service_->RemoveVisits(query_results);
}

void BrowsingHistoryCleaner::OnRemoveVisitsComplete() {
  OnRemoveRequestCompleted();
}

void BrowsingHistoryCleaner::OnRemoveVisitsFailed() {
  OnRemoveRequestCompleted();
}

Profile* BrowsingHistoryCleaner::GetProfile() {
  return profile_;
}

void BrowsingHistoryCleaner::SetOnQueryCompleteCallbackForTesting(  // IN-TEST
    base::OnceClosure callback) {
  on_query_complete_callback_for_testing_ = std::move(callback);
}

}  // namespace ephemeral_storage
