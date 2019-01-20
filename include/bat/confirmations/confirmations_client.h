/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_CONFIRMATIONS_CONFIRMATIONS_CLIENT_H_
#define BAT_CONFIRMATIONS_CONFIRMATIONS_CLIENT_H_

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

enum CONFIRMATIONS_EXPORT Result {
  SUCCESS,
  FAILED
};

class CONFIRMATIONS_EXPORT LogStream {
 public:
  virtual ~LogStream() = default;
  virtual std::ostream& stream() = 0;
};

using OnSaveCallback = std::function<void(const Result)>;
using OnLoadCallback = std::function<void(const Result, const std::string&)>;

using OnResetCallback = std::function<void(const Result)>;

class CONFIRMATIONS_EXPORT ConfirmationsClient {
 public:
  virtual ~ConfirmationsClient() = default;

  // Should create a timer to trigger after the time offset specified in
  // seconds. If the timer was created successfully a unique identifier should
  // be returned, otherwise returns 0
  virtual uint32_t SetTimer(const uint64_t time_offset) = 0;

  // Should destroy the timer associated with the specified timer identifier
  virtual void KillTimer(uint32_t timer_id) = 0;

  // Should save a value to persistent storage
  virtual void Save(
      const std::string& name,
      const std::string& value,
      OnSaveCallback callback) = 0;

  // Should load a value from persistent storage
  virtual void Load(const std::string& name, OnLoadCallback callback) = 0;

  // Should reset a previously saved value, i.e. remove the file from persistent
  // storage
  virtual void Reset(const std::string& name, OnResetCallback callback) = 0;

  // Should log diagnostic information
  virtual std::unique_ptr<LogStream> Log(
      const char* file,
      const int line,
      const confirmations::LogLevel log_level) const = 0;
};

}  // namespace confirmations

#endif  // BAT_CONFIRMATIONS_CONFIRMATIONS_CLIENT_H_
