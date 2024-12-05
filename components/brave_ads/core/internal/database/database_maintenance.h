/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DATABASE_DATABASE_MAINTENANCE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DATABASE_DATABASE_MAINTENANCE_H_

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/core/internal/common/timer/timer.h"
#include "brave/components/brave_ads/core/internal/database/database_manager_observer.h"

namespace base {
class TimeDelta;
}  // namespace base

namespace brave_ads::database {

class Maintenance final : public DatabaseManagerObserver {
 public:
  Maintenance();

  Maintenance(const Maintenance&) = delete;
  Maintenance& operator=(const Maintenance&) = delete;

  ~Maintenance() override;

 private:
  void RepeatedlyScheduleAfter(base::TimeDelta after);
  void RepeatedlyScheduleAfterCallback();

  // DatabaseManagerObserver:
  void OnDatabaseIsReady() override;

  Timer timer_;

  base::WeakPtrFactory<Maintenance> weak_factory_{this};
};

}  // namespace brave_ads::database

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DATABASE_DATABASE_MAINTENANCE_H_
