/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_CATALOG_GEO_TARGET_INFO_H_
#define BAT_ADS_CATALOG_GEO_TARGET_INFO_H_

#include <string>

namespace ads {

struct GeoTargetInfo {
  GeoTargetInfo() :
      code(""),
      name("") {}

  explicit GeoTargetInfo(const GeoTargetInfo& info) :
      code(info.code),
      name(info.name) {}

  ~GeoTargetInfo() {}

  std::string code;
  std::string name;
};

}  // namespace ads

#endif  // BAT_ADS_CATALOG_GEO_TARGET_INFO_H_
