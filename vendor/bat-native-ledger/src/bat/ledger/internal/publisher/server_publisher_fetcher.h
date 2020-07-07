/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_PUBLISHER_SERVER_PUBLISHER_FETCHER_H_
#define BRAVELEDGER_PUBLISHER_SERVER_PUBLISHER_FETCHER_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "bat/ledger/ledger.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_publisher {

using FetchCallbackVector =
    std::vector<ledger::GetServerPublisherInfoCallback>;

// Fetches server publisher info and provides methods for determining
// whether a server publisher info record is expired
class ServerPublisherFetcher {
 public:
  explicit ServerPublisherFetcher(bat_ledger::LedgerImpl* ledger);

  ServerPublisherFetcher(const ServerPublisherFetcher&) = delete;
  ServerPublisherFetcher& operator=(const ServerPublisherFetcher&) = delete;

  ~ServerPublisherFetcher();

  // Returns a value indicating whether a server info record with
  // the specified last update time is expired
  bool IsExpired(ledger::ServerPublisherInfo* server_info);

  // Fetches server publisher info for the specified publisher key
  void Fetch(
      const std::string& publisher_key,
      ledger::GetServerPublisherInfoCallback callback);

  // Purges expired records from the backing database
  void PurgeExpiredRecords();

 private:
  void OnFetchCompleted(
      const std::string& publisher_key,
      const ledger::UrlResponse& response);

  ledger::ServerPublisherInfoPtr ParseResponse(
      const std::string& publisher_key,
      int response_status_code,
      const std::string& response);

  ledger::ServerPublisherInfoPtr GetServerInfoForEmptyResponse(
      const std::string& publisher_key);

  FetchCallbackVector GetCallbacks(const std::string& publisher_key);

  void RunCallbacks(
      const std::string& publisher_key,
      ledger::ServerPublisherInfoPtr server_info);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
  std::map<std::string, FetchCallbackVector> callback_map_;
};

}  // namespace braveledger_publisher

#endif  // BRAVELEDGER_PUBLISHER_SERVER_PUBLISHER_FETCHER_H_
