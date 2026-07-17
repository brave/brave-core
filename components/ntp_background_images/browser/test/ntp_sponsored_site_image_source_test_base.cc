/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/test/ntp_sponsored_site_image_source_test_base.h"

#include "base/files/file_path.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_service.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_service_waiter.h"
#include "brave/components/ntp_background_images/browser/switches.h"

namespace ntp_background_images::test {

NTPSponsoredSiteImageSourceTestBase::NTPSponsoredSiteImageSourceTestBase() =
    default;

NTPSponsoredSiteImageSourceTestBase::~NTPSponsoredSiteImageSourceTestBase() =
    default;

void NTPSponsoredSiteImageSourceTestBase::SetUp() {
  NTPBackgroundImagesService::RegisterLocalStatePrefsForMigration(
      pref_service_.registry());

  background_images_service_ = std::make_unique<NTPBackgroundImagesService>(
      /*variations_service=*/nullptr, /*component_update_service=*/nullptr,
      &pref_service_);
}

void NTPSponsoredSiteImageSourceTestBase::SimulateOnSponsoredSitesDataDidUpdate(
    const base::FilePath& component_path) {
  url_data_source_ = std::make_unique<NTPSponsoredSiteImageSource>(
      background_images_service_.get());

  scoped_command_line_.GetProcessCommandLine()->AppendSwitchPath(
      switches::kOverrideSponsoredImagesComponentPath, component_path);

  NTPBackgroundImagesServiceWaiter waiter(*background_images_service_);
  background_images_service_->Init();
  waiter.WaitForOnSponsoredSitesDataDidUpdate();
}

}  // namespace ntp_background_images::test
