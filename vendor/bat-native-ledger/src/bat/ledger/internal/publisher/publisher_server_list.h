/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_PUBLISHER_PUBLISHER_SERVER_LIST_H_
#define BRAVELEDGER_PUBLISHER_PUBLISHER_SERVER_LIST_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/values.h"
#include "bat/ledger/ledger.h"
#include "bat/ledger/internal/publisher/publisher.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_publisher {

class PublisherServerList {
 public:
  explicit PublisherServerList(
      bat_ledger::LedgerImpl* ledger,
      Publisher* publisher);
  ~PublisherServerList();

  void Download(DownloadServerPublisherListCallback callback);

  // Called when timer is triggered
  void OnTimer(uint32_t timer_id);

  void SetTimer(bool retry_after_error);

 private:
  void OnDownload(
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers,
    DownloadServerPublisherListCallback callback);

  void OnParsePublisherList(
    const ledger::Result result,
    DownloadServerPublisherListCallback callback);

  uint64_t GetTimerTime(
    bool retry_after_error,
    const uint64_t last_download);

  void ParsePublisherList(
    const std::string& data,
    ParsePublisherListCallback callback);

  ledger::PublisherBannerPtr ParsePublisherBanner(
      base::DictionaryValue* dictionary);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
  Publisher* publisher_;  // NOT OWNED
  uint32_t server_list_timer_id_;
};

}  // namespace braveledger_publisher
#endif  // BRAVELEDGER_PUBLISHER_PUBLISHER_SERVER_LIST_H_
