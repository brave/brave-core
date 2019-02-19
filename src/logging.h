/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_LOGGING_H_
#define BAT_ADS_LOGGING_H_

#include "bat/ads/ads_client.h"

#define ADS_LOG_INFO \
  ads_client_->Log(__FILE__, __LINE__, ::ads::LogLevel::LOG_INFO)

#define ADS_LOG_WARNING \
  ads_client_->Log(__FILE__, __LINE__, ::ads::LogLevel::LOG_WARNING)

#define ADS_LOG_ERROR \
  ads_client_->Log(__FILE__, __LINE__, ::ads::LogLevel::LOG_ERROR)

#define LOG(severity) ADS_LOG_ ## severity->stream()

#if defined(ERROR)
#define LOG_0 ADS_LOG_ERROR
#endif

#endif  // BAT_ADS_LOGGING_H_
