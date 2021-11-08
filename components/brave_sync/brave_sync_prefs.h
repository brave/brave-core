/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SYNC_BRAVE_SYNC_PREFS_H_
#define BRAVE_COMPONENTS_BRAVE_SYNC_BRAVE_SYNC_PREFS_H_

#include <memory>
#include <string>
#include <vector>

#include "base/values.h"

class PrefRegistrySimple;
class PrefService;

namespace base {
class Time;
}

namespace brave_sync {

class Prefs {
 public:
  explicit Prefs(PrefService* pref_service);
  Prefs(const Prefs&) = delete;
  Prefs& operator=(const Prefs&) = delete;
  virtual ~Prefs();

  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

  static std::string GetSeedPath();

  std::string GetSeed() const;
  bool SetSeed(const std::string& seed);

  bool IsSyncV1Migrated() const;
  void SetSyncV1Migrated(bool is_migrated);

  bool IsSyncV1MetaInfoCleared() const;
  void SetSyncV1MetaInfoCleared(bool is_cleared);

  // It is only used to read whether sync v1 was enabled before upgrading
  bool IsSyncV1Enabled() const;
#if defined(OS_ANDROID)
  // Used for android clients to pretend sync v1 was enabled through the prefs.
  // Originally Android used shared preferences
  void SetSyncV1WasEnabled() const;
#endif

  bool IsSyncMigrateNoticeDismissed() const;
  void SetDismissSyncMigrateNotice(bool is_dismissed);

  void Clear();

 private:
  PrefService* const pref_service_;
};

void MigrateBraveSyncPrefs(PrefService* prefs);

}  // namespace brave_sync

#endif  // BRAVE_COMPONENTS_BRAVE_SYNC_BRAVE_SYNC_PREFS_H_
