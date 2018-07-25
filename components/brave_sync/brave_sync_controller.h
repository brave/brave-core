/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef BRAVE_COMPONENTS_SYNC_CONTROLLER_H_
#define BRAVE_COMPONENTS_SYNC_CONTROLLER_H_

#include <string>

class SyncJsLayer;
class SyncUI;

namespace brave_sync {

  struct BraveSyncSettings;
  class SyncDevices;

  class BraveSyncController {
  public:
    virtual ~BraveSyncController(){};
    virtual void OnSetupSyncHaveCode(const std::string &sync_words,
      const std::string &device_name) = 0;

    virtual void OnSetupSyncNewToSync(const std::string &device_name) = 0;

    virtual void OnDeleteDevice(const std::string &device_id) = 0;
    virtual void OnResetSync() = 0;

    virtual void GetSettings(BraveSyncSettings &settings) = 0;
    virtual void GetDevices(SyncDevices &devices) = 0;

    virtual void GetSyncWords() = 0;
    virtual std::string GetSeed() = 0;

    virtual void SetupJsLayer(SyncJsLayer *sync_js_layer) = 0;
    virtual void SetupUi(SyncUI *sync_ui) = 0;
  };

} // namespace brave_sync

#endif //BRAVE_COMPONENTS_SYNC_CONTROLLER_H_
