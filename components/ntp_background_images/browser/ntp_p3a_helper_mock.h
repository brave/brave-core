/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_NTP_P3A_HELPER_MOCK_H_
#define BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_NTP_P3A_HELPER_MOCK_H_

#include <string>

#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"
#include "brave/components/ntp_background_images/browser/ntp_p3a_helper.h"
#include "testing/gmock/include/gmock/gmock.h"

class GURL;

namespace ntp_background_images {

class NTPP3AHelperMock : public NTPP3AHelper {
 public:
  NTPP3AHelperMock();

  NTPP3AHelperMock(const NTPP3AHelperMock&) = delete;
  NTPP3AHelperMock& operator=(const NTPP3AHelperMock&) = delete;

  ~NTPP3AHelperMock() override;

  MOCK_METHOD(void,
              RecordView,
              (const std::string& creative_instance_id,
               const std::string& campaign_id),
              (override));

  MOCK_METHOD(void,
              RecordNewTabPageAdEvent,
              (brave_ads::mojom::NewTabPageAdEventType mojom_ad_event_type,
               const std::string& creative_instance_id),
              (override));

  MOCK_METHOD(void, OnNavigationDidFinish, (const GURL& url), (override));
};

}  // namespace ntp_background_images

#endif  // BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_NTP_P3A_HELPER_MOCK_H_
