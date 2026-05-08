/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EPHEMERAL_STORAGE_BROWSING_HISTORY_CLEANER_H_
#define BRAVE_BROWSER_EPHEMERAL_STORAGE_BROWSING_HISTORY_CLEANER_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/history/profile_based_browsing_history_driver.h"
#include "components/history/core/browser/browsing_history_service.h"
#include "components/sync/service/sync_service.h"

namespace ephemeral_storage {

class BrowsingHistoryCleaner : public ProfileBasedBrowsingHistoryDriver {
 public:
  explicit BrowsingHistoryCleaner(Profile* profile,
                                  history::HistoryService* history_service,
                                  syncer::SyncService* sync_service);
  ~BrowsingHistoryCleaner() override;
  using WebHistoryServiceGetter =
      base::RepeatingCallback<history::WebHistoryService*()>;
  using HistoryEntryRequests =
      std::vector<history::BrowsingHistoryService::HistoryEntry>;

  void CleanupBrowsingHistoryForDomain(const std::string& search_text);

 private:
  friend class BrowsingHistoryCleanerTest;

  void CleanupQuery(const std::string& search_text);
  void OnRemoveRequestCompleted();

  // BrowsingHistoryDriver implementation.
  void OnQueryComplete(const HistoryEntryRequests& query_results,
                       const history::BrowsingHistoryService::QueryResultsInfo&
                           query_results_info,
                       base::OnceClosure continuation_closure) override;
  void OnRemoveVisitsComplete() override;
  void OnRemoveVisitsFailed() override;
  Profile* GetProfile() override;

  // For testing: allows setting a callback that will be run when
  // OnQueryComplete is called.
  void SetOnQueryCompleteCallbackForTesting(base::OnceClosure callback);

  std::string search_text_;
  std::vector<base::OnceCallback<void()>> queries_;
  std::unique_ptr<history::BrowsingHistoryService> browsing_history_service_;
  raw_ptr<Profile> profile_ = nullptr;
  base::OnceClosure on_query_complete_callback_for_testing_;
  base::WeakPtrFactory<BrowsingHistoryCleaner> weak_factory_{this};
};

}  // namespace ephemeral_storage

#endif  // BRAVE_BROWSER_EPHEMERAL_STORAGE_BROWSING_HISTORY_CLEANER_H_
