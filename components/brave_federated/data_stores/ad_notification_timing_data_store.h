/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_FEDERATED_DATA_STORES_AD_NOTIFICATION_TIMING_DATA_STORE_H_
#define BRAVE_COMPONENTS_BRAVE_FEDERATED_DATA_STORES_AD_NOTIFICATION_TIMING_DATA_STORE_H_

#include <string>

#include "base/containers/flat_map.h"
#include "base/files/file_path.h"
#include "base/sequence_checker.h"
#include "base/time/time.h"
#include "brave/components/brave_federated/data_stores/data_store.h"

namespace brave_federated {

// Log Definition --------------------------------------------------------
struct AdNotificationTimingTaskLog {
  AdNotificationTimingTaskLog(int id,
                              const base::Time& time,
                              const std::string& locale,
                              int number_of_tabs,
                              bool label,
                              const base::Time& creation_date);
  AdNotificationTimingTaskLog(const AdNotificationTimingTaskLog& other);
  AdNotificationTimingTaskLog();
  ~AdNotificationTimingTaskLog();

  int id;
  base::Time time;
  std::string locale;
  int number_of_tabs;
  bool label;
  base::Time creation_date;
};

// AdNotificationTimingDataStore stores logs for the ad notification timing
// prediction task. The logs are composed of the following features:
// 1. time: time that the notification has been delivered to the user.
// 2. locale: user's locale.
// 3. number_of_tabs: the number of tabs open at the time the notification
//    has been delivered.
// 4. label: 1 if user has clicked on the notification, 0 if the user has
//    ignored or actively dismissed the notification
// 5. creation_date: the date of the log creation, useful to enforce
//    retention policies on the stored logs.
class AdNotificationTimingDataStore final : public DataStore {
 public:
  explicit AdNotificationTimingDataStore(const base::FilePath& database_path);
  ~AdNotificationTimingDataStore() final;

  AdNotificationTimingDataStore(const AdNotificationTimingDataStore&) = delete;
  AdNotificationTimingDataStore& operator=(
      const AdNotificationTimingDataStore&) = delete;

  typedef base::flat_map<int, AdNotificationTimingTaskLog>
      IdToAdNotificationTimingTaskLogMap;

  bool Init(int task_id,
            const std::string& task_name,
            int max_number_of_records,
            int max_retention_days);
  void EnforceRetentionPolicy();

  using DataStore::DeleteLogs;

  bool AddLog(const AdNotificationTimingTaskLog& log);
  IdToAdNotificationTimingTaskLogMap LoadLogs();
  bool EnsureTable() override;

 private:
  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace brave_federated

#endif  // BRAVE_COMPONENTS_BRAVE_FEDERATED_DATA_STORES_AD_NOTIFICATION_TIMING_DATA_STORE_H_
