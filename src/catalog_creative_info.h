/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_CATALOG_CREATIVE_INFO_H_
#define BAT_ADS_CATALOG_CREATIVE_INFO_H_

#include <string>

#include "catalog_type_info.h"
#include "catalog_payload_info.h"

namespace ads {

struct CreativeInfo {
  CreativeInfo();
  explicit CreativeInfo(const CreativeInfo& info);
  ~CreativeInfo();

  std::string creative_instance_id;
  TypeInfo type;
  PayloadInfo payload;
};

}  // namespace ads

#endif  // BAT_ADS_CATALOG_CREATIVE_INFO_H_
