/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_NTP_SPONSORED_IMAGES_NTP_SPONSORED_IMAGES_SERVICE_H_
#define BRAVE_COMPONENTS_NTP_SPONSORED_IMAGES_NTP_SPONSORED_IMAGES_SERVICE_H_

#include "base/component_export.h"
#include "brave/components/ntp_sponsored_images/ntp_sponsored_images_component_manager.h"
#include "components/keyed_service/core/keyed_service.h"

namespace content {
class BrowserContext;
}  // namespace content

class COMPONENT_EXPORT(BRAVE_COMPONENTS_NTP_SPONSORED_IMAGES)
NTPSponsoredImagesService
    : public KeyedService,
      public NTPSponsoredImagesComponentManager::Observer {
 public:
  NTPSponsoredImagesService(content::BrowserContext* browser_context,
                            NTPSponsoredImagesComponentManager* manager);
  ~NTPSponsoredImagesService() override;

  NTPSponsoredImagesService(const NTPSponsoredImagesService&) = delete;
  NTPSponsoredImagesService operator=(
      const NTPSponsoredImagesService&) = delete;

 private:
  // NTPSponsoredImagesComponentManager::Observer overrides:
  void OnUpdated(const NTPSponsoredImagesData& data) override;

  content::BrowserContext* browser_context_;
  NTPSponsoredImagesComponentManager* manager_;
};

#endif  // BRAVE_COMPONENTS_NTP_SPONSORED_IMAGES_NTP_SPONSORED_IMAGES_SERVICE_H_
