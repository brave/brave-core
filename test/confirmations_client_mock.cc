// /* This Source Code Form is subject to the terms of the Mozilla Public
//  * License, v. 2.0. If a copy of the MPL was not distributed with this
//  * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "confirmations_client_mock.h"

namespace confirmations {

MockLogStreamImpl::MockLogStreamImpl(
    const char* file,
    const int line,
    const ledger::LogLevel log_level) {
  std::string level;

  switch (log_level) {
    case ledger::LogLevel::LOG_ERROR: {
      level = "ERROR";
      break;
    }
    case ledger::LogLevel::LOG_WARNING: {
      level = "WARNING";
      break;
    }
    case ledger::LogLevel::LOG_INFO: {
      level = "INFO";
      break;
    }
    case ledger::LogLevel::LOG_DEBUG: {
      level = "DEBUG";
      break;
    }
    case ledger::LogLevel::LOG_REQUEST: {
      level = "REQUEST";
      break;
    }
    case ledger::LogLevel::LOG_RESPONSE: {
      level = "RESPONSE";
      break;
    }
  }

  log_message_ = level + ": in " + std::string(file) + " on line "
      + std::to_string(line) + ": ";
}

std::ostream& MockLogStreamImpl::stream() {
  std::cout << std::endl << log_message_;
  return std::cout;
}

MockVerboseLogStreamImpl::MockVerboseLogStreamImpl(
    const char* file,
    int line,
    int vlog_level) {
  log_message_ = "VLOG: in " + std::string(file) + " on line "
      + std::to_string(line) + ": ";
}

std::ostream& MockVerboseLogStreamImpl::stream() {
  std::cout << std::endl << log_message_;
  return std::cout;
}

MockConfirmationsClient::MockConfirmationsClient() = default;

MockConfirmationsClient::~MockConfirmationsClient() = default;

std::unique_ptr<ledger::LogStream> MockConfirmationsClient::Log(
    const char* file,
    int line,
    const ledger::LogLevel log_level) const {
  return std::make_unique<MockLogStreamImpl>(file, line, log_level);
}

std::unique_ptr<ledger::LogStream> MockConfirmationsClient::VerboseLog(
    const char* file,
    int line,
    int vlog_level) const {
  return std::make_unique<MockVerboseLogStreamImpl>(file, line, vlog_level);
}

}  // namespace confirmations
