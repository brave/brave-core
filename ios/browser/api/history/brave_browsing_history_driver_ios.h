/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_HISTORY_BRAVE_BROWSING_HISTORY_DRIVER_IOS_H_
#define BRAVE_IOS_BROWSER_API_HISTORY_BRAVE_BROWSING_HISTORY_DRIVER_IOS_H_

#import <Foundation/Foundation.h>
#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"

#include "components/history/core/browser/browsing_history_driver.h"
#include "components/history/core/browser/browsing_history_service.h"

#include "url/gurl.h"

class ChromeBrowserState;

namespace history {
class HistoryService;
}

NS_ASSUME_NONNULL_BEGIN

@protocol BraveHistoryDriverDelegate

/// Tells the consumer that the result of a history query has been retrieved
/// @param results - Results of the query (Sorted)
/// @param queryResultsInfo - Detailed Query Results
/// @param continuationClosure - Pagination lambda
/// If called, will continue fetching results where last left off.
- (void)
    historyQueryWasCompletedWithResults:
        (const std::vector<history::BrowsingHistoryService::HistoryEntry>&)
            results
                       queryResultsInfo:(const history::BrowsingHistoryService::
                                             QueryResultsInfo&)queryResultsInfo
                    continuationClosure:(base::OnceClosure)continuationClosure;

/// Tells the consumer that history entries
/// have been deleted from a different client
- (void)historyWasDeleted;

@end

class BraveBrowsingHistoryDriverIOS : public history::BrowsingHistoryDriver {
 public:
  BraveBrowsingHistoryDriverIOS(ChromeBrowserState* browser_state,
                                id<BraveHistoryDriverDelegate> delegate);
  ~BraveBrowsingHistoryDriverIOS() override;

 private:
  // history::BrowsingHistoryDriver Implementation.
  void OnQueryComplete(
      const std::vector<history::BrowsingHistoryService::HistoryEntry>& results,
      const history::BrowsingHistoryService::QueryResultsInfo&
          query_results_info,
      base::OnceClosure continuation_closure) override;
  void OnRemoveVisitsComplete() override;
  void OnRemoveVisitsFailed() override;
  void OnRemoveVisits(
      const std::vector<history::ExpireHistoryArgs>& expire_list) override;
  void HistoryDeleted() override;
  void HasOtherFormsOfBrowsingHistory(bool has_other_forms,
                                      bool has_synced_results) override;
  bool AllowHistoryDeletions() override;
  bool ShouldHideWebHistoryUrl(const GURL& url) override;
  history::WebHistoryService* GetWebHistoryService() override;
  void ShouldShowNoticeAboutOtherFormsOfBrowsingHistory(
      const syncer::SyncService* sync_service,
      history::WebHistoryService* history_service,
      base::OnceCallback<void(bool)> callback) override;

  // The current browser state.
  ChromeBrowserState* browser_state_;
  __weak id<BraveHistoryDriverDelegate> delegate_;
};

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_HISTORY_BRAVE_BROWSING_HISTORY_DRIVER_IOS_H_
