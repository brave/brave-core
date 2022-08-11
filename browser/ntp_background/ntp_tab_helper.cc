/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ntp_background/ntp_tab_helper.h"

#include "brave/browser/ntp_background/view_counter_service_factory.h"
#include "brave/components/ntp_background_images/browser/view_counter_service.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/navigation_handle.h"

namespace ntp_background_images {

NTPTabHelper::NTPTabHelper(content::WebContents* web_contents)
    : WebContentsObserver(web_contents),
      content::WebContentsUserData<NTPTabHelper>(*web_contents) {
  Profile* profile =
      Profile::FromBrowserContext(web_contents->GetBrowserContext());
  if (profile) {
    view_counter_service_ = ViewCounterServiceFactory::GetForProfile(profile);
  }
}

NTPTabHelper::~NTPTabHelper() = default;

void NTPTabHelper::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInMainFrame() ||
      !navigation_handle->HasCommitted()) {
    return;
  }
  if (view_counter_service_) {
    view_counter_service_->OnTabURLChanged(navigation_handle->GetURL());
  }
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(NTPTabHelper);

}  // namespace ntp_background_images
