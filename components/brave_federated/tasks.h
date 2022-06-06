// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_FEDERATED_TASKS_H_
#define BRAVE_COMPONENTS_BRAVE_FEDERATED_TASKS_H_

namespace brave_federated {

constexpr char kAdNotificationTaskName[] = "ad_notification_timing_task";
constexpr int kAdNotificationTaskId = 0;

constexpr int kMaxNumberOfRecords = 50;
constexpr int kMaxRetentionDays = 30;

} // namespace brave_federated

#endif // BRAVE_COMPONENTS_BRAVE_FEDERATED_TASKS_H_