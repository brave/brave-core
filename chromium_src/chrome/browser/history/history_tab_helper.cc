/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/request_otr/common/buildflags/buildflags.h"
#include "build/build_config.h"

#if BUILDFLAG(ENABLE_REQUEST_OTR)

#include "chrome/browser/history/history_tab_helper.h"

#include "brave/components/request_otr/browser/request_otr_tab_storage.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service_factory.h"
#include "content/public/browser/browser_context.h"

namespace {

bool BraveTabRequestedOffTheRecord(content::WebContents* web_contents) {
  if (request_otr::RequestOTRTabStorage* tab_storage =
          request_otr::RequestOTRTabStorage::FromWebContents(web_contents)) {
    return tab_storage->RequestedOTR();
  }
  return false;
}

bool BraveOTRDummyFunctionAlwaysFalse() {
  return false;
}

}  // namespace

#define IsOffTheRecord                                                 \
  IsOffTheRecord() || BraveTabRequestedOffTheRecord(web_contents()) || \
      BraveOTRDummyFunctionAlwaysFalse

#endif

#include "src/chrome/browser/history/history_tab_helper.cc"

#if BUILDFLAG(ENABLE_REQUEST_OTR)
#undef IsOffTheRecord
#endif
