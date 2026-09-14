/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_SERP_METRICS_SERP_METRICS_TAB_HELPER_H_
#define BRAVE_BROWSER_SERP_METRICS_SERP_METRICS_TAB_HELPER_H_

#include <memory>

#include "base/memory/raw_ref.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

namespace content {
class WebContents;
}  // namespace content

namespace serp_metrics {

class SerpMetrics;
class SerpMetricsSharedTabHelper;

class SerpMetricsTabHelper final
    : public content::WebContentsObserver,
      public content::WebContentsUserData<SerpMetricsTabHelper> {
 public:
  SerpMetricsTabHelper(const SerpMetricsTabHelper&) = delete;
  SerpMetricsTabHelper& operator=(const SerpMetricsTabHelper&) = delete;

  ~SerpMetricsTabHelper() override;

  static void MaybeCreateForWebContents(content::WebContents* web_contents);

 private:
  friend class content::WebContentsUserData<SerpMetricsTabHelper>;

  SerpMetricsTabHelper(content::WebContents*, SerpMetrics& serp_metrics);

  // content::WebContentsObserver:
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;

  const raw_ref<SerpMetrics> serp_metrics_;

  std::unique_ptr<SerpMetricsSharedTabHelper> shared_tab_helper_;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace serp_metrics

#endif  // BRAVE_BROWSER_SERP_METRICS_SERP_METRICS_TAB_HELPER_H_
