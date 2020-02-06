/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_ISSUER_INFO_H_
#define BAT_ADS_ISSUER_INFO_H_

#include <string>

#include "bat/ads/export.h"

namespace ads {

struct ADS_EXPORT IssuerInfo {
  std::string name;
  std::string public_key;
};

}  // namespace ads

#endif  // BAT_ADS_ISSUER_INFO_H_
