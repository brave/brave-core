/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DATABASE_DATABASE_MANAGER_OBSERVER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DATABASE_DATABASE_MANAGER_OBSERVER_H_

#include "base/observer_list_types.h"

namespace ads {

class DatabaseManagerObserver : public base::CheckedObserver {
 public:
  // Invoked when about to create or open the database.
  virtual void OnWillCreateOrOpenDatabase() {}

  // Invoked when the database was created or opened.
  virtual void OnDidCreateOrOpenDatabase() {}

  // Invoked when the database cannot be created or opened.
  virtual void OnFailedToCreateOrOpenDatabase() {}

  // Invoked when about to migrate the database from |from_version| to
  // |to_version|.
  virtual void OnWillMigrateDatabase(const int from_version,
                                     const int to_version) {}

  // Invoked when the database was migrated from |from_version| to |to_version|.
  virtual void OnDidMigrateDatabase(const int from_version,
                                    const int to_version) {}

  // Invoked when the database cannot be migrated from |from_version| to
  // |to_version|.
  virtual void OnFailedToMigrateDatabase(const int from_version,
                                         const int to_version) {}

  // Invoked when the database is ready to be queried.
  virtual void OnDatabaseIsReady() {}
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DATABASE_DATABASE_MANAGER_OBSERVER_H_
