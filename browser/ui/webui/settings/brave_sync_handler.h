// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_SYNC_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_SYNC_HANDLER_H_

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "base/values.h"
#include "chrome/browser/ui/webui/settings/settings_page_ui_handler.h"
#include "components/sync/engine/sync_protocol_error.h"
#include "components/sync_device_info/device_info_tracker.h"

namespace syncer {
class DeviceInfoTracker;
class LocalDeviceInfoProvider;
class BraveSyncServiceImpl;
}  // namespace syncer
class Profile;

class BraveSyncHandler : public settings::SettingsPageUIHandler,
                         public syncer::DeviceInfoTracker::Observer {
 public:
  BraveSyncHandler();
  BraveSyncHandler(const BraveSyncHandler&) = delete;
  BraveSyncHandler& operator=(const BraveSyncHandler&) = delete;
  ~BraveSyncHandler() override;

  // syncer::DeviceInfoTracker::Observer
  void OnDeviceInfoChange() override;

 private:
  // SettingsPageUIHandler overrides:
  void RegisterMessages() override;
  void OnJavascriptAllowed() override;
  void OnJavascriptDisallowed() override;

  // Custom message handlers:
  void HandleGetDeviceList(const base::Value::List& args);
  void HandleGetSyncCode(const base::Value::List& args);
  void HandleGetPureSyncCode(const base::Value::List& args);
  void HandleSetSyncCode(const base::Value::List& args);
  void HandleGetQRCode(const base::Value::List& args);
  void HandleReset(const base::Value::List& args);
  void HandleDeleteDevice(const base::Value::List& args);
  void HandlePermanentlyDeleteAccount(const base::Value::List& args);
  void HandleSyncGetWordsCount(const base::Value::List& args);

  void OnResetDone(base::Value callback_id);
  void OnAccountPermanentlyDeleted(base::Value callback_id,
                                   const syncer::SyncProtocolError& spe);

  void OnJoinChainResult(base::Value callback_id, bool result);

  base::Value::List GetSyncDeviceList();
  syncer::BraveSyncServiceImpl* GetSyncService() const;
  syncer::DeviceInfoTracker* GetDeviceInfoTracker() const;
  syncer::LocalDeviceInfoProvider* GetLocalDeviceInfoProvider() const;

  raw_ptr<Profile> profile_ = nullptr;

  // Manages observer lifetimes.
  base::ScopedObservation<syncer::DeviceInfoTracker,
                          syncer::DeviceInfoTracker::Observer>
      device_info_tracker_observer_{this};

  base::WeakPtrFactory<BraveSyncHandler> weak_ptr_factory_;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_SYNC_HANDLER_H_
