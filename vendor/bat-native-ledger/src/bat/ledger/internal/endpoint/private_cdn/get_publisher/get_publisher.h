/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_ENDPOINT_PRIVATE_CDN_GET_PUBLISHER_GET_PUBLISHER_H_
#define BRAVELEDGER_ENDPOINT_PRIVATE_CDN_GET_PUBLISHER_GET_PUBLISHER_H_

#include <string>

#include "bat/ledger/ledger.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace ledger {
namespace endpoint {
namespace private_cdn {

using GetPublisherCallback = std::function<void(
    const ledger::Result result,
    ledger::ServerPublisherInfoPtr info)>;

class GetPublisher {
 public:
  explicit GetPublisher(bat_ledger::LedgerImpl* ledger);
  ~GetPublisher();

  void Request(
      const std::string& publisher_key,
      const std::string& hash_prefix,
      GetPublisherCallback callback);

 private:
  std::string GetUrl(const std::string& hash_prefix);

  ledger::Result CheckStatusCode(const int status_code);

  ledger::Result ParseBody(
      const std::string& body,
      const std::string& publisher_key,
      ledger::ServerPublisherInfo* info);

  void OnRequest(
      const ledger::UrlResponse& response,
      const std::string& publisher_key,
      GetPublisherCallback callback);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace private_cdn
}  // namespace endpoint
}  // namespace ledger

#endif  // BRAVELEDGER_ENDPOINT_PRIVATE_CDN_GET_PUBLISHER_GET_PUBLISHER_H_
