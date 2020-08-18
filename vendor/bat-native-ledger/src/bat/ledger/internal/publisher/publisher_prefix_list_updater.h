/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_PUBLISHER_PUBLISHER_PREFIX_LIST_UPDATER_H_
#define BRAVELEDGER_PUBLISHER_PUBLISHER_PREFIX_LIST_UPDATER_H_

#include <functional>
#include <map>
#include <memory>
#include <string>

#include "base/time/time.h"
#include "base/timer/timer.h"
#include "bat/ledger/internal/endpoint/rewards/rewards_server.h"
#include "bat/ledger/ledger.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_publisher {

// Automatically updates the publisher prefix list store on regular
// intervals.
class PublisherPrefixListUpdater {
 public:
  explicit PublisherPrefixListUpdater(bat_ledger::LedgerImpl* ledger);

  PublisherPrefixListUpdater(const PublisherPrefixListUpdater&) = delete;
  PublisherPrefixListUpdater& operator=(
      const PublisherPrefixListUpdater&) = delete;

  ~PublisherPrefixListUpdater();

  // Starts the auto updater
  void StartAutoUpdate(ledger::PublisherPrefixListUpdatedCallback callback);

  // Cancels the auto updater
  void StopAutoUpdate();

 private:
  void StartFetchTimer(
      const base::Location& posted_from,
      base::TimeDelta delay);

  void OnFetchTimerElapsed();
  void OnFetchCompleted(
      const ledger::Result result,
      const std::string& body);
  void OnPrefixListInserted(const ledger::Result result);

  base::TimeDelta GetAutoUpdateDelay();
  base::TimeDelta GetRetryAfterFailureDelay();

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
  base::OneShotTimer timer_;
  bool auto_update_ = false;
  int retry_count_ = 0;
  ledger::PublisherPrefixListUpdatedCallback on_updated_callback_;
  std::unique_ptr<ledger::endpoint::RewardsServer> rewrads_server_;
};

}  // namespace braveledger_publisher

#endif  // BRAVELEDGER_PUBLISHER_PUBLISHER_PREFIX_LIST_UPDATER_H_
