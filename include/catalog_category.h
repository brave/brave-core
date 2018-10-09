/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>

namespace catalog {

struct CategoryInfo {
  CategoryInfo() :
      advertiser(""),
      notification_text(""),
      notification_url(""),
      uuid(""),
      create_set_id(0) {}

  CategoryInfo(const CategoryInfo& info) :
      advertiser(info.advertiser),
      notification_text(info.notification_text),
      notification_url(info.notification_url),
      uuid(info.uuid),
      create_set_id(info.create_set_id) {}

  ~CategoryInfo() {}

  std::string advertiser;
  std::string notification_text;
  std::string notification_url;
  std::string uuid;
  int64_t create_set_id;
};

}  // namespace catalog
