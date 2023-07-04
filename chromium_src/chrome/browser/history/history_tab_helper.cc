/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/request_otr/common/buildflags/buildflags.h"
#include "build/build_config.h"

#if BUILDFLAG(ENABLE_REQUEST_OTR)

#include "chrome/browser/history/history_tab_helper.h"

// Include these here to avoid overriding "IsOffTheRecord" in them.
#include "brave/components/request_otr/browser/request_otr_storage_tab_helper.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service_factory.h"
#include "content/public/browser/browser_context.h"

#if BUILDFLAG(IS_ANDROID)
// Include these here to avoid overriding "IsOffTheRecord" in them.
#include "chrome/browser/ui/android/tab_model/tab_model.h"
#endif

namespace {

bool BraveTabRequestedOffTheRecord(content::WebContents* web_contents) {
  if (request_otr::RequestOTRStorageTabHelper* tab_storage =
          request_otr::RequestOTRStorageTabHelper::FromWebContents(
              web_contents)) {
    return tab_storage->has_requested_otr();
  }
  return false;
}

}  // namespace

#define IsOffTheRecord() \
  IsOffTheRecord() || BraveTabRequestedOffTheRecord(web_contents())

#endif

#include "src/chrome/browser/history/history_tab_helper.cc"

#if BUILDFLAG(ENABLE_REQUEST_OTR)
#undef IsOffTheRecord
#endif
