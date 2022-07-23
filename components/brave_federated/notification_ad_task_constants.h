// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_FEDERATED_NOTIFICATION_AD_TASK_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_FEDERATED_NOTIFICATION_AD_TASK_CONSTANTS_H_

namespace brave_federated {

constexpr char kNotificationAdTaskName[] = "notification_ad_timing_task";
constexpr int kNotificationAdTaskId = 0;

constexpr int kMaxEvents = 200;
constexpr int kFeaturesPerEvent = 30;
constexpr int kMaxNumberOfRecords = kMaxEvents * kFeaturesPerEvent;
constexpr base::TimeDelta kMaxRetentionDays = base::Days(30);

}  // namespace brave_federated

#endif  // BRAVE_COMPONENTS_BRAVE_FEDERATED_NOTIFICATION_AD_TASK_CONSTANTS_H_
