/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SYNC_BRAVE_SYNC_CLIENT_H
#define BRAVE_COMPONENTS_BRAVE_SYNC_BRAVE_SYNC_CLIENT_H

#include <string>
#include <vector>
#include <memory>

namespace base {
  class Time;

  // Temporary from SyncJsLayer
  class ListValue;
  class Value;
}

// Temporary from SyncJsLayer
class SyncJsLayer;

namespace brave_sync {

typedef std::vector<unsigned char> Uint8Array;

namespace jslib {
  class SyncRecord;
}

typedef std::unique_ptr<jslib::SyncRecord> SyncRecordPtr;
typedef std::vector<SyncRecordPtr> RecordsList;
typedef std::pair<SyncRecordPtr, SyncRecordPtr> SyncRecordAndExisting;
typedef std::vector<SyncRecordAndExisting> SyncRecordAndExistingList;

class SyncLibToBrowserHandler {
public:
  virtual ~SyncLibToBrowserHandler() = default;
  virtual void OnMessageFromSyncReceived() = 0;

  //SYNC_DEBUG
  //SYNC_SETUP_ERROR
  //GET_INIT_DATA
  //SAVE_INIT_DATA
  //SYNC_READY
  //GET_EXISTING_OBJECTS
  //RESOLVED_SYNC_RECORDS
  //DELETED_SYNC_USER
  //DELETE_SYNC_SITE_SETTINGS
  //SAVE_BOOKMARKS_BASE_ORDER
  //SAVE_BOOKMARK_ORDER

  virtual void OnSyncDebug(const std::string &message) = 0;
  virtual void OnSyncSetupError(const std::string &error) = 0;
  virtual void OnGetInitData(const std::string &sync_version) = 0;
  virtual void OnSaveInitData(const Uint8Array &seed, const Uint8Array &device_id) = 0;
  virtual void OnSyncReady() = 0;
  virtual void OnGetExistingObjects(const std::string &category_name,
    const RecordsList &records, const base::Time &last_record_time_stamp) = 0;
  virtual void OnResolvedSyncRecords(const std::string &category_name,
    const RecordsList &records) = 0;
  virtual void OnDeletedSyncUser() = 0;
  virtual void OnDeleteSyncSiteSettings() = 0;
  virtual void OnSaveBookmarksBaseOrder(const std::string &order) = 0;
  virtual void OnSaveBookmarkOrder(const std::string &order,
    const std::string &prev_order, const std::string &next_order) = 0;

  // Temporary from SyncJsLayerResponseReceiver
  virtual void OnJsLibMessage(const std::string &message, const base::ListValue* args) = 0;
};

class BraveSyncClient {
public:
  virtual ~BraveSyncClient() = default;

  // BraveSync to Browser messages
  virtual void SetSyncToBrowserHandler(SyncLibToBrowserHandler *handler) = 0;

  // After this call the library gets loaded and
  // sends SyncLibToBrowserHandler::OnGetInitData and so on
  virtual void LoadClient() = 0;

  // Browser to BraveSync messages
  virtual void SendBrowserToSync(
      const std::string &message,
      const base::Value &arg1,
      const base::Value &arg2,
      const base::Value &arg3,
      const base::Value &arg4) = 0;
  //GOT_INIT_DATA
  //FETCH_SYNC_RECORDS
  //FETCH_SYNC_DEVICES
  //RESOLVE_SYNC_RECORDS
  //SEND_SYNC_RECORDS
  //DELETE_SYNC_USER
  //DELETE_SYNC_CATEGORY
  //GET_BOOKMARKS_BASE_ORDER
  //GET_BOOKMARK_ORDER

  virtual void SendGotInitDataStr(const std::string &seed, const std::string &device_id, const std::string & config) = 0;
  virtual void SendFetchSyncRecords(
    const std::vector<std::string> &category_names, const base::Time &startAt,
    const int &max_records) = 0;
  virtual void SendFetchSyncDevices() = 0;
  virtual void SendResolveSyncRecords(const std::string &category_name,
    const SyncRecordAndExistingList &records_and_existing_objects) = 0;
  virtual void SendSyncRecords(const std::string &category_name,
    const RecordsList &records) = 0;
  virtual void SendDeleteSyncUser() = 0;
  virtual void SendDeleteSyncCategory(const std::string &category_name) = 0;
  virtual void SendGetBookmarksBaseOrder(const std::string &device_id, const std::string &platform) = 0;
  virtual void SendGetBookmarkOrder(const std::string &prevOrder, const std::string &nextOrder) = 0;

  // Temporary from SyncJsLayer
  virtual void SetupJsLayer(SyncJsLayer *sync_js_layer) = 0;
  virtual void RunCommandBV(const std::vector<const base::Value*> &args) = 0;
  virtual void RunCommandStr(const std::string &command,
    const std::string &arg1, const std::string &arg2, const std::string &arg3,
    const std::string &arg4) = 0;
};

} // namespace brave_sync

#endif // BRAVE_COMPONENTS_BRAVE_SYNC_BRAVE_SYNC_CLIENT_H
