/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_CORE_METRICS_CORE_METRICS_TAB_HELPER_H_
#define BRAVE_BROWSER_CORE_METRICS_CORE_METRICS_TAB_HELPER_H_

#include "base/memory/raw_ptr.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

namespace content {
class NavigationHandle;
}  // namespace content

namespace core_metrics {

class CoreMetricsService;

class CoreMetricsTabHelper
    : public content::WebContentsObserver,
      public content::WebContentsUserData<CoreMetricsTabHelper> {
 public:
  explicit CoreMetricsTabHelper(content::WebContents* web_contents);
  ~CoreMetricsTabHelper() override;

  CoreMetricsTabHelper(const CoreMetricsTabHelper&) = delete;
  CoreMetricsTabHelper& operator=(const CoreMetricsTabHelper&) = delete;

 private:
  friend class content::WebContentsUserData<CoreMetricsTabHelper>;

  // content::WebContentsObserver:
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;

  raw_ptr<CoreMetricsService> core_metrics_service_;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace core_metrics

#endif  // BRAVE_BROWSER_CORE_METRICS_CORE_METRICS_TAB_HELPER_H_
