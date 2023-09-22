/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "build/build_config.h"

#include "brave/browser/request_otr/request_otr_service_factory.h"
#include "brave/components/request_otr/browser/request_otr_service.h"
#include "chrome/browser/history/history_tab_helper.h"

// Include these here to avoid overriding "IsOffTheRecord" in them.
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
  if (request_otr::RequestOTRService* request_otr_service =
          request_otr::RequestOTRServiceFactory::GetForBrowserContext(
              web_contents->GetBrowserContext())) {
    return request_otr_service->IsOTR(web_contents->GetLastCommittedURL());
  }
  return false;
}

}  // namespace

#define IsOffTheRecord() \
  IsOffTheRecord() || BraveTabRequestedOffTheRecord(web_contents())

#include "src/chrome/browser/history/history_tab_helper.cc"

#undef IsOffTheRecord
