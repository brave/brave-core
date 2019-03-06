/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/confirmations/internal/confirmations_client_mock.h"

namespace confirmations {

MockLogStreamImpl::MockLogStreamImpl(
    const char* file,
    const int line,
    const ledger::LogLevel log_level) {
  (void)file;
  (void)line;
  (void)log_level;
}

std::ostream& MockLogStreamImpl::stream() {
  return std::cout;
}

MockVerboseLogStreamImpl::MockVerboseLogStreamImpl(
    const char* file,
    int line,
    int vlog_level) {
  (void)file;
  (void)line;
  (void)vlog_level;
}

std::ostream& MockVerboseLogStreamImpl::stream() {
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
