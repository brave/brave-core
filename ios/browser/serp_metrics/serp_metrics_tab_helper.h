/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_SERP_METRICS_SERP_METRICS_TAB_HELPER_H_
#define BRAVE_IOS_BROWSER_SERP_METRICS_SERP_METRICS_TAB_HELPER_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "base/memory/raw_ref.h"
#include "ios/web/public/web_state_observer.h"
#include "ios/web/public/web_state_user_data.h"

namespace web {
class NavigationContext;
class WebState;
}  // namespace web

namespace serp_metrics {

class SerpMetrics;
class SerpMetricsSharedTabHelper;

// Observes navigations and records search engine result page visits.
class SerpMetricsTabHelper : public web::WebStateUserData<SerpMetricsTabHelper>,
                             public web::WebStateObserver {
 public:
  SerpMetricsTabHelper(const SerpMetricsTabHelper&) = delete;
  SerpMetricsTabHelper& operator=(const SerpMetricsTabHelper&) = delete;

  ~SerpMetricsTabHelper() override;

  static void MaybeCreateForWebState(web::WebState* web_state);

 private:
  friend class web::WebStateUserData<SerpMetricsTabHelper>;

  SerpMetricsTabHelper(web::WebState* web_state, SerpMetrics& serp_metrics);

  // web::WebStateObserver:
  void DidFinishNavigation(web::WebState* web_state,
                           web::NavigationContext* navigation_context) override;
  void WebStateDestroyed(web::WebState* web_state) override;

  raw_ptr<web::WebState> web_state_;
  const raw_ref<SerpMetrics> serp_metrics_;

  std::unique_ptr<SerpMetricsSharedTabHelper> shared_tab_helper_;
};

}  // namespace serp_metrics

#endif  // BRAVE_IOS_BROWSER_SERP_METRICS_SERP_METRICS_TAB_HELPER_H_
