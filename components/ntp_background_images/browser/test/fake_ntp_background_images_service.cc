/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/test/fake_ntp_background_images_service.h"

#include <optional>

#include "base/files/file_path.h"
#include "base/json/json_reader.h"

namespace ntp_background_images {

FakeNTPBackgroundImagesService::FakeNTPBackgroundImagesService(
    variations::VariationsService* variations_service,
    component_updater::ComponentUpdateService* component_update_service,
    PrefService* pref_service)
    : NTPBackgroundImagesService(variations_service,
                                 component_update_service,
                                 pref_service) {}

FakeNTPBackgroundImagesService::~FakeNTPBackgroundImagesService() = default;

void FakeNTPBackgroundImagesService::RegisterSponsoredImagesComponent() {
  NTPBackgroundImagesService::RegisterSponsoredImagesComponent();
  ++register_sponsored_images_component_call_count_;
}

void FakeNTPBackgroundImagesService::OnGetSponsoredComponentJsonData(
    const std::string& json) {
  // Pass `std::nullopt` for `component_id` so the stale callback guard in
  // `OnHandledSponsoredComponentData` passes.
  NTPBackgroundImagesService::OnHandledSponsoredComponentData(
      /*component_id=*/std::nullopt,
      base::FilePath::FromASCII("fake_installed_dir"),
      base::JSONReader::ReadDict(json, base::JSON_PARSE_CHROMIUM_EXTENSIONS));
}

}  // namespace ntp_background_images
