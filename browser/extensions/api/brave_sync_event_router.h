/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EXTENSIONS_API_BRAVE_SYNC_EVENT_ROUTER_H_
#define BRAVE_BROWSER_EXTENSIONS_API_BRAVE_SYNC_EVENT_ROUTER_H_

#include <string>
#include <vector>
#include "extensions/browser/event_router.h"

class Profile;

namespace extensions {
namespace api {
namespace brave_sync {
struct SyncRecord;
struct Config;
struct RecordAndExistingObject;
}  // namespace brave_sync
}  // namespace api
}  // namespace extensions

using extensions::api::brave_sync::RecordAndExistingObject;

namespace brave_sync {
using Uint8Array = std::vector<unsigned char>;
}

namespace extensions {

class BraveSyncEventRouter {
 public:
  explicit BraveSyncEventRouter(Profile* profile);
  ~BraveSyncEventRouter();

  void GotInitData(const brave_sync::Uint8Array& seed,
                   const brave_sync::Uint8Array& device_id,
                   const extensions::api::brave_sync::Config& config,
                   const std::string& device_id_v2);

  void FetchSyncRecords(
    const std::vector<std::string>& category_names,
    const base::Time& startAt,
    const int max_records);

  void ResolveSyncRecords(const std::string &category_name,
    const std::vector<RecordAndExistingObject>& records_and_existing_objects);

  void SendSyncRecords(const std::string& category_name,
                       const std::vector<api::brave_sync::SyncRecord>& records);

  void SendGetBookmarksBaseOrder(const std::string& device_id,
                                 const std::string& platform);

  void SendCompact(const std::string& category_name);

  void LoadClient();

 private:
  EventRouter* event_router_;
};

}  // namespace extensions

#endif  // BRAVE_BROWSER_EXTENSIONS_API_BRAVE_SYNC_EVENT_ROUTER_H_
