/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_CONFIRMATIONS_PUBLISHER_AD_INFO_H_
#define BAT_CONFIRMATIONS_PUBLISHER_AD_INFO_H_

#include <string>

#include "bat/confirmations/ad_info.h"

namespace confirmations {

struct CONFIRMATIONS_EXPORT PublisherAdInfo : AdInfo {
  PublisherAdInfo();
  PublisherAdInfo(
      const PublisherAdInfo& info);
  ~PublisherAdInfo();

  std::string size;
  std::string creative_url;
};

}  // namespace confirmations

#endif  // BAT_CONFIRMATIONS_PUBLISHER_AD_INFO_H_
