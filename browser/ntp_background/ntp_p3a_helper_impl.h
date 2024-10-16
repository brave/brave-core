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
#include "brave/components/ntp_background_images/browser/ntp_background_images_service.h"
#include "brave/components/ntp_background_images/browser/ntp_p3a_helper.h"

class PrefRegistrySimple;
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
                   ntp_background_images::NTPBackgroundImagesService*
                       ntp_background_images_service,
                   PrefService* prefs,
                   bool use_uma_for_testing = false);
  ~NTPP3AHelperImpl() override;

  static void RegisterLocalStatePrefs(PrefRegistrySimple* registry);

  void RecordView(const std::string& creative_instance_id,
                  const std::string& campaign_id) override;

  void RecordClickAndMaybeLand(
      const std::string& creative_instance_id) override;

  void SetLastTabURL(const GURL& url) override;

  void CheckLoadedCampaigns(const NTPSponsoredImagesData& data);

  // See BraveP3AService::RegisterDynamicMetric and
  // BraveP3AService::RegisterMetricCycledCallback header comments for more
  // info.
  void OnP3ARotation(p3a::MetricLogType log_type, bool is_constellation);
  void OnP3AMetricCycled(const std::string& histogram_name,
                         bool is_constellation);

 private:
  void RecordCreativeMetric(const std::string& histogram_name,
                            int count,
                            bool is_constellation);
  void RemoveMetricIfInstanceDoesNotExist(
      const std::string& histogram_name,
      const std::string& event_type,
      const std::string& creative_instance_id);
  void CleanOldCampaignsAndCreatives();

  void UpdateMetricCount(const std::string& creative_instance_id,
                         const std::string& event_type);

  void UpdateCampaignMetric(const std::string& campaign_id,
                            const std::string& event_type);

  void OnLandingStartCheck(const std::string& creative_instance_id);

  void OnLandingEndCheck(const std::string& creative_instance_id,
                         const std::string& expected_hostname);

  // NTPBackgroundImagesService::Observer:
  void OnUpdated(NTPBackgroundImagesData* data) override;
  void OnUpdated(NTPSponsoredImagesData* data) override;
  void OnSuperReferralEnded() override;

  raw_ptr<PrefService> local_state_;
  raw_ptr<p3a::P3AService> p3a_service_;
  raw_ptr<PrefService> prefs_;

  std::optional<std::string> last_tab_hostname_;

  base::OneShotTimer landing_check_timer_;

  base::CallbackListSubscription metric_sent_subscription_;
  base::CallbackListSubscription rotation_subscription_;

  base::ScopedObservation<NTPBackgroundImagesService,
                          NTPBackgroundImagesService::Observer>
      ntp_background_images_service_observation_{this};

  const bool is_json_deprecated_;
  bool use_uma_for_testing_;
};

}  // namespace ntp_background_images

#endif  // BRAVE_BROWSER_NTP_BACKGROUND_NTP_P3A_HELPER_IMPL_H_
