/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_TEST_FAKE_NTP_BACKGROUND_IMAGES_SERVICE_H_
#define BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_TEST_FAKE_NTP_BACKGROUND_IMAGES_SERVICE_H_

#include <cstddef>
#include <memory>

#include "brave/components/ntp_background_images/browser/ntp_background_images_service.h"

class PrefService;

namespace component_updater {
class ComponentUpdateService;
}  // namespace component_updater

namespace sponsored_content {
class SponsoredContentService;
}  // namespace sponsored_content

namespace ntp_background_images {

struct NTPBackgroundImagesData;
struct NTPSponsoredSitesData;

class FakeNTPBackgroundImagesService final : public NTPBackgroundImagesService {
 public:
  FakeNTPBackgroundImagesService(
      component_updater::ComponentUpdateService* component_update_service,
      sponsored_content::SponsoredContentService& sponsored_content_service,
      PrefService* pref_service);

  FakeNTPBackgroundImagesService(const FakeNTPBackgroundImagesService&) =
      delete;
  FakeNTPBackgroundImagesService& operator=(
      const FakeNTPBackgroundImagesService&) = delete;

  ~FakeNTPBackgroundImagesService() override;

  void RegisterSponsoredImagesComponent() override;
  void UnregisterSponsoredImagesComponent() override;

  // Test-only entry point: injects `data` as the current background images
  // data, as if the component had just been loaded.
  void SetBackgroundImagesData(
      std::unique_ptr<NTPBackgroundImagesData> data);

  // Test-only entry point, injects `data` as the current sponsored sites
  // data, as if the component had just been loaded.
  void SetSponsoredSitesData(
      std::unique_ptr<NTPSponsoredSitesData> sponsored_sites_data);

  size_t register_sponsored_images_component_call_count() const {
    return register_sponsored_images_component_call_count_;
  }

  size_t unregister_sponsored_images_component_call_count() const {
    return unregister_sponsored_images_component_call_count_;
  }

 private:
  size_t register_sponsored_images_component_call_count_ = 0;
  size_t unregister_sponsored_images_component_call_count_ = 0;
};

}  // namespace ntp_background_images

#endif  // BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_TEST_FAKE_NTP_BACKGROUND_IMAGES_SERVICE_H_
