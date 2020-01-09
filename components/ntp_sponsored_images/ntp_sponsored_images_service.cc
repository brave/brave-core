/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_sponsored_images/ntp_sponsored_images_service.h"

NTPSponsoredImagesService::NTPSponsoredImagesService(
    content::BrowserContext* browser_context,
    NTPSponsoredImagesComponentManager* manager)
    : browser_context_(browser_context),
      manager_(manager) {
  manager_->AddDataSources(browser_context_);
  manager_->AddObserver(this);
}

NTPSponsoredImagesService::~NTPSponsoredImagesService() {
  // |manager_| owned by browser process that outlives this service instance.
  manager_->RemoveObserver(this);
}

void NTPSponsoredImagesService::OnUpdated(const NTPSponsoredImagesData& data) {
  manager_->AddDataSources(browser_context_);
}
