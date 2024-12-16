/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_SERVICE_SYNC_SERVICE_IMPL_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_SERVICE_SYNC_SERVICE_IMPL_H_

#include "base/gtest_prod_util.h"
#include "components/signin/public/identity_manager/identity_manager.h"

namespace base {
class CommandLine;
}

#define BRAVE_SYNC_SERVICE_IMPL_H_                                             \
 protected:                                                                    \
  static GURL BraveGetSyncServiceURL(const base::CommandLine& command_line,    \
                                     version_info::Channel channel,            \
                                     PrefService* prefs);                      \
                                                                               \
 private:                                                                      \
  friend class BraveSyncServiceImpl;                                           \
  friend class BraveSyncServiceImplTest;                                       \
  FRIEND_TEST_ALL_PREFIXES(BraveSyncServiceImplTest, OnSelfDeviceInfoDeleted); \
  FRIEND_TEST_ALL_PREFIXES(BraveSyncServiceImplTest,                           \
                           PermanentlyDeleteAccount);                          \
  FRIEND_TEST_ALL_PREFIXES(BraveSyncServiceImplTest,                           \
                           OnAccountDeleted_FailureAndRetry);                  \
  FRIEND_TEST_ALL_PREFIXES(BraveSyncServiceImplTest, JoinDeletedChain);        \
  FRIEND_TEST_ALL_PREFIXES(BraveSyncServiceImplTest,                           \
                           ForcedSetDecryptionPassphrase);

// Forcing this include before define virtual to avoid error of
// "duplicate 'virtual' declaration specifier" at SyncEngine's
// 'virtual void Initialize(InitParams params) = 0'
// This also resolves confusion with existing 'Initialize' methods in
//   third_party/protobuf/src/google/protobuf/map_type_handler.h,
//   third_party/protobuf/src/google/protobuf/map_entry_lite.h
#include "components/sync/engine/sync_engine.h"
#define Initialize virtual Initialize
#define ResetEngine virtual ResetEngine
#define StopAndClear virtual StopAndClear

#include "src/components/sync/service/sync_service_impl.h"  // IWYU pragma: export

#undef ResetEngine
#undef Initialize
#undef StopAndClear

#undef BRAVE_SYNC_SERVICE_IMPL_H_

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_SERVICE_SYNC_SERVICE_IMPL_H_
