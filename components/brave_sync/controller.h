/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef BRAVE_COMPONENTS_SYNC_CONTROLLER_H_
#define BRAVE_COMPONENTS_SYNC_CONTROLLER_H_

#include <string>

#include "components/keyed_service/core/keyed_service.h"

class SyncJsLayer;
class SyncUI;

class Profile;

namespace brave_sync {

  class Settings;
  class SyncDevices;

  class Controller : public KeyedService {
  public:
    ~Controller() override = default;
    virtual void OnSetupSyncHaveCode(const std::string &sync_words,
      const std::string &device_name) = 0;

    virtual void OnSetupSyncNewToSync(const std::string &device_name) = 0;

    virtual void OnDeleteDevice(const std::string &device_id) = 0;
    virtual void OnResetSync() = 0;

    typedef base::Callback<void(std::unique_ptr<brave_sync::Settings>, std::unique_ptr<brave_sync::SyncDevices>)> GetSettingsAndDevicesCallback;
    virtual void GetSettingsAndDevices(const GetSettingsAndDevicesCallback &callback) = 0;

    virtual void GetSyncWords() = 0;
    virtual std::string GetSeed() = 0;

    virtual void SetupUi(SyncUI *sync_ui) = 0;
  };

} // namespace brave_sync

#endif //BRAVE_COMPONENTS_SYNC_CONTROLLER_H_
