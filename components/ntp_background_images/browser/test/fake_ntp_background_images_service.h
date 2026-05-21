/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_TEST_FAKE_NTP_BACKGROUND_IMAGES_SERVICE_H_
#define BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_TEST_FAKE_NTP_BACKGROUND_IMAGES_SERVICE_H_

#include <cstddef>

#include "brave/components/ntp_background_images/browser/ntp_background_images_service.h"

class PrefService;

namespace component_updater {
class ComponentUpdateService;
}  // namespace component_updater

namespace variations {
class VariationsService;
}  // namespace variations

namespace ntp_background_images {

class FakeNTPBackgroundImagesService final : public NTPBackgroundImagesService {
 public:
  FakeNTPBackgroundImagesService(
      variations::VariationsService* variations_service,
      component_updater::ComponentUpdateService* component_update_service,
      PrefService* pref_service);

  FakeNTPBackgroundImagesService(const FakeNTPBackgroundImagesService&) =
      delete;
  FakeNTPBackgroundImagesService& operator=(
      const FakeNTPBackgroundImagesService&) = delete;

  ~FakeNTPBackgroundImagesService() override;

  void RegisterSponsoredImagesComponent() override;

  size_t register_sponsored_images_component_call_count() const {
    return register_sponsored_images_component_call_count_;
  }

 private:
  size_t register_sponsored_images_component_call_count_ = 0;
};

}  // namespace ntp_background_images

#endif  // BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_TEST_FAKE_NTP_BACKGROUND_IMAGES_SERVICE_H_
