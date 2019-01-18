/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <iostream>
#include <fstream>
#include <limits>

#include "mock_confirmations_client.h"
#include "bat/confirmations/notification_info.h"
#include "static_values.h"

class MockLogStreamImpl : public confirmations::LogStream {
 public:
  MockLogStreamImpl(
      const char* file,
      const int line,
      const confirmations::LogLevel log_level) {
    std::string level;

    switch (log_level) {
      case confirmations::LogLevel::LOG_ERROR: {
        level = "ERROR";
        break;
      }
      case confirmations::LogLevel::LOG_WARNING: {
        level = "WARNING";
        break;
      }
      case confirmations::LogLevel::LOG_INFO: {
        level = "INFO";
        break;
      }
    }

    log_message_ = level + ": in " + file + " on line "
      + std::to_string(line) + ": ";
  }

  std::ostream& stream() override {
    std::cout << std::endl << log_message_;
    return std::cout;
  }

 private:
  std::string log_message_;

  // Not copyable, not assignable
  MockLogStreamImpl(const MockLogStreamImpl&) = delete;
  MockLogStreamImpl& operator=(const MockLogStreamImpl&) = delete;
};

namespace confirmations {

#define LOG(severity) \
  Log(__FILE__, __LINE__, severity)->stream()

MockConfirmationsClient::MockConfirmationsClient() :
    confirmations_(Confirmations::CreateInstance(this)) {
}

MockConfirmationsClient::~MockConfirmationsClient() = default;

uint32_t MockConfirmationsClient::SetTimer(const uint64_t time_offset) {
  (void)time_offset;

  static uint32_t mock_timer_id = 0;
  mock_timer_id++;

  return mock_timer_id;
}

void MockConfirmationsClient::KillTimer(uint32_t timer_id) {
  (void)timer_id;
}

void MockConfirmationsClient::Save(
    const std::string& name,
    const std::string& value,
    OnSaveCallback callback) {
  callback(FAILED);
}

void MockConfirmationsClient::Load(
    const std::string& name,
    OnLoadCallback callback) {
  callback(FAILED, "");
}

void MockConfirmationsClient::Reset(
    const std::string& name,
    OnResetCallback callback) {
  callback(FAILED);
}

std::unique_ptr<LogStream> MockConfirmationsClient::Log(
    const char* file,
    const int line,
    const LogLevel log_level) const {
  return std::make_unique<MockLogStreamImpl>(file, line, log_level);
}

}  // namespace confirmations
