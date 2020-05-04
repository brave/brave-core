/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_LOG_STREAM_H_
#define BAT_ADS_LOG_STREAM_H_

#include <ostream>

#include "bat/ads/export.h"

namespace ads {

enum LogLevel {
  LOG_ERROR = 1,
  LOG_WARNING,
  LOG_INFO
};

class ADS_EXPORT LogStream {
 public:
  virtual ~LogStream() = default;
  virtual std::ostream& stream() = 0;
};

}  // namespace ads

#endif  // BAT_ADS_LOG_STREAM_H_
