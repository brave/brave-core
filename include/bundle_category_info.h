/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>

#include "export.h"

namespace bundle {

ADS_EXPORT struct CategoryInfo {
  CategoryInfo() :
    creative_set_id(""),
    advertiser(""),
    notification_text(""),
    notification_url(""),
    uuid("") {}

  CategoryInfo(const CategoryInfo& info) :
    creative_set_id(info.creative_set_id),
    advertiser(info.advertiser),
    notification_text(info.notification_text),
    notification_url(info.notification_url),
    uuid(info.uuid) {}

  ~CategoryInfo() {}

  std::string creative_set_id;
  std::string advertiser;
  std::string notification_text;
  std::string notification_url;
  std::string uuid;
};

}  // namespace bundle
