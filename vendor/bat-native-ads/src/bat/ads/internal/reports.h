/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_REPORTS_H_
#define BAT_ADS_INTERNAL_REPORTS_H_

#include <string>

#include "bat/ads/confirmation_type.h"
#include "bat/ads/mojom.h"
#include "bat/ads/internal/classification_helper.h"
#include "bat/ads/internal/event_type_blur_info.h"
#include "bat/ads/internal/event_type_destroy_info.h"
#include "bat/ads/internal/event_type_focus_info.h"
#include "bat/ads/internal/event_type_load_info.h"

namespace ads {

class AdsImpl;
struct AdNotificationInfo;
struct PublisherAdInfo;

class Reports {
 public:
  Reports(
      AdsImpl* ads);
  ~Reports();

  std::string GenerateAdNotificationEventReport(
      const AdNotificationInfo& info,
      const AdNotificationEventType event_type);

  std::string GeneratePublisherAdEventReport(
      const PublisherAdInfo& info,
      const PublisherAdEventType event_type);

  std::string GenerateConfirmationEventReport(
      const std::string& creative_instance_id,
      const ConfirmationType& confirmation_type) const;

  std::string GenerateLoadEventReport(
      const LoadInfo& info) const;

  std::string GenerateBackgroundEventReport() const;

  std::string GenerateForegroundEventReport() const;

  std::string GenerateBlurEventReport(
      const BlurInfo& info) const;

  std::string GenerateDestroyEventReport(
      const DestroyInfo& info) const;

  std::string GenerateFocusEventReport(
      const FocusInfo& info) const;

  std::string GenerateSettingsEventReport() const;

 private:
  bool is_first_run_;

  AdsImpl* ads_;  // NOT OWNED
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_REPORTS_H_
