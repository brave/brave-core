/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_ADS_SERVICE_
#define BRAVE_COMPONENTS_BRAVE_ADS_ADS_SERVICE_

#include "base/macros.h"
#include "build/build_config.h"
#include "components/keyed_service/core/keyed_service.h"

namespace brave_ads {

class AdsService : public KeyedService {
 public:
  AdsService() = default;

  virtual bool is_enabled() = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(AdsService);
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_ADS_SERVICE_
