/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_LOG_STREAM_IMPL_MOCK_H_
#define BAT_ADS_INTERNAL_LOG_STREAM_IMPL_MOCK_H_

#include "bat/ads/log_stream.h"

#include <ostream>

namespace ads {

class LogStreamImplMock : public LogStream {
 public:
  LogStreamImplMock(
      const char* file,
      const int line,
      const LogLevel log_level);

  LogStreamImplMock(const LogStreamImplMock&) = delete;
  LogStreamImplMock& operator=(const LogStreamImplMock&) = delete;

  std::ostream& stream() override;
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_LOG_STREAM_IMPL_MOCK_H_
