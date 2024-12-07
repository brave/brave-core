/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DATABASE_DATABASE_MANAGER_OBSERVER_MOCK_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DATABASE_DATABASE_MANAGER_OBSERVER_MOCK_H_

#include "brave/components/brave_ads/core/internal/database/database_manager_observer.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace brave_ads {

class DatabaseManagerObserverMock : public DatabaseManagerObserver {
 public:
  DatabaseManagerObserverMock();

  DatabaseManagerObserverMock(const DatabaseManagerObserverMock&) = delete;
  DatabaseManagerObserverMock& operator=(const DatabaseManagerObserverMock&) =
      delete;

  ~DatabaseManagerObserverMock() override;

  MOCK_METHOD(void, OnWillCreateOrOpenDatabase, ());

  MOCK_METHOD(void, OnDidCreateDatabase, ());

  MOCK_METHOD(void, OnDidOpenDatabase, ());

  MOCK_METHOD(void, OnFailedToCreateOrOpenDatabase, ());

  MOCK_METHOD(void, OnWillMigrateDatabase, (int from_version, int to_version));

  MOCK_METHOD(void, OnDidMigrateDatabase, (int from_version, int to_version));

  MOCK_METHOD(void,
              OnFailedToMigrateDatabase,
              (int from_version, int to_version));

  MOCK_METHOD(void, OnDatabaseIsReady, ());
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DATABASE_DATABASE_MANAGER_OBSERVER_MOCK_H_
