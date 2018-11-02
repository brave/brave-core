/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>

#include "bat/ads/export.h"

namespace ads {

struct ADS_EXPORT AdInfo {
  AdInfo() :
    category(""),
    advertiser(""),
    notification_text(""),
    notification_url(""),
    uuid("") {}

  AdInfo(const AdInfo& info) :
    category(info.category),
    advertiser(info.advertiser),
    notification_text(info.notification_text),
    notification_url(info.notification_url),
    uuid(info.uuid) {}

  ~AdInfo() {}

  std::string category;
  std::string advertiser;
  std::string notification_text;
  std::string notification_url;
  std::string uuid;
};

}  // namespace ads
