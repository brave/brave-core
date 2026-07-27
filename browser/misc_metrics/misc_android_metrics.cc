/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/misc_metrics/misc_android_metrics.h"

#include "base/metrics/histogram_macros.h"
#include "brave/browser/misc_metrics/process_misc_metrics.h"
#include "brave/browser/misc_metrics/uptime_monitor_impl.h"
#include "brave/browser/search_engines/search_engine_tracker.h"
#include "brave/components/misc_metrics/default_browser_monitor.h"
#include "brave/components/misc_metrics/page_metrics.h"
#include "brave/components/misc_metrics/privacy_hub_metrics.h"
#include "brave/components/misc_metrics/quick_search_metrics.h"
#include "brave/components/misc_metrics/tab_metrics.h"
#include "url/gurl.h"

namespace {

constexpr char kOpeningScreenSwitchHistogramName[] =
    "Brave.NTP.OpeningScreenSwitch";

}  // namespace

namespace misc_metrics {

MiscAndroidMetrics::MiscAndroidMetrics(
    PrefService* local_state,
    ProcessMiscMetrics* misc_metrics,
    SearchEngineTracker* search_engine_tracker,
    TemplateURLService* template_url_service,
    PageMetrics* page_metrics)
    : misc_metrics_(misc_metrics),
      search_engine_tracker_(search_engine_tracker),
      page_metrics_(page_metrics),
      quick_search_metrics_(
          std::make_unique<QuickSearchMetrics>(local_state,
                                               template_url_service)) {}

MiscAndroidMetrics::~MiscAndroidMetrics() = default;

mojo::PendingRemote<mojom::MiscAndroidMetrics>
MiscAndroidMetrics::MakeRemote() {
  mojo::PendingRemote<mojom::MiscAndroidMetrics> remote;
  receivers_.Add(this, remote.InitWithNewPipeAndPassReceiver());
  return remote;
}

void MiscAndroidMetrics::RecordPrivacyHubView() {
  misc_metrics_->privacy_hub_metrics()->RecordView();
}

void MiscAndroidMetrics::RecordPrivacyHubEnabledStatus(bool is_enabled) {
  misc_metrics_->privacy_hub_metrics()->RecordEnabledStatus(is_enabled);
}

void MiscAndroidMetrics::RecordLocationBarChange(bool is_new_tab,
                                                 bool is_search_query) {
  if (is_search_query) {
    search_engine_tracker_->RecordLocationBarQuery();
  }
  misc_metrics_->tab_metrics()->RecordLocationBarChange(is_new_tab);
}

void MiscAndroidMetrics::RecordAppMenuNewTab() {
  misc_metrics_->tab_metrics()->RecordAppMenuNewTab();
}

void MiscAndroidMetrics::RecordTabSwitcherNewTab() {
  misc_metrics_->tab_metrics()->RecordTabSwitcherNewTab();
}

void MiscAndroidMetrics::RecordBrowserUsageDuration(base::TimeDelta duration) {
  misc_metrics_->uptime_monitor()->ReportUsageDuration(duration);
}

void MiscAndroidMetrics::RecordSetAsDefault(bool is_default) {
  misc_metrics_->default_browser_monitor()->OnDefaultBrowserStateReceived(
      is_default);
}

void MiscAndroidMetrics::RecordQuickSearch(bool is_leo,
                                           const std::string& keyword) {
  quick_search_metrics_->RecordQuickSearch(is_leo, keyword);
  if (page_metrics_) {
    page_metrics_->brave_search_metrics().MaybeRecordQuickSearch(is_leo,
                                                                 keyword);
  }
}

void MiscAndroidMetrics::RecordIntentURL(const std::string& url) {
  if (!page_metrics_) {
    return;
  }
  page_metrics_->brave_search_metrics().MaybeRecordWidgetSearch(GURL(url));
}

void MiscAndroidMetrics::RecordOmniboxSearchQuery(
    const std::string& destination_url,
    bool is_suggestion) {
  if (!page_metrics_) {
    return;
  }
  page_metrics_->brave_search_metrics().MaybeRecordOmniboxQuery(
      GURL(destination_url), is_suggestion);
  page_metrics_->RecordOmniboxQuery();
}

void MiscAndroidMetrics::RecordOmniboxDirectNavigation() {
  if (!page_metrics_) {
    return;
  }
  page_metrics_->navigation_source_metrics().RecordDirectNavigation();
}

void MiscAndroidMetrics::RecordOmniboxHistoryNavigation() {
  if (!page_metrics_) {
    return;
  }
  page_metrics_->navigation_source_metrics().RecordHistoryNavigation();
}

void MiscAndroidMetrics::RecordOmniboxBookmarkNavigation() {
  if (!page_metrics_) {
    return;
  }
  page_metrics_->navigation_source_metrics().RecordBookmarkNavigation();
}

void MiscAndroidMetrics::RecordTopSiteNavigation(bool is_custom) {
  if (!page_metrics_) {
    return;
  }
  page_metrics_->navigation_source_metrics().RecordTopSiteNavigation(is_custom);
}

void MiscAndroidMetrics::RecordOpeningScreenSettingSwitch(
    mojom::OpeningScreenSwitchType switch_type) {
  UMA_HISTOGRAM_ENUMERATION(kOpeningScreenSwitchHistogramName, switch_type);
}

}  // namespace misc_metrics
