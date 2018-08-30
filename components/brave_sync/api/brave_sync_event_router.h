/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EXTENSIONS_API_BRAVE_SYNC_BRAVE_SYNC_EVENT_ROUTER_H
#define BRAVE_BROWSER_EXTENSIONS_API_BRAVE_SYNC_BRAVE_SYNC_EVENT_ROUTER_H

#include <string>
#include "extensions/browser/event_router.h"

class Profile;

namespace extensions {
namespace api {
namespace brave_sync {
  struct SyncRecord2;
  struct Config;
  struct RecordAndExistingObject;
} // namespace brave_sync
} // namespace api
} // namespace extensions


namespace brave_sync {
using Uint8Array = std::vector<unsigned char>;
}

namespace extensions {

class BraveSyncEventRouter {
public:
  BraveSyncEventRouter(Profile* profile);
  ~BraveSyncEventRouter();

  void BrowserToBackgroundPage(const std::string &arg1);

  void GotInitData(
    const brave_sync::Uint8Array &seed,
    const brave_sync::Uint8Array &device_id,
    const extensions::api::brave_sync::Config &config);

  void FetchSyncRecords(
    const std::vector<std::string> &category_names,
    const base::Time &startAt,
    const int &max_records);

  void ResolveSyncRecords(const std::string &category_name,
    const std::vector<extensions::api::brave_sync::RecordAndExistingObject>& records_and_existing_objects);

  void SendSyncRecords(const std::string &category_name, const std::vector<api::brave_sync::SyncRecord2>& records);

  void NeedSyncWords(const std::string &seed);

  void NeedBytesFromSyncWords(const std::string &words);

  void LoadClient();

private:
  Profile* profile_;
};

} // namespace extensions

#endif // BRAVE_BROWSER_EXTENSIONS_API_BRAVE_SYNC_BRAVE_SYNC_EVENT_ROUTER_H
