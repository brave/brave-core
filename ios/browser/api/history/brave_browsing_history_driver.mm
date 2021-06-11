/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/history/brave_browsing_history_driver.h"
#include <utility>

#include "base/check.h"
#include "base/strings/utf_string_conversions.h"

#include "components/browsing_data/core/history_notice_utils.h"

#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/history/history_utils.h"
#include "ios/chrome/browser/history/web_history_service_factory.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using history::BrowsingHistoryService;

#pragma mark - BraveBrowsingHistoryDriver

BraveBrowsingHistoryDriver::BraveBrowsingHistoryDriver(
    ChromeBrowserState* browser_state,
    id<BraveHistoryDriverDelegate> delegate)
    : browser_state_(browser_state), delegate_(delegate) {
  DCHECK(browser_state_);
}

BraveBrowsingHistoryDriver::~BraveBrowsingHistoryDriver() {
  delegate_ = nil;
}

#pragma mark - Private methods

void BraveBrowsingHistoryDriver::OnQueryComplete(
    const std::vector<BrowsingHistoryService::HistoryEntry>& results,
    const BrowsingHistoryService::QueryResultsInfo& query_results_info,
    base::OnceClosure continuation_closure) {
  [delegate_
      historyQueryWasCompletedWithResults:results
                         queryResultsInfo:query_results_info
                      continuationClosure:std::move(continuation_closure)];
}

void BraveBrowsingHistoryDriver::OnRemoveVisitsComplete() {
  // Ignored.
}

void BraveBrowsingHistoryDriver::OnRemoveVisitsFailed() {
  // Ignored.
}

void BraveBrowsingHistoryDriver::OnRemoveVisits(
    const std::vector<history::ExpireHistoryArgs>& expire_list) {
  // Ignored.
}

void BraveBrowsingHistoryDriver::HistoryDeleted() {
  [delegate_ historyWasDeleted];
}

void BraveBrowsingHistoryDriver::HasOtherFormsOfBrowsingHistory(
    bool has_other_forms,
    bool has_synced_results) {
  // Ignored.
}

bool BraveBrowsingHistoryDriver::AllowHistoryDeletions() {
  return true;
}

bool BraveBrowsingHistoryDriver::ShouldHideWebHistoryUrl(const GURL& url) {
  return !ios::CanAddURLToHistory(url);
}

history::WebHistoryService* BraveBrowsingHistoryDriver::GetWebHistoryService() {
  return ios::WebHistoryServiceFactory::GetForBrowserState(browser_state_);
}

void BraveBrowsingHistoryDriver::ShouldShowNoticeAboutOtherFormsOfBrowsingHistory(
    const syncer::SyncService* sync_service,
    history::WebHistoryService* history_service,
    base::OnceCallback<void(bool)> callback) {
  browsing_data::ShouldShowNoticeAboutOtherFormsOfBrowsingHistory(
      sync_service, history_service, std::move(callback));
}
