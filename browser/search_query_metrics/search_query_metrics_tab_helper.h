/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_SEARCH_QUERY_METRICS_SEARCH_QUERY_METRICS_TAB_HELPER_H_
#define BRAVE_BROWSER_SEARCH_QUERY_METRICS_SEARCH_QUERY_METRICS_TAB_HELPER_H_

#include <optional>

#include "base/memory/raw_ptr.h"
#include "brave/components/search_query_metrics/search_query_metrics_entry_point_type.h"
#include "build/build_config.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

class GURL;

namespace metrics {

class SearchQueryMetricsService;

// Tracks search query metrics for a browser tab, recording the entry point (not
// the query text) by observing navigation and search events.

class SearchQueryMetricsTabHelper final
    : public content::WebContentsObserver,
      public content::WebContentsUserData<SearchQueryMetricsTabHelper> {
 public:
  explicit SearchQueryMetricsTabHelper(content::WebContents*);

  SearchQueryMetricsTabHelper(const SearchQueryMetricsTabHelper&) = delete;
  SearchQueryMetricsTabHelper& operator=(const SearchQueryMetricsTabHelper&) =
      delete;

  ~SearchQueryMetricsTabHelper() override;

  SearchQueryMetricsService* search_query_metrics_service() {
    return search_query_metrics_service_;
  }

  void MarkEntryPointAsDirect() {
    DVLOG(1) << "[METRIC] Marking entry point as direct";
    entry_point_type_ = SearchQueryMetricsEntryPointType::kDirect;
  }

  void MarkEntryPointAsNewTabPage() {
    DVLOG(1) << "[METRIC] Marking entry point as NTP";
    entry_point_type_ = SearchQueryMetricsEntryPointType::kNTP;
  }

  void MarkEntryPointAsOmniboxHistory() {
    DVLOG(1) << "[METRIC] Marking entry point as omnibox history";
    entry_point_type_ = SearchQueryMetricsEntryPointType::kOmniboxHistory;
  }

  void MarkEntryPointAsOmniboxSuggestion() {
    DVLOG(1) << "[METRIC] Marking entry point as omnibox suggestion";
    entry_point_type_ = SearchQueryMetricsEntryPointType::kOmniboxSuggestion;
  }

  void MarkEntryPointAsOmniboxSearch() {
    DVLOG(1) << "[METRIC] Marking entry point as omnibox search";
    entry_point_type_ = SearchQueryMetricsEntryPointType::kOmniboxSearch;
  }

  void MarkEntryPointAsQuickSearch() {
    DVLOG(1) << "[METRIC] Marking entry point as quick search";
    entry_point_type_ = SearchQueryMetricsEntryPointType::kQuickSearch;
  }

  void MarkEntryPointAsShortcut() {
    DVLOG(1) << "[METRIC] Marking entry point as shortcut";
    entry_point_type_ = SearchQueryMetricsEntryPointType::kShortcut;
  }

  void MarkEntryPointAsTopSite() {
    DVLOG(1) << "[METRIC] Marking entry point as top site";
    entry_point_type_ = SearchQueryMetricsEntryPointType::kTopSite;
  }

 private:
  friend class content::WebContentsUserData<SearchQueryMetricsTabHelper>;

  void MaybeClassifyAndReport(content::NavigationHandle* navigation_handle);

  // content::WebContentsObserver:
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;

  // Determines how a search engine results page was reached, using navigation
  // context and user action to attribute search queries to their entry points.
  std::optional<SearchQueryMetricsEntryPointType> MaybeClassifyEntryPoint(
      const GURL& url,
      ui::PageTransition page_transition,
      bool is_bookmark);

  raw_ptr<SearchQueryMetricsService> search_query_metrics_service_ =
      nullptr;  // Not owned.

  bool is_initial_navigation_ = true;
  std::optional<SearchQueryMetricsEntryPointType> entry_point_type_;
  bool defer_reporting_until_next_search_query_ = false;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace metrics

#endif  // BRAVE_BROWSER_SEARCH_QUERY_METRICS_SEARCH_QUERY_METRICS_TAB_HELPER_H_
