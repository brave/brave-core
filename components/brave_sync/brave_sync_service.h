/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef BRAVE_COMPONENTS_SYNC_BRAVE_SYNC_SERVICE_H_
#define BRAVE_COMPONENTS_SYNC_BRAVE_SYNC_SERVICE_H_

#include <string>

#include "base/macros.h"
#include "base/observer_list.h"
#include "components/bookmarks/browser/bookmark_storage.h"
#include "extensions/buildflags/buildflags.h"

class Profile;

namespace brave_sync {

class BraveSyncClient;
class BraveSyncServiceObserver;
class Settings;
class SyncDevices;

bookmarks::BookmarkPermanentNodeList
LoadExtraNodes(bookmarks::LoadExtraCallback callback, int64_t* next_node_id);
bool IsSyncManagedNode(const bookmarks::BookmarkPermanentNode* node);

class BraveSyncService {
 public:
  BraveSyncService();
  virtual ~BraveSyncService();

  typedef base::Callback<void(std::unique_ptr<brave_sync::Settings>,
                              std::unique_ptr<brave_sync::SyncDevices>)>
      GetSettingsAndDevicesCallback;

  virtual void OnSetupSyncHaveCode(
      const std::string& sync_words,
      const std::string& device_name) = 0;
  virtual void OnSetupSyncNewToSync(const std::string& device_name) = 0;
  virtual void OnDeleteDevice(const std::string& device_id) = 0;
  virtual void OnResetSync() = 0;
  virtual void GetSettingsAndDevices(
      const GetSettingsAndDevicesCallback& callback) = 0;

  virtual void GetSyncWords() = 0;
  virtual std::string GetSeed() = 0;

  virtual void OnSetSyncEnabled(const bool enabled) = 0;
  virtual void OnSetSyncBookmarks(const bool sync_bookmarks) = 0;
  virtual void OnSetSyncBrowsingHistory(const bool sync_browsing_history) = 0;
  virtual void OnSetSyncSavedSiteSettings(
      const bool sync_saved_site_settings) = 0;

  void AddObserver(BraveSyncServiceObserver* observer);
  void RemoveObserver(BraveSyncServiceObserver* observer);

  static bool is_enabled();

#if BUILDFLAG(ENABLE_EXTENSIONS)
  virtual BraveSyncClient* GetBraveSyncClient() = 0;
#endif


 protected:
  base::ObserverList<BraveSyncServiceObserver> observers_;

 private:
  DISALLOW_COPY_AND_ASSIGN(BraveSyncService);
};

}  // namespace brave_sync

#endif  // BRAVE_COMPONENTS_SYNC_BRAVE_SYNC_SERVICE_H_
