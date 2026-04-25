/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_MISC_METRICS_MISC_ANDROID_METRICS_H_
#define BRAVE_BROWSER_MISC_METRICS_MISC_ANDROID_METRICS_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/time/time.h"
#include "brave/components/misc_metrics/common/misc_metrics.mojom.h"
#include "mojo/public/cpp/bindings/receiver_set.h"

class PrefService;
class SearchEngineTracker;
class TemplateURLService;

namespace misc_metrics {

class BraveSearchMetrics;
class NavigationSourceMetrics;
class ProcessMiscMetrics;
class QuickSearchMetrics;

class MiscAndroidMetrics : public mojom::MiscAndroidMetrics {
 public:
  MiscAndroidMetrics(PrefService* local_state,
                     ProcessMiscMetrics* misc_metrics,
                     SearchEngineTracker* search_engine_tracker,
                     TemplateURLService* template_url_service,
                     BraveSearchMetrics* brave_search_metrics,
                     NavigationSourceMetrics* navigation_source_metrics);
  ~MiscAndroidMetrics() override;

  MiscAndroidMetrics(const MiscAndroidMetrics&) = delete;
  MiscAndroidMetrics& operator=(const MiscAndroidMetrics&) = delete;

  mojo::PendingRemote<mojom::MiscAndroidMetrics> MakeRemote();

  // mojom::MiscAndroidMetrics:
  void RecordPrivacyHubView() override;
  void RecordPrivacyHubEnabledStatus(bool is_enabled) override;
  void RecordBrowserUsageDuration(base::TimeDelta duration) override;
  void RecordLocationBarChange(bool is_new_tab, bool is_search_query) override;
  void RecordAppMenuNewTab() override;
  void RecordTabSwitcherNewTab() override;
  void RecordSetAsDefault(bool is_default) override;
  void RecordQuickSearch(bool is_leo, const std::string& keyword) override;
  void RecordIntentURL(const std::string& url) override;
  void RecordOmniboxSearchQuery(const std::string& destination_url,
                                bool is_suggestion) override;
  void RecordOmniboxDirectNavigation() override;
  void RecordOmniboxHistoryNavigation() override;
  void RecordOmniboxBookmarkNavigation() override;
  void RecordTopSiteNavigation(bool is_custom) override;

 private:
  raw_ptr<ProcessMiscMetrics> misc_metrics_;
  raw_ptr<SearchEngineTracker> search_engine_tracker_;
  raw_ptr<BraveSearchMetrics> brave_search_metrics_;
  raw_ptr<NavigationSourceMetrics> navigation_source_metrics_;

  std::unique_ptr<QuickSearchMetrics> quick_search_metrics_;
  mojo::ReceiverSet<mojom::MiscAndroidMetrics> receivers_;
};

}  // namespace misc_metrics

#endif  // BRAVE_BROWSER_MISC_METRICS_MISC_ANDROID_METRICS_H_
