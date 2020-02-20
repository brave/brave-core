/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_CATALOG_TYPE_INFO_H_
#define BAT_ADS_INTERNAL_CATALOG_TYPE_INFO_H_

#include <stdint.h>
#include <string>

namespace ads {

struct TypeInfo {
  TypeInfo();
  TypeInfo(
      const TypeInfo& info);
  ~TypeInfo();

  std::string code;
  std::string name;
  std::string platform;
  uint64_t version = 0;
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_CATALOG_TYPE_INFO_H_
