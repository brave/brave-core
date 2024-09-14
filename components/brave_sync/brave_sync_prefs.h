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

inline constexpr char kCustomSyncServiceUrl[] = "brave_sync.sync_service_url";

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

  enum class AddLeaveChainDetailBehaviour { kAdd, kIgnore };
  void AddLeaveChainDetail(const char* file, int line, const char* func);
  std::string GetLeaveChainDetails() const;
  void ClearLeaveChainDetails();
  static size_t GetLeaveChainDetailsMaxLenForTests();
  static std::string GetLeaveChainDetailsPathForTests();
  void SetAddLeaveChainDetailBehaviourForTests(
      AddLeaveChainDetailBehaviour add_leave_chain_detail_behaviour);

  void Clear();

 private:
  const raw_ref<PrefService> pref_service_;
  AddLeaveChainDetailBehaviour add_leave_chain_detail_behaviour_;
};

void MigrateBraveSyncPrefs(PrefService* prefs);

}  // namespace brave_sync

#endif  // BRAVE_COMPONENTS_BRAVE_SYNC_BRAVE_SYNC_PREFS_H_
