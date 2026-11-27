/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/test/fake_ntp_background_images_service.h"

#include <utility>

#include "brave/components/ntp_background_images/browser/ntp_background_images_data.h"
#include "brave/components/ntp_background_images/browser/sponsored_content/site/ntp_sponsored_sites_data.h"

namespace ntp_background_images {

FakeNTPBackgroundImagesService::FakeNTPBackgroundImagesService(
    component_updater::ComponentUpdateService* component_update_service,
    sponsored_content::SponsoredContentService& sponsored_content_service,
    PrefService* pref_service)
    : NTPBackgroundImagesService(component_update_service,
                                 sponsored_content_service,
                                 pref_service) {}

FakeNTPBackgroundImagesService::~FakeNTPBackgroundImagesService() = default;

void FakeNTPBackgroundImagesService::RegisterSponsoredImagesComponent() {
  NTPBackgroundImagesService::RegisterSponsoredImagesComponent();
  ++register_sponsored_images_component_call_count_;
}

void FakeNTPBackgroundImagesService::UnregisterSponsoredImagesComponent() {
  NTPBackgroundImagesService::UnregisterSponsoredImagesComponent();
  ++unregister_sponsored_images_component_call_count_;
}

void FakeNTPBackgroundImagesService::SetBackgroundImagesData(
    std::unique_ptr<NTPBackgroundImagesData> data) {
  background_images_data_ = std::move(data);
}

void FakeNTPBackgroundImagesService::SetSponsoredSitesData(
    std::unique_ptr<NTPSponsoredSitesData> sponsored_sites_data) {
  sponsored_sites_data_ = std::move(sponsored_sites_data);
  observers_.Notify(&Observer::OnSponsoredSitesDataDidUpdate);
}

}  // namespace ntp_background_images
