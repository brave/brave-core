/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_ENDPOINT_PRIVATE_CDN_GET_PUBLISHER_GET_PUBLISHER_H_
#define BRAVELEDGER_ENDPOINT_PRIVATE_CDN_GET_PUBLISHER_GET_PUBLISHER_H_

#include <string>

#include "bat/ledger/ledger.h"

// GET /publishers/prefixes/{prefix}
//
// Success code:
// HTTP_OK (200)
//
// Error codes:
// HTTP_NOT_FOUND (404)
//
// Response body:
// See https://github.com/brave/brave-core/blob/master/vendor/bat-native-ledger/src/bat/ledger/internal/publisher/protos/channel_response.proto

namespace ledger {
class LedgerImpl;

namespace endpoint {
namespace private_cdn {

using GetPublisherCallback = std::function<void(
    const type::Result result,
    type::ServerPublisherInfoPtr info)>;

class GetPublisher {
 public:
  explicit GetPublisher(LedgerImpl* ledger);
  ~GetPublisher();

  void Request(
      const std::string& publisher_key,
      const std::string& hash_prefix,
      GetPublisherCallback callback);

 private:
  std::string GetUrl(const std::string& hash_prefix);

  type::Result CheckStatusCode(const int status_code);

  type::Result ParseBody(
      const std::string& body,
      const std::string& publisher_key,
      type::ServerPublisherInfo* info);

  void OnRequest(
      const type::UrlResponse& response,
      const std::string& publisher_key,
      GetPublisherCallback callback);

  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace private_cdn
}  // namespace endpoint
}  // namespace ledger

#endif  // BRAVELEDGER_ENDPOINT_PRIVATE_CDN_GET_PUBLISHER_GET_PUBLISHER_H_
