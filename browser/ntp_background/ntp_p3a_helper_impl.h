/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NTP_BACKGROUND_NTP_P3A_HELPER_IMPL_H_
#define BRAVE_BROWSER_NTP_BACKGROUND_NTP_P3A_HELPER_IMPL_H_

#include <optional>
#include <string>

#include "base/callback_list.h"
#include "base/scoped_observation.h"
#include "base/timer/timer.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_service.h"
#include "brave/components/ntp_background_images/browser/ntp_p3a_helper.h"

class GURL;
class PrefService;

namespace p3a {
class P3AService;
enum class MetricLogType;
}  // namespace p3a

namespace ntp_background_images {

class NTPP3AHelperImpl : public NTPP3AHelper,
                         public NTPBackgroundImagesService::Observer {
 public:
  NTPP3AHelperImpl(PrefService* local_state,
                   p3a::P3AService* p3a_service,
                   NTPBackgroundImagesService* ntp_background_images_service,
                   PrefService* prefs);
  ~NTPP3AHelperImpl() override;

  void RecordView(const std::string& creative_instance_id,
                  const std::string& campaign_id) override;

  void RecordNewTabPageAdEvent(
      brave_ads::mojom::NewTabPageAdEventType mojom_ad_event_type,
      const std::string& creative_instance_id) override;

  void OnNavigationDidFinish(const GURL& url) override;

  void CheckLoadedCampaigns(const NTPSponsoredImagesData& data);

  // See BraveP3AService::RegisterDynamicMetric and
  // BraveP3AService::RegisterMetricCycledCallback header comments for more
  // info.
  void OnP3ARotation(p3a::MetricLogType log_type);
  void OnP3AMetricCycled(const std::string& histogram_name);

 private:
  void MaybeLand(const GURL& url);
  void MaybeLandCallback(const std::string& creative_instance_id,
                         const GURL& url);

  void RecordCreativeMetric(const std::string& histogram_name, int count);
  void RemoveMetricIfInstanceDoesNotExist(
      const std::string& histogram_name,
      const std::string& event_type,
      const std::string& creative_instance_id);
  void CleanOldCampaignsAndCreatives();

  void UpdateMetricCount(const std::string& creative_instance_id,
                         const std::string& event_type);

  void UpdateCampaignMetric(const std::string& campaign_id,
                            const std::string& event_type);

  // NTPBackgroundImagesService::Observer:
  void OnSponsoredImagesDataDidUpdate(NTPSponsoredImagesData* data) override;

  raw_ptr<PrefService> local_state_;
  raw_ptr<p3a::P3AService> p3a_service_;
  raw_ptr<PrefService> prefs_;

  std::optional<GURL> last_url_;
  std::optional<std::string> last_clicked_creative_instance_id_;
  base::OneShotTimer page_land_timer_;

  base::CallbackListSubscription metric_sent_subscription_;
  base::CallbackListSubscription rotation_subscription_;

  base::ScopedObservation<NTPBackgroundImagesService,
                          NTPBackgroundImagesService::Observer>
      ntp_background_images_service_observation_{this};
};

}  // namespace ntp_background_images

#endif  // BRAVE_BROWSER_NTP_BACKGROUND_NTP_P3A_HELPER_IMPL_H_
