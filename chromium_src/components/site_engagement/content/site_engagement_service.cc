/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/request_otr/browser/request_otr_storage_tab_helper.h"

#define BRAVE_EARLY_RETURN_IF_OTR                                       \
    if (request_otr::RequestOTRStorageTabHelper* tab_storage =          \
        request_otr::RequestOTRStorageTabHelper::FromWebContents(       \
            web_contents)) {                                            \
        if (tab_storage->has_offered_otr()) {                           \
            return;                                                     \
        }                                                               \
    }

#define BRAVE_HANDLE_USER_INPUT BRAVE_EARLY_RETURN_IF_OTR
#define BRAVE_HANDLE_NAVIGATION BRAVE_EARLY_RETURN_IF_OTR
#define BRAVE_HANDLE_MEDIA_PLAYING BRAVE_EARLY_RETURN_IF_OTR

#include "src/components/site_engagement/content/site_engagement_service.cc"

#undef BRAVE_HANDLE_MEDIA_PLAYING
#undef BRAVE_HANDLE_NAVIGATION
#undef BRAVE_HANDLE_USER_INPUT
#undef BRAVE_EARLY_RETURN_IF_OTR
