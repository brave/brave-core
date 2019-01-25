/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_CONFIRMATIONS_CONFIRMATIONS_CLIENT_MOCK_H_
#define BAT_CONFIRMATIONS_CONFIRMATIONS_CLIENT_MOCK_H_

#include <iostream>

#include "testing/gmock/include/gmock/gmock.h"

#include "bat/confirmations/confirmations_client.h"
#include "bat/confirmations/confirmations.h"

namespace confirmations {

class MockLogStreamImpl : public LogStream {
 public:
  MockLogStreamImpl(const char* file, int line, const LogLevel log_level);
  std::ostream& stream() override;

 private:
  std::string log_message_;

  // Not copyable, not assignable
  MockLogStreamImpl(const MockLogStreamImpl&) = delete;
  MockLogStreamImpl& operator=(const MockLogStreamImpl&) = delete;
};

class MockConfirmationsClient : public ConfirmationsClient {
 public:
  MockConfirmationsClient();
  ~MockConfirmationsClient() override;

  MOCK_METHOD1(SetTimer, uint32_t(
      const uint64_t time_offset));

  MOCK_METHOD1(KillTimer, void(
      const uint32_t timer_id));

  MOCK_METHOD6(URLRequest, void(
      const std::string& url,
      const std::vector<std::string>& headers,
      const std::string& content,
      const std::string& content_type,
      const URLRequestMethod method,
      URLRequestCallback callback));

  MOCK_METHOD3(Save, void(
      const std::string& name,
      const std::string& value,
      OnSaveCallback callback));

  MOCK_METHOD2(Load, void(
      const std::string& name,
      OnLoadCallback callback));

  MOCK_METHOD2(Reset, void(
      const std::string& name,
      OnResetCallback callback));

  MOCK_METHOD1(SetConfirmationsIsReady, void(
      const bool is_ready));

  std::unique_ptr<LogStream> Log(
      const char* file,
      const int line,
      const LogLevel log_level) const;
};

}  // namespace confirmations

#endif  // BAT_CONFIRMATIONS_CONFIRMATIONS_CLIENT_MOCK_H_
