/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <iostream>
#include <fstream>
#include <limits>

#include "mock_confirmations_client.h"
#include "bat/confirmations/ad_info.h"
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

bool MockConfirmationsClient::IsAdsEnabled() const {
  return true;
}

void MockConfirmationsClient::GetWalletInfo(WalletInfo* info) const {
  info->payment_id = "1234567890";
  info->signing_key = "abcdefghijklmnoprstuvwxyz";
}

uint32_t MockConfirmationsClient::SetTimer(const uint64_t time_offset) {
  (void)time_offset;

  static uint32_t mock_timer_id = 0;
  mock_timer_id++;

  return mock_timer_id;
}

void MockConfirmationsClient::KillTimer(uint32_t timer_id) {
  (void)timer_id;
}

void MockConfirmationsClient::URLRequest(
    const std::string& url,
    const std::vector<std::string>& headers,
    const std::string& content,
    const std::string& content_type,
    const URLRequestMethod method,
    URLRequestCallback callback) {
  (void)url;
  (void)headers;
  (void)content;
  (void)content_type;
  (void)method;

  auto response_status_code = 200;
  std::string response = "";

  std::ifstream ifs{"test/data/catalog.json"};
  if (ifs.fail()) {
    response_status_code = 404;
  } else {
    std::stringstream stream;
    stream << ifs.rdbuf();

    response = stream.str();
  }

  callback(response_status_code, response, {});
}

std::unique_ptr<LogStream> MockConfirmationsClient::Log(
    const char* file,
    const int line,
    const LogLevel log_level) const {
  return std::make_unique<MockLogStreamImpl>(file, line, log_level);
}

}  // namespace confirmations
