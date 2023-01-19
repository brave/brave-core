/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/common/logging_util.h"

#include "bat/ads/internal/ads_client_helper.h"

namespace ads {

void Log(const char* file,
         const int line,
         const int verbose_level,
         const std::string& message) {
  if (!AdsClientHelper::HasInstance()) {
    return;
  }

  AdsClientHelper::GetInstance()->Log(file, line, verbose_level, message);
}

}  // namespace ads
