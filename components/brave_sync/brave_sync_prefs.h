/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SYNC_BRAVE_SYNC_PREFS_H_
#define BRAVE_COMPONENTS_BRAVE_SYNC_BRAVE_SYNC_PREFS_H_

#include <memory>
#include <string>
#include <vector>

#include "base/memory/raw_ref.h"
#include "base/values.h"
#include "build/build_config.h"

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

  static void RegisterProfilePrefsForMigration(PrefRegistrySimple* registry);

  static std::string GetSeedPath();

  std::string GetSeed(bool* failed_to_decrypt) const;
  bool SetSeed(const std::string& seed);

  bool IsSyncAccountDeletedNoticePending() const;
  void SetSyncAccountDeletedNoticePending(bool is_pending);

  bool IsFailedDecryptSeedNoticeDismissed() const;
  void DismissFailedDecryptSeedNotice();

  void AddLeaveChainDetail(const char* file, int line, const char* func);
  void ClearLeaveChainDetails();

  void Clear();

 private:
  const raw_ref<PrefService> pref_service_;
};

void MigrateBraveSyncPrefs(PrefService* prefs);

}  // namespace brave_sync

#endif  // BRAVE_COMPONENTS_BRAVE_SYNC_BRAVE_SYNC_PREFS_H_
