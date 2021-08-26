/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SYNC_DRIVER_BRAVE_SYNC_SERVICE_IMPL_H_
#define BRAVE_COMPONENTS_SYNC_DRIVER_BRAVE_SYNC_SERVICE_IMPL_H_

#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_sync/brave_sync_prefs.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/sync/driver/sync_service_impl.h"

class Profile;

namespace syncer {

class BraveSyncAuthManager;
class SyncServiceImplDelegate;

class BraveSyncServiceImpl : public SyncServiceImpl {
 public:
  explicit BraveSyncServiceImpl(
      InitParams init_params,
      std::unique_ptr<SyncServiceImplDelegate> sync_service_impl_delegate);
  ~BraveSyncServiceImpl() override;

  // SyncServiceImpl implementation
  bool IsSetupInProgress() const override;

  std::string GetOrCreateSyncCode();
  bool SetSyncCode(const std::string& sync_code);

  // This should only be called by helper function, brave_sync::ResetSync, or by
  // OnDeviceInfoChange internally
  void OnSelfDeviceInfoDeleted(base::OnceClosure cb);

  // These functions are for disabling device_info_observer_ from firing
  // when the device is doing own reset sync operation, to prevent early call
  // of StopAndClear prior to device sends delete record
  void SuspendDeviceObserverForOwnReset();
  void ResumeDeviceObserver();

  void Initialize() override;

 private:
  BraveSyncAuthManager* GetBraveSyncAuthManager();

  void OnBraveSyncPrefsChanged(const std::string& path);

  brave_sync::Prefs brave_sync_prefs_;

  PrefChangeRegistrar brave_sync_prefs_change_registrar_;

  std::unique_ptr<SyncServiceImplDelegate> sync_service_impl_delegate_;

  base::WeakPtrFactory<BraveSyncServiceImpl> weak_ptr_factory_;

  BraveSyncServiceImpl(const BraveSyncServiceImpl&) = delete;
  BraveSyncServiceImpl& operator=(const BraveSyncServiceImpl&) = delete;
};

}  // namespace syncer

#endif  // BRAVE_COMPONENTS_SYNC_DRIVER_BRAVE_SYNC_SERVICE_IMPL_H_
