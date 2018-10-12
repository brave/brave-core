// /* This Source Code Form is subject to the terms of the Mozilla Public
//  * License, v. 2.0. If a copy of the MPL was not distributed with this
//  * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "../include/ads.h"
#include "../include/ads_impl.h"

namespace ads {

bool is_production = false;
bool is_verbose = true;

// static
ads::Ads* Ads::CreateInstance(AdsClient* ads_client) {
  return new bat_ads::AdsImpl(ads_client);
}

}  // namespace ads
