/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_MISC_METRICS_PAGE_METRICS_TAB_HELPER_H_
#define BRAVE_BROWSER_MISC_METRICS_PAGE_METRICS_TAB_HELPER_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

namespace content {
class NavigationHandle;
}  // namespace content

namespace misc_metrics {

class PageMetrics;

class PageMetricsTabHelper
    : public content::WebContentsObserver,
      public content::WebContentsUserData<PageMetricsTabHelper> {
 public:
  explicit PageMetricsTabHelper(content::WebContents* web_contents);
  ~PageMetricsTabHelper() override;

  PageMetricsTabHelper(const PageMetricsTabHelper&) = delete;
  PageMetricsTabHelper& operator=(const PageMetricsTabHelper&) = delete;

 private:
  friend class content::WebContentsUserData<PageMetricsTabHelper>;

  // content::WebContentsObserver:
  void DidStartNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DidRedirectNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;

  bool CheckNavigationEvent(content::NavigationHandle* navigation_handle,
                            bool is_finished);
  bool IsHttpAllowedForHost(content::NavigationHandle* navigation_handle);

  bool was_http_allowlist_ = false;
  std::string last_started_host_;
  raw_ptr<PageMetrics> page_metrics_;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace misc_metrics

#endif  // BRAVE_BROWSER_MISC_METRICS_PAGE_METRICS_TAB_HELPER_H_
