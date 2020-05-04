/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/log_stream_impl_mock.h"

#include <iostream>

namespace ads {

LogStreamImplMock::LogStreamImplMock(
    const char* file,
    const int line,
    const LogLevel log_level) {
  (void)file;
  (void)line;
  (void)log_level;
}

std::ostream& LogStreamImplMock::stream() {
  return std::cout;
}

}  // namespace ads
