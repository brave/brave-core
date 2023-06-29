/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/request_otr/browser/request_otr_storage_tab_helper.h"

#define BRAVE_GET_OR_CREATE_SESSION                                   \
  if (content::WebContents* web_contents =                            \
          navigation_handle->GetWebContents()) {                      \
    if (request_otr::RequestOTRStorageTabHelper* tab_storage =        \
            request_otr::RequestOTRStorageTabHelper::FromWebContents( \
                web_contents)) {                                      \
      if (tab_storage->has_offered_otr()) {                           \
        return nullptr;                                               \
      }                                                               \
    }                                                                 \
  }

#include "src/chrome/browser/media/media_engagement_contents_observer.cc"

#undef BRAVE_GET_OR_CREATE_SESSION
