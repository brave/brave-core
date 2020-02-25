/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_DATABASE_DATABASE_H_
#define BRAVELEDGER_DATABASE_DATABASE_H_

#include <memory>
#include <string>

#include "bat/ledger/ledger.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_database {

class DatabaseInitialize;
class DatabaseActivityInfo;
class DatabasePublisherInfo;

class Database {
 public:
  explicit Database(bat_ledger::LedgerImpl* ledger);
  ~Database();

  void Initialize(
      const bool execute_create_script,
      ledger::ResultCallback callback);

  /**
   * ACTIVITY INFO
   */
  void SaveActivityInfo(
      ledger::PublisherInfoPtr info,
      ledger::ResultCallback callback);

  void SaveActivityInfoList(
      ledger::PublisherInfoList list,
      ledger::ResultCallback callback);

  void GetActivityInfoList(
      uint32_t start,
      uint32_t limit,
      ledger::ActivityInfoFilterPtr filter,
      ledger::PublisherInfoListCallback callback);

  void DeleteActivityInfo(
      const std::string& publisher_key,
      ledger::ResultCallback callback);

  /**
   * PUBLISHER INFO
   */
  void SavePublisherInfo(
      ledger::PublisherInfoPtr publisher_info,
      ledger::ResultCallback callback);

  void GetPublisherInfo(
      const std::string& publisher_key,
      ledger::PublisherInfoCallback callback);

  void GetPanelPublisherInfo(
      ledger::ActivityInfoFilterPtr filter,
      ledger::PublisherInfoCallback callback);

  void RestorePublishers(ledger::ResultCallback callback);

  void GetExcludedList(ledger::PublisherInfoListCallback callback);

 private:
  std::unique_ptr<DatabaseInitialize> initialize_;
  std::unique_ptr<DatabaseActivityInfo> activity_info_;
  std::unique_ptr<DatabasePublisherInfo> publisher_info_;
  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace braveledger_database

#endif  // BRAVELEDGER_DATABASE_DATABASE_H_
