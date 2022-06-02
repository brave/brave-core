/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DATABASE_DATABASE_MANAGER_OBSERVER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DATABASE_DATABASE_MANAGER_OBSERVER_H_

#include "base/observer_list_types.h"

namespace ads {

class DatabaseManagerObserver : public base::CheckedObserver {
 public:
  virtual void OnWillCreateOrOpenDatabase() {}

  virtual void OnDidCreateOrOpenDatabase() {}

  virtual void OnFailedToCreateOrOpenDatabase() {}

  virtual void OnWillMigrateDatabase(const int from_version,
                                     const int to_version) {}

  virtual void OnDidMigrateDatabase(const int from_version,
                                    const int to_version) {}

  virtual void OnFailedToMigrateDatabase(const int from_version,
                                         const int to_version) {}

 protected:
  ~DatabaseManagerObserver() override = default;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DATABASE_DATABASE_MANAGER_OBSERVER_H_
