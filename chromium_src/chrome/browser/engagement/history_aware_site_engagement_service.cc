/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/engagement/history_aware_site_engagement_service.h"

#include "brave/browser/request_otr/request_otr_service_factory.h"
#include "brave/components/request_otr/browser/request_otr_service.h"
#include "content/public/browser/web_contents.h"

#include "src/chrome/browser/engagement/history_aware_site_engagement_service.cc"

namespace brave {

bool ShouldHandleSiteEngagementEvent(content::WebContents* web_contents) {
  if (request_otr::RequestOTRService* request_otr_service =
          request_otr::RequestOTRServiceFactory::GetForBrowserContext(
              web_contents->GetBrowserContext())) {
    if (request_otr_service->IsOTR(web_contents->GetLastCommittedURL())) {
      return false;
    }
  }
  return true;
}

}  // namespace brave

namespace site_engagement {

void HistoryAwareSiteEngagementService::HandleMediaPlaying(
    content::WebContents* web_contents,
    bool is_hidden) {
  if (!brave::ShouldHandleSiteEngagementEvent(web_contents)) {
    return;
  }
  SiteEngagementService::HandleMediaPlaying(web_contents, is_hidden);
}

void HistoryAwareSiteEngagementService::HandleNavigation(
    content::WebContents* web_contents,
    ui::PageTransition transition) {
  if (!brave::ShouldHandleSiteEngagementEvent(web_contents)) {
    return;
  }
  SiteEngagementService::HandleNavigation(web_contents, transition);
}

void HistoryAwareSiteEngagementService::HandleUserInput(
    content::WebContents* web_contents,
    EngagementType type) {
  if (!brave::ShouldHandleSiteEngagementEvent(web_contents)) {
    return;
  }
  SiteEngagementService::HandleUserInput(web_contents, type);
}

}  // namespace site_engagement
