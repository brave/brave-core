/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_FEDERATED_LEARNING_DATA_STORES_AD_NOTIFICATION_TIMING_DATA_STORE_H_
#define BRAVE_COMPONENTS_BRAVE_FEDERATED_LEARNING_DATA_STORES_AD_NOTIFICATION_TIMING_DATA_STORE_H_

#include <map>
#include <string>

#include "base/files/file_path.h"
#include "base/time/time.h"
#include "brave/components/brave_federated_learning/data_stores/data_store.h"

namespace brave {

namespace federated_learning {

// Log Definition --------------------------------------------------------
struct AdNotificationTimingTaskLog {
  AdNotificationTimingTaskLog(const int id,
                              const base::Time& time,
                              const std::string& locale,
                              const int number_of_tabs,
                              const bool click,
                              const base::Time& creation_date);
  AdNotificationTimingTaskLog(const AdNotificationTimingTaskLog& other);
  AdNotificationTimingTaskLog();
  ~AdNotificationTimingTaskLog();

  int id;
  const base::Time time;
  std::string locale;
  int number_of_tabs;
  bool click;
  const base::Time creation_date;
};

class AdNotificationTimingDataStore final : public DataStore {
 public:
  explicit AdNotificationTimingDataStore(const base::FilePath& database_path);
  ~AdNotificationTimingDataStore() final;

  AdNotificationTimingDataStore(const AdNotificationTimingDataStore&) = delete;
  AdNotificationTimingDataStore& operator=(
      const AdNotificationTimingDataStore&) = delete;

  typedef std::map<int, AdNotificationTimingTaskLog>
      IdToAdNotificationTimingTaskLogMap;

  bool Init(const std::string& task_id,
            const std::string& task_name,
            const int max_number_of_records,
            const int max_retention_days);
  void EnforceRetentionPolicy();

  using DataStore::DeleteLogs;

  bool AddLog(const AdNotificationTimingTaskLog& log);
  void LoadLogs(IdToAdNotificationTimingTaskLogMap* notification_timing_logs);
  bool EnsureTable() override;
};

}  // namespace federated_learning

}  // namespace brave

#endif  // BRAVE_COMPONENTS_BRAVE_FEDERATED_LEARNING_DATA_STORES_AD_NOTIFICATION_TIMING_DATA_STORE_H_
