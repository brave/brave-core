/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/misc_metrics/page_metrics_tab_helper.h"

#include "brave/browser/misc_metrics/page_metrics_service_factory.h"
#include "brave/components/misc_metrics/page_metrics_service.h"
#include "content/public/browser/navigation_handle.h"

namespace misc_metrics {

PageMetricsTabHelper::PageMetricsTabHelper(content::WebContents* web_contents)
    : WebContentsObserver(web_contents),
      content::WebContentsUserData<PageMetricsTabHelper>(*web_contents) {
  page_metrics_service_ = PageMetricsServiceFactory::GetServiceForContext(
      web_contents->GetBrowserContext());
  DCHECK(page_metrics_service_);
}

PageMetricsTabHelper::~PageMetricsTabHelper() = default;

void PageMetricsTabHelper::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!page_metrics_service_ || !navigation_handle->IsInMainFrame() ||
      navigation_handle->IsSameDocument() ||
      !navigation_handle->HasCommitted() ||
      !navigation_handle->GetURL().SchemeIsHTTPOrHTTPS()) {
    return;
  }
  page_metrics_service_->IncrementPagesLoadedCount();
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(PageMetricsTabHelper);

}  // namespace misc_metrics
