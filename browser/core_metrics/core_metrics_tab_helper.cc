/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/core_metrics/core_metrics_tab_helper.h"

#include "brave/browser/core_metrics/core_metrics_service_factory.h"
#include "brave/components/core_metrics/core_metrics_service.h"
#include "content/public/browser/navigation_handle.h"

namespace core_metrics {

CoreMetricsTabHelper::CoreMetricsTabHelper(content::WebContents* web_contents)
    : WebContentsObserver(web_contents),
      content::WebContentsUserData<CoreMetricsTabHelper>(*web_contents) {
  core_metrics_service_ = CoreMetricsServiceFactory::GetServiceForContext(
      web_contents->GetBrowserContext());
  DCHECK(core_metrics_service_);
}

CoreMetricsTabHelper::~CoreMetricsTabHelper() = default;

void CoreMetricsTabHelper::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!core_metrics_service_ || !navigation_handle->IsInMainFrame() ||
      navigation_handle->IsSameDocument() ||
      !navigation_handle->HasCommitted() ||
      !navigation_handle->GetURL().SchemeIsHTTPOrHTTPS()) {
    return;
  }
  core_metrics_service_->IncrementPagesLoadedCount();
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(CoreMetricsTabHelper);

}  // namespace core_metrics
