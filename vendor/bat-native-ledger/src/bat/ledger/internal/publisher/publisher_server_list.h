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

using SharedServerPublisherPartial =
    std::shared_ptr<std::vector<ledger::ServerPublisherPartial>>;
using SharedPublisherBanner =
    std::shared_ptr<std::vector<ledger::PublisherBanner>>;

class PublisherServerList {
 public:
  explicit PublisherServerList(bat_ledger::LedgerImpl* ledger);
  ~PublisherServerList();

  void Start(ledger::ResultCallback callback);

  // Called when timer is triggered
  void OnTimer(uint32_t timer_id);

  void SetTimer(bool retry_after_error);

  void ClearTimer();

 private:
  void Download(ledger::ResultCallback callback);

  void OnDownload(
      int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers,
      ledger::ResultCallback callback);

  void OnParsePublisherList(
      const ledger::Result result,
      ledger::ResultCallback callback);

  uint64_t GetTimerTime(
      bool retry_after_error,
      const uint64_t last_download);

  ledger::PublisherStatus ParsePublisherStatus(const std::string& status);

  void ParsePublisherList(
      const std::string& data,
      ledger::ResultCallback callback);

  void ParsePublisherBanner(
      ledger::PublisherBanner* banner,
      base::Value* dictionary);

  void SaveParsedData(
      const ledger::Result result,
      const SharedServerPublisherPartial& list_publisher,
      const SharedPublisherBanner& list_banner,
      ledger::ResultCallback callback);

  void SaveBanners(
      const ledger::Result result,
      const SharedPublisherBanner& list_banner,
      ledger::ResultCallback callback);

  void BannerSaved(
      const ledger::Result result,
      ledger::ResultCallback callback);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
  uint32_t server_list_timer_id_;
  bool in_progress_ = false;
  uint32_t current_page_ = 1;
};

}  // namespace braveledger_publisher
#endif  // BRAVELEDGER_PUBLISHER_PUBLISHER_SERVER_LIST_H_
