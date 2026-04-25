/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_HISTORY_HISTORY_DRIVER_IOS_H_
#define BRAVE_IOS_BROWSER_API_HISTORY_HISTORY_DRIVER_IOS_H_

#include <vector>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "components/history/core/browser/browsing_history_driver.h"
#include "components/history/core/browser/browsing_history_service.h"
#include "url/gurl.h"

class HistoryDriverIOS : public history::BrowsingHistoryDriver {
 public:
  using WebHistoryServiceGetter =
      base::RepeatingCallback<history::WebHistoryService*()>;

  explicit HistoryDriverIOS(WebHistoryServiceGetter history_service_getter);

  HistoryDriverIOS(const HistoryDriverIOS&) = delete;
  HistoryDriverIOS& operator=(const HistoryDriverIOS&) = delete;

  ~HistoryDriverIOS() override;

 private:
  // history::BrowsingHistoryDriver implementation.
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

  // The current web history service.
  WebHistoryServiceGetter history_service_getter_;
};

#endif  // BRAVE_IOS_BROWSER_API_HISTORY_HISTORY_DRIVER_IOS_H_
