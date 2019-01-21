/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_CONFIRMATIONS_MOCK_CONFIRMATIONS_CLIENT_H_
#define BAT_CONFIRMATIONS_MOCK_CONFIRMATIONS_CLIENT_H_

#include <sstream>
#include <memory>

#include "bat/confirmations/confirmations_client.h"
#include "bat/confirmations/confirmations.h"
#include "bat/confirmations/wallet_info.h"

namespace confirmations {

class Confirmations;

class MockConfirmationsClient : public ConfirmationsClient {
 public:
  MockConfirmationsClient();
  ~MockConfirmationsClient() override;

  std::unique_ptr<Confirmations> confirmations_;

 protected:
  // ConfirmationsClient
  uint32_t SetTimer(const uint64_t time_offset) override;
  void KillTimer(const uint32_t timer_id) override;

  void URLRequest(
      const std::string& url,
      const std::vector<std::string>& headers,
      const std::string& content,
      const std::string& content_type,
      const URLRequestMethod method,
      URLRequestCallback callback) override;

  void Save(
      const std::string& name,
      const std::string& value,
      OnSaveCallback callback) override;

  void Load(const std::string& name, OnLoadCallback callback) override;

  void Reset(const std::string& name, OnResetCallback callback) override;

  std::unique_ptr<LogStream> Log(
      const char* file,
      const int line,
      const LogLevel log_level) const override;
};

}  // namespace confirmations

#endif  // BAT_CONFIRMATIONS_MOCK_CONFIRMATIONS_CLIENT_H_
