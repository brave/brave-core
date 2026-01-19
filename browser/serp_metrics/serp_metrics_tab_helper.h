/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_SERP_METRICS_SERP_METRICS_TAB_HELPER_H_
#define BRAVE_BROWSER_SERP_METRICS_SERP_METRICS_TAB_HELPER_H_

#include "brave/components/serp_metrics/serp_metrics.h"
#include "components/search_engines/search_engine_type.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

namespace metrics {

class SerpMetrics;

class SerpMetricsTabHelper final
    : public content::WebContentsObserver,
      public content::WebContentsUserData<SerpMetricsTabHelper> {
 public:
  explicit SerpMetricsTabHelper(content::WebContents*);

  SerpMetricsTabHelper(const SerpMetricsTabHelper&) = delete;
  SerpMetricsTabHelper& operator=(const SerpMetricsTabHelper&) = delete;

  ~SerpMetricsTabHelper() override;

 private:
  friend class content::WebContentsUserData<SerpMetricsTabHelper>;

  void Record(SearchEngineType search_engine_type);

  // content::WebContentsObserver:
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;

  raw_ptr<SerpMetrics> serp_metrics_ = nullptr;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace metrics

#endif  // BRAVE_BROWSER_SERP_METRICS_SERP_METRICS_TAB_HELPER_H_
