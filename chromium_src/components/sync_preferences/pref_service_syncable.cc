/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/sync_preferences/pref_service_syncable.h"

#include "brave/common/rewards_sync_scoped_persistent_pref_store.h"
#include "src/components/sync_preferences/pref_service_syncable.cc"

namespace sync_preferences {
std::unique_ptr<PrefServiceSyncable>
PrefServiceSyncable::CreateScopedPrefService(PrefStore*,
                                             const std::vector<const char*>&) {
  // pref_service_forked_ = true; // ?
  auto pref_notifier = std::make_unique<PrefNotifierImpl>();

  auto user_prefs =
      base::MakeRefCounted<brave::RewardsSyncScopedPersistentPrefStore>(
          user_pref_store_.get());

  auto pref_value_store =
      pref_value_store_->CloneAndSpecialize(nullptr,  // managed_prefs
                                            nullptr,  // supervised_user_prefs
                                            nullptr,  // extension_prefs
                                            nullptr,  // standalone_browser_prefs
                                            nullptr,  // command_line_prefs
                                            user_prefs.get(),
                                            nullptr,  // recommended_prefs
                                            nullptr,  // default_prefs
                                            pref_notifier.get(),
                                            nullptr);  // delegate
  return std::make_unique<PrefServiceSyncable>(
      std::move(pref_notifier), std::move(pref_value_store),
      std::move(user_prefs), pref_registry_, pref_sync_associator_.client(),
      read_error_callback_, false);
}
}  // namespace sync_preferences
