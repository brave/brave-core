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
#include "url/gurl.h"

namespace brave_ads {

class AdsService : public KeyedService {
 public:
  AdsService() = default;

  virtual bool is_enabled() const = 0;
  virtual uint64_t ads_per_hour() const = 0;

  virtual void set_ads_enabled(bool enabled) = 0;
  virtual void set_ads_per_hour(int ads_per_hour) = 0;

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
  virtual void SetConfirmationsIsReady(const bool is_ready) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(AdsService);
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_SERVICE_H_
