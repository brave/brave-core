/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ADS_BRAVE_ADS_HOST_H_
#define BRAVE_BROWSER_BRAVE_ADS_BRAVE_ADS_HOST_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "brave/components/brave_ads/common/brave_ads_host.mojom.h"
#include "components/sessions/core/session_id.h"

class Profile;

namespace content {
class WebContents;
}  // namespace content

namespace brave_ads {

// The class handles ads requests from renderer side for Desktop platforms.
class BraveAdsHost : public brave_ads::mojom::BraveAdsHost {
 public:
  BraveAdsHost(Profile* profile, content::WebContents* web_contents);
  BraveAdsHost(const BraveAdsHost&) = delete;
  BraveAdsHost& operator=(const BraveAdsHost&) = delete;
  ~BraveAdsHost() override;

  // brave_ads::mojom::BraveAdsHost
  void MaybeTriggerAdViewedEvent(
      const std::string& creative_instance_id,
      MaybeTriggerAdViewedEventCallback callback) override;

 private:
  raw_ptr<Profile> profile_ = nullptr;
  SessionID tab_id_;
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_BRAVE_ADS_BRAVE_ADS_HOST_H_
