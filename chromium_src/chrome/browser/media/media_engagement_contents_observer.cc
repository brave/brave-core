/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/media/media_engagement_contents_observer.h"

#include "brave/browser/request_otr/request_otr_service_factory.h"
#include "brave/components/request_otr/browser/request_otr_service.h"
#include "chrome/browser/media/media_engagement_service.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"

namespace {

bool BraveShouldRecordEngagement(content::NavigationHandle* navigation_handle) {
  if (content::WebContents* web_contents =
          navigation_handle->GetWebContents()) {
    if (request_otr::RequestOTRService* request_otr_service =
            request_otr::RequestOTRServiceFactory::GetForBrowserContext(
                web_contents->GetBrowserContext())) {
      if (request_otr_service->IsOTR(navigation_handle->GetURL())) {
        return false;
      }
    }
  }

  return true;
}

}  // namespace

#define ShouldRecordEngagement(origin) \
  ShouldRecordEngagement(origin) ||    \
      !BraveShouldRecordEngagement(navigation_handle)

#include "src/chrome/browser/media/media_engagement_contents_observer.cc"

#undef ShouldRecordEngagement
