/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_SERVICE_H_

#include <string>

#include "base/macros.h"
#include "build/build_config.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/sessions/core/session_id.h"
#include "chrome/browser/profiles/profile.h"
#include "url/gurl.h"

namespace brave_ads {

class AdsService : public KeyedService {
 public:
  AdsService() = default;

  virtual bool IsSupportedRegion() const = 0;
  virtual bool IsEnabled() const = 0;
  virtual uint64_t AdsPerHour() const = 0;

  virtual void SetAdsEnabled(bool enabled) = 0;
  virtual void SetAdsPerHour(uint64_t ads_per_hour) = 0;

  // ads::Ads proxy
  virtual void TabUpdated(
      SessionID tab_id,
      const GURL& url,
      const bool is_active) = 0;
  virtual void TabClosed(SessionID tab_id) = 0;
  virtual void OnMediaStart(SessionID tab_id) = 0;
  virtual void OnMediaStop(SessionID tab_id) = 0;
  virtual void ClassifyPage(const std::string& url,
                            const std::string& page) = 0;
  virtual void OnShow(Profile* profile, const std::string& notification_id) = 0;
  virtual void OpenSettings(Profile* profile,
                                  const GURL& origin,
                                  bool should_close) = 0;
  virtual void OnClose(Profile* profile,
                       const GURL& origin,
                       const std::string& notification_id,
                       bool by_user,
                       base::OnceClosure completed_closure) = 0;
  virtual void SetConfirmationsIsReady(const bool is_ready) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(AdsService);
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_SERVICE_H_
