// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_SYNC_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_SYNC_HANDLER_H_

#include "base/scoped_observer.h"
#include "base/values.h"
#include "chrome/browser/ui/webui/settings/settings_page_ui_handler.h"
#include "components/sync_device_info/device_info_tracker.h"

class Profile;

class BraveSyncHandler : public settings::SettingsPageUIHandler,
                         public syncer::DeviceInfoTracker::Observer {
 public:
  BraveSyncHandler();
  ~BraveSyncHandler() override;

  // syncer::DeviceInfoTracker::Observer
  void OnDeviceInfoChange() override;

 private:
  // SettingsPageUIHandler overrides:
  void RegisterMessages() override;
  void OnJavascriptAllowed() override;
  void OnJavascriptDisallowed() override;

  // Custom message handlers:
  void HandleGetDeviceList(const base::ListValue* args);
  void HandleGetSyncCode(const base::ListValue* args);
  void HandleSetSyncCode(const base::ListValue* args);
  void HandleGetQRCode(const base::ListValue* args);
  void HandleReset(const base::ListValue* args);

  base::Value GetSyncDeviceList();

  Profile* profile_ = nullptr;

  // Manages observer lifetimes.
  ScopedObserver<syncer::DeviceInfoTracker, syncer::DeviceInfoTracker::Observer>
      device_info_tracker_observer_{this};

  DISALLOW_COPY_AND_ASSIGN(BraveSyncHandler);
};

#endif  // BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_SYNC_HANDLER_H_
