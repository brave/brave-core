/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <sstream>
#include <fstream>

#include "bat/confirmations/export.h"
#include "bat/confirmations/wallet_info.h"

namespace confirmations {

CONFIRMATIONS_EXPORT enum LogLevel {
  LOG_ERROR = 1,
  LOG_WARNING = 2,
  LOG_INFO = 3
};

enum CONFIRMATIONS_EXPORT URLRequestMethod {
  GET = 0,
  PUT = 1,
  POST = 2
};

enum CONFIRMATIONS_EXPORT Result {
  SUCCESS,
  FAILED
};

class CONFIRMATIONS_EXPORT LogStream {
 public:
  virtual ~LogStream() = default;
  virtual std::ostream& stream() = 0;
};

using URLRequestCallback = std::function<void(const int, const std::string&,
  const std::map<std::string, std::string>& headers)>;

class CONFIRMATIONS_EXPORT ConfirmationsClient {
 public:
  virtual ~ConfirmationsClient() = default;

  // Should return true if Brave Ads is enabled otherwise returns false
  virtual bool IsAdsEnabled() const = 0;

  // Should get wallet info
  virtual void GetWalletInfo(WalletInfo *info) const = 0;

  // Should create a timer to trigger after the time offset specified in
  // seconds. If the timer was created successfully a unique identifier should
  // be returned, otherwise returns 0
  virtual uint32_t SetTimer(const uint64_t time_offset) = 0;

  // Should destroy the timer associated with the specified timer identifier
  virtual void KillTimer(uint32_t timer_id) = 0;

  // Should start a URL request
  virtual void URLRequest(
      const std::string& url,
      const std::vector<std::string>& headers,
      const std::string& content,
      const std::string& content_type,
      const URLRequestMethod method,
      URLRequestCallback callback) = 0;

  // Should log diagnostic information
  virtual std::unique_ptr<LogStream> Log(
      const char* file,
      const int line,
      const confirmations::LogLevel log_level) const = 0;
};

}  // namespace confirmations
