/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SYNC_CLIENT_BRAVE_SYNC_CLIENT_H_
#define BRAVE_COMPONENTS_BRAVE_SYNC_CLIENT_BRAVE_SYNC_CLIENT_H_

#include <string>
#include <vector>
#include <memory>

#include "components/keyed_service/core/keyed_service.h"
#include "brave/components/brave_sync/client/client_data.h"
#include "brave/components/brave_sync/jslib_messages_fwd.h"

namespace base {
class Time;
}

class Profile;

namespace brave_sync {

class SyncMessageHandler {
 public:
  virtual void BackgroundSyncStarted(bool startup) = 0;
  virtual void BackgroundSyncStopped(bool shutdown) = 0;

  // SYNC_DEBUG
  virtual void OnSyncDebug(const std::string &message) = 0;
  // SYNC_SETUP_ERROR
  virtual void OnSyncSetupError(const std::string &error) = 0;
  // GET_INIT_DATA
  virtual void OnGetInitData(const std::string &sync_version) = 0;
  // SAVE_INIT_DATA
  virtual void OnSaveInitData(const Uint8Array& seed,
                              const Uint8Array& device_id,
                              const std::string& device_id_v2) = 0;
  // SYNC_READY
  virtual void OnSyncReady() = 0;
  // GET_EXISTING_OBJECTS
  virtual void OnGetExistingObjects(const std::string &category_name,
      std::unique_ptr<RecordsList> records,
      const base::Time &last_record_time_stamp, const bool is_truncated) = 0;
  // RESOLVED_SYNC_RECORDS
  virtual void OnResolvedSyncRecords(const std::string &category_name,
    std::unique_ptr<RecordsList> records) = 0;
  // DELETED_SYNC_USER
  virtual void OnDeletedSyncUser() = 0;
  // DELETE_SYNC_SITE_SETTINGS
  virtual void OnDeleteSyncSiteSettings() = 0;
  // SAVE_BOOKMARKS_BASE_ORDER
  virtual void OnSaveBookmarksBaseOrder(const std::string& order) = 0;
  // COMOACTED_SYNC_CATEGORY
  virtual void OnCompactComplete(const std::string& category_name) = 0;
  virtual void OnRecordsSent(const std::string &category_name,
                             std::unique_ptr<RecordsList> records) = 0;
};

class BraveSyncClient {
 public:
  static BraveSyncClient* Create(SyncMessageHandler* handler,
                                 Profile* profile);
  virtual ~BraveSyncClient() = default;

  // BraveSync to Browser messages
  virtual SyncMessageHandler* sync_message_handler() = 0;

  virtual void SendGotInitData(const Uint8Array& seed,
                               const Uint8Array& device_id,
                               const client_data::Config& config,
                               const std::string& device_id_v2) = 0;
  virtual void SendFetchSyncRecords(
      const std::vector<std::string> &category_names,
      const base::Time &startAt,
      const int max_records) = 0;
  virtual void SendResolveSyncRecords(
      const std::string &category_name,
      std::unique_ptr<SyncRecordAndExistingList> list) = 0;
  virtual void SendSyncRecords(const std::string &category_name,
    const RecordsList &records) = 0;
  virtual void SendDeleteSyncUser() = 0;
  virtual void SendDeleteSyncCategory(const std::string &category_name) = 0;
  virtual void SendGetBookmarksBaseOrder(const std::string &device_id,
                                         const std::string &platform) = 0;
  // COMPACT_SYNC_CATEGORY
  virtual void SendCompact(const std::string& category_name) = 0;

  virtual void OnExtensionInitialized() = 0;

  virtual void OnSyncEnabledChanged() = 0;
};

}   // namespace brave_sync

#endif    // BRAVE_COMPONENTS_BRAVE_SYNC_CLIENT_BRAVE_SYNC_CLIENT_H_
