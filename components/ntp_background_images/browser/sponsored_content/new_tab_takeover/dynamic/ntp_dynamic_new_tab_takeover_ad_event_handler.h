/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_SPONSORED_CONTENT_NEW_TAB_TAKEOVER_DYNAMIC_NTP_DYNAMIC_NEW_TAB_TAKEOVER_AD_EVENT_HANDLER_H_
#define BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_SPONSORED_CONTENT_NEW_TAB_TAKEOVER_DYNAMIC_NTP_DYNAMIC_NEW_TAB_TAKEOVER_AD_EVENT_HANDLER_H_

#include <string>

#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"
#include "brave/components/ntp_background_images/browser/mojom/ntp_background_images.mojom.h"
#include "mojo/public/cpp/bindings/receiver.h"

namespace brave_ads {
class AdsService;
}  // namespace brave_ads

namespace ntp_background_images {

class NTPDynamicNewTabTakeoverAdEventHandler
    : public mojom::SponsoredContentAdEventHandler {
 public:
  explicit NTPDynamicNewTabTakeoverAdEventHandler(
      brave_ads::AdsService* ads_service);

  NTPDynamicNewTabTakeoverAdEventHandler(
      const NTPDynamicNewTabTakeoverAdEventHandler&) = delete;
  NTPDynamicNewTabTakeoverAdEventHandler& operator=(
      const NTPDynamicNewTabTakeoverAdEventHandler&) = delete;

  ~NTPDynamicNewTabTakeoverAdEventHandler() override;

  void Bind(mojo::PendingReceiver<mojom::SponsoredContentAdEventHandler>
                pending_receiver);

  // mojom::SponsoredContentAdEventHandler:
  void MaybeReportSponsoredContentAdEvent(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      brave_ads::mojom::NewTabPageAdMetricType mojom_ad_metric_type,
      brave_ads::mojom::NewTabPageAdEventType mojom_ad_event_type) override;

 private:
  const raw_ptr<brave_ads::AdsService> ads_service_ = nullptr;  // Not owned.

  mojo::Receiver<mojom::SponsoredContentAdEventHandler> receiver_{this};
};

}  // namespace ntp_background_images

#endif  // BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_SPONSORED_CONTENT_NEW_TAB_TAKEOVER_DYNAMIC_NTP_DYNAMIC_NEW_TAB_TAKEOVER_AD_EVENT_HANDLER_H_
