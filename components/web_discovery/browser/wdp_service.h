/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_WDP_SERVICE_H_
#define BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_WDP_SERVICE_H_

#include "base/memory/raw_ptr.h"
#include "components/keyed_service/core/keyed_service.h"

class PrefService;

namespace web_discovery {

class WDPService : public KeyedService {
 public:
  explicit WDPService(PrefService* profile_prefs);

  WDPService(const WDPService&) = delete;
  WDPService& operator=(const WDPService&) = delete;

 private:
  raw_ptr<PrefService> profile_prefs_;
};

}  // namespace web_discovery

#endif  // BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_WDP_SERVICE_H_
