/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/history/history_driver_ios.h"

#import <utility>

#import "base/check.h"
#import "base/strings/utf_string_conversions.h"
#import "components/browsing_data/core/history_notice_utils.h"
#import "ios/chrome/browser/history/model/history_utils.h"

using history::BrowsingHistoryService;

#pragma mark - HistoryDriverIOS

HistoryDriverIOS::HistoryDriverIOS(
    WebHistoryServiceGetter history_service_getter)
    : history_service_getter_(history_service_getter) {
  DCHECK(!history_service_getter_.is_null());
}

HistoryDriverIOS::~HistoryDriverIOS() = default;

#pragma mark - Private methods

void HistoryDriverIOS::OnQueryComplete(
    const std::vector<BrowsingHistoryService::HistoryEntry>& results,
    const BrowsingHistoryService::QueryResultsInfo& query_results_info,
    base::OnceClosure continuation_closure) {
  // Ignored.
}

void HistoryDriverIOS::OnRemoveVisitsComplete() {
  // Ignored.
}

void HistoryDriverIOS::OnRemoveVisitsFailed() {
  // Ignored.
}

void HistoryDriverIOS::OnRemoveVisits(
    const std::vector<history::ExpireHistoryArgs>& expire_list) {
  // Ignored.
}

void HistoryDriverIOS::HistoryDeleted() {
  // Ignored.
}

void HistoryDriverIOS::HasOtherFormsOfBrowsingHistory(bool has_other_forms,
                                                      bool has_synced_results) {
  // Ignored.
}

bool HistoryDriverIOS::AllowHistoryDeletions() {
  return true;
}

bool HistoryDriverIOS::ShouldHideWebHistoryUrl(const GURL& url) {
  return !ios::CanAddURLToHistory(url);
}

history::WebHistoryService* HistoryDriverIOS::GetWebHistoryService() {
  return history_service_getter_.Run();
}

void HistoryDriverIOS::ShouldShowNoticeAboutOtherFormsOfBrowsingHistory(
    const syncer::SyncService* sync_service,
    history::WebHistoryService* history_service,
    base::OnceCallback<void(bool)> callback) {
  browsing_data::ShouldShowNoticeAboutOtherFormsOfBrowsingHistory(
      sync_service, history_service, std::move(callback));
}
