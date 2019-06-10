/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_CONFIRMATIONS_INTERNAL_LOGGING_H_
#define BAT_CONFIRMATIONS_INTERNAL_LOGGING_H_

#include "bat/confirmations/confirmations_client.h"

#define CONFIRMATIONS_LOG_INFO \
  confirmations_client_->Log(__FILE__, __LINE__, ::ledger::LogLevel::LOG_INFO)

#define CONFIRMATIONS_LOG_WARNING \
  confirmations_client_->Log(     \
      __FILE__, __LINE__, ::ledger::LogLevel::LOG_WARNING)

#define CONFIRMATIONS_LOG_ERROR \
  confirmations_client_->Log(__FILE__, __LINE__, ::ledger::LogLevel::LOG_ERROR)

#define BLOG(severity) CONFIRMATIONS_LOG_##severity->stream()

#if defined(ERROR)
#define LOG_0 CONFIRMATIONS_LOG_ERROR
#endif

#endif  // BAT_CONFIRMATIONS_INTERNAL_LOGGING_H_
