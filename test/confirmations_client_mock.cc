// /* This Source Code Form is subject to the terms of the Mozilla Public
//  * License, v. 2.0. If a copy of the MPL was not distributed with this
//  * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "confirmations_client_mock.h"

namespace confirmations {

MockLogStreamImpl::MockLogStreamImpl(
    const char* file,
    const int line,
    const LogLevel log_level) {
  std::string level;

  switch (log_level) {
    case LogLevel::LOG_ERROR: {
      level = "ERROR";
      break;
    }
    case LogLevel::LOG_WARNING: {
      level = "WARNING";
      break;
    }
    case LogLevel::LOG_INFO: {
      level = "INFO";
      break;
    }
  }

  log_message_ = level + ": in " + file + " on line "
    + std::to_string(line) + ": ";
}

std::ostream& MockLogStreamImpl::stream() {
  std::cout << std::endl << log_message_;
  return std::cout;
}

MockConfirmationsClient::MockConfirmationsClient() = default;

MockConfirmationsClient::~MockConfirmationsClient() = default;

}  // namespace confirmations
