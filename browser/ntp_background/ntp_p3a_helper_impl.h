/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NTP_BACKGROUND_NTP_P3A_HELPER_IMPL_H_
#define BRAVE_BROWSER_NTP_BACKGROUND_NTP_P3A_HELPER_IMPL_H_

#include "brave/components/ntp_background_images/browser/ntp_p3a_helper.h"

#include <string>

#include "base/callback_list.h"
#include "base/timer/timer.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

class PrefRegistrySimple;
class PrefService;

namespace brave {
class BraveP3AService;
}  // namespace brave

namespace ntp_background_images {

class NTPP3AHelperImpl : public NTPP3AHelper {
 public:
  NTPP3AHelperImpl(PrefService* local_state,
                   brave::BraveP3AService* p3a_service);
  ~NTPP3AHelperImpl() override;

  static void RegisterLocalStatePrefs(PrefRegistrySimple* registry);

  void RecordView(const std::string& creative_instance_id) override;

  void RecordClickAndMaybeLand(
      const std::string& creative_instance_id) override;

  void SetLastTabURL(const GURL& url) override;

  void OnP3ARotation(bool is_express);

  void OnP3AMetricSent(const std::string& histogram_name);

 private:
  std::string BuildHistogramName(const std::string& creative_instance_id,
                                 const std::string& event_type);

  void UpdateMetricCount(const std::string& creative_instance_id,
                         const std::string& event_type);

  void OnLandingStartCheck(const std::string& creative_instance_id);

  void OnLandingEndCheck(const std::string& creative_instance_id,
                         const std::string& expected_hostname);

  raw_ptr<PrefService> local_state_;
  raw_ptr<brave::BraveP3AService> p3a_service_;

  absl::optional<std::string> last_tab_hostname_;

  base::OneShotTimer landing_check_timer_;

  base::CallbackListSubscription metric_sent_subscription_;
  base::CallbackListSubscription rotation_subscription_;
};

}  // namespace ntp_background_images

#endif  // BRAVE_BROWSER_NTP_BACKGROUND_NTP_P3A_HELPER_IMPL_H_
