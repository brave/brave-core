/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/media/media_engagement_contents_observer.h"
#include "brave/components/request_otr/browser/request_otr_storage_tab_helper.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"

#define GetOrCreateSession GetOrCreateSession_ChromiumImpl

#include "src/chrome/browser/media/media_engagement_contents_observer.cc"

#undef GetOrCreateSession

scoped_refptr<MediaEngagementSession>
MediaEngagementContentsObserver::GetOrCreateSession(
    content::NavigationHandle* navigation_handle,
    content::WebContents* opener) const {
  if (content::WebContents* web_contents =
          navigation_handle->GetWebContents()) {
    if (request_otr::RequestOTRStorageTabHelper* tab_storage =
            request_otr::RequestOTRStorageTabHelper::FromWebContents(
                web_contents)) {
      if (tab_storage->has_offered_otr()) {
        return nullptr;
      }
    }
  }

  return GetOrCreateSession_ChromiumImpl(navigation_handle, opener);
}
