/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SYNC_BRAVE_SYNC_SERVICE_IMPL_H_
#define BRAVE_COMPONENTS_SYNC_BRAVE_SYNC_SERVICE_IMPL_H_

#include <map>

#include "base/macros.h"
#include "base/scoped_observer.h"
#include "base/sequence_checker.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_sync/brave_sync_service.h"
#include "brave/components/brave_sync/client/brave_sync_client.h"
#include "brave/components/brave_sync/can_send_history.h"
#include "components/bookmarks/browser/bookmark_model_observer.h"
#include "components/bookmarks/browser/bookmark_node_data.h"
#include "extensions/browser/extension_registry_observer.h"

namespace base {
  class RepeatingTimer;
  class SequencedTaskRunner;
}

namespace bookmarks {
  class BookmarkNode;
}

namespace extensions {
class ExtensionRegistry;
}

using extensions::ExtensionRegistryObserver;
using extensions::ExtensionRegistry;

namespace history {
  class URLResult;
}

namespace brave_sync {

namespace storage {
  class ObjectMap;
}

namespace prefs {
  class Prefs;
}

class SyncDevices;
class BraveSyncService;
class Settings;
class History;
class InitialBookmarkNodeInfo;

class BraveSyncServiceImpl : public BraveSyncService,
                             public SyncLibToBrowserHandler,
                             public CanSendSyncHistory,
                             public bookmarks::BookmarkModelObserver,
                             public ExtensionRegistryObserver {
 public:
  BraveSyncServiceImpl(Profile *profile);
  ~BraveSyncServiceImpl() override;

  // KeyedService overrides
  void Shutdown() override;

  // BraveSyncService messages from UI
  void OnSetupSyncHaveCode(const std::string &sync_words,
    const std::string &device_name) override;
  void OnSetupSyncNewToSync(const std::string &device_name) override;
  void OnDeleteDevice(const std::string &device_id) override;
  void OnResetSync() override;
  void GetSyncWords() override;
  std::string GetSeed() override;
  void OnSetSyncThisDevice(const bool &sync_this_device) override;
  void OnSetSyncBookmarks(const bool &sync_bookmarks) override;
  void OnSetSyncBrowsingHistory(const bool &sync_browsing_history) override;
  void OnSetSyncSavedSiteSettings(const bool &sync_saved_site_settings) override;

  void GetSettingsAndDevices(const GetSettingsAndDevicesCallback &callback) override;
  void GetSettingsAndDevicesImpl(std::unique_ptr<brave_sync::Settings> settings, const GetSettingsAndDevicesCallback &callback);

  bool IsSyncConfigured();
  bool IsSyncInitialized();

 private:
  bookmarks::BookmarkNode* GetDeletedNodeRoot();
  // bookmarks::BookmarkModelObserver implementation
  void BookmarkModelLoaded(bookmarks::BookmarkModel* model,
                           bool ids_reassigned) override;
  void BookmarkNodeMoved(bookmarks::BookmarkModel* model,
                         const bookmarks::BookmarkNode* old_parent,
                         int old_index,
                         const bookmarks::BookmarkNode* new_parent,
                         int new_index) override;
  void BookmarkNodeAdded(bookmarks::BookmarkModel* model,
                         const bookmarks::BookmarkNode* parent,
                         int index) override;
  void BookmarkNodeRemoved(
      bookmarks::BookmarkModel* model,
      const bookmarks::BookmarkNode* parent,
      int old_index,
      const bookmarks::BookmarkNode* node,
      const std::set<GURL>& no_longer_bookmarked) override;
  void BookmarkNodeChanged(bookmarks::BookmarkModel* model,
                           const bookmarks::BookmarkNode* node) override;
  void BookmarkNodeFaviconChanged(bookmarks::BookmarkModel* model,
                                  const bookmarks::BookmarkNode* node) override;
  void BookmarkNodeChildrenReordered(bookmarks::BookmarkModel* model,
                                     const bookmarks::BookmarkNode* node) override;
  void BookmarkAllUserNodesRemoved(
      bookmarks::BookmarkModel* model,
      const std::set<GURL>& removed_urls) override;

  void CloneBookmarkNodeForDeleteImpl(
      const bookmarks::BookmarkNodeData::Element& element,
      bookmarks::BookmarkNode* parent,
      int index);
  void CloneBookmarkNodeForDelete(
      const std::vector<bookmarks::BookmarkNodeData::Element>& elements,
      bookmarks::BookmarkNode* parent,
      int index);

  void GetExistingBookmarks(
      const std::vector<std::unique_ptr<jslib::SyncRecord>>& records,
      SyncRecordAndExistingList* records_and_existing_objects);

  // Compiler complains at
  void OnMessageFromSyncReceived() override;

  void OnResetBookmarks();
  // SyncLibToBrowserHandler overrides
  void OnSyncDebug(const std::string &message) override;
  void OnSyncSetupError(const std::string &error) override;
  void OnGetInitData(const std::string &sync_version) override;
  void OnSaveInitData(const Uint8Array &seed, const Uint8Array &device_id) override;
  void OnSyncReady() override;
  void OnGetExistingObjects(const std::string &category_name,
    std::unique_ptr<RecordsList> records,
    const base::Time &last_record_time_stamp,
    const bool is_truncated) override;
  void OnResolvedSyncRecords(const std::string &category_name,
    std::unique_ptr<RecordsList> records) override;
  void OnDeletedSyncUser() override;
  void OnDeleteSyncSiteSettings() override;
  void OnSaveBookmarksBaseOrder(const std::string &order) override;
  void OnSaveBookmarkOrder(const std::string &order,
                           const std::string &prev_order,
                           const std::string &next_order,
                           const std::string &parent_order) override;
  void OnSyncWordsPrepared(const std::string &words) override;

  void OnResolvedPreferences(std::unique_ptr<RecordsList> records, const std::string& this_device_id);
  void OnResolvedBookmarks(const RecordsList &records);
  void OnResolvedHistorySites(const RecordsList &records);

  // Other private methods
  void RequestSyncData();
  void FetchSyncRecords(const bool bookmarks, const bool history,
    const bool preferences, int max_records);

  void SendCreateDevice();

  void SendUnsyncedBookmarks();
  void SendAllLocalHistorySites();

  void CreateUpdateDeleteHistorySites(
    const int &action,
    //history::QueryResults::URLResultVector list,
    const std::vector<history::URLResult> &list,
    const bool &addIdsToNotSynced,
    const bool &isInitialSync);

  // CanSendHistory overrides
  void HaveInitialHistory(history::QueryResults* results) override;

  void SetUpdateDeleteDeviceName_Ext(
    const std::string &action,
    const std::string &deviceName,
    const std::string &deviceId,
    const std::string &objectId);

  void StartLoop();
  void StopLoop();
  void LoopProc();
  void LoopProcThreadAligned();

  void GetExistingHistoryObjects(
    const RecordsList &records,
    const base::Time &last_record_time_stamp,
    const bool &is_truncated );

  void PushRRContext(const std::string &prev_order,
                     const std::string &next_order,
                     const std::string &parent_order,
                     const int64_t &node_id,
                     const int &action);
  void PopRRContext(const std::string &prev_order,
                    const std::string &next_order,
                    const std::string &parent_order,
                    int64_t &node_id,
                    int &action);

  void TriggerOnLogMessage(const std::string &message);
  void TriggerOnSyncStateChanged();
  void TriggerOnHaveSyncWords(const std::string &sync_words);

  std::unique_ptr<jslib::SyncRecord> BookmarkNodeToSyncBookmark(
      const bookmarks::BookmarkNode* node);

  void OnExtensionSystemReady();
  // ExtensionRegistryObserver:
  void OnExtensionReady(content::BrowserContext* browser_context,
                         const extensions::Extension* extension) override;
  void OnExtensionUnloaded(content::BrowserContext* browser_context,
                           const extensions::Extension* extension,
                           extensions::UnloadedExtensionReason reason) override;

  BraveSyncClient *sync_client_;

  // True when is in active sync chain
  bool sync_configured_ = false;
  // True if we have received SyncReady from JS lib
  bool sync_initialized_ = false;
  // Version received from "get-init-data" command
  std::string sync_version_;

  // Prevent two sequential calls OnSetupSyncHaveCode or OnSetupSyncNewToSync
  // while being initializing
  bool initializing_ = false;

  std::string sync_words_;

  std::unique_ptr<brave_sync::prefs::Prefs> sync_prefs_;
  std::unique_ptr<brave_sync::Settings> settings_;
  std::unique_ptr<brave_sync::storage::ObjectMap> sync_obj_map_;
  std::unique_ptr<brave_sync::History> history_;

  Profile *profile_;

  // Moment when FETCH_SYNC_RECORDS was sent,
  // will be saved on GET_EXISTING_OBJECTS to be sure request was processed
  base::Time last_time_fetch_sent_;

  // Map to keep tracking between request and response on query bookmarks order, access only in UI thread
  // <prev_order, next_order> => <node_id, action>
  std::map<std::string, std::tuple<int64_t, int>> rr_map_;

  const int ATTEMPTS_BEFORE_SENDING_NOT_SYNCED_RECORDS = 10;
  int attempts_before_send_not_synced_records_ = ATTEMPTS_BEFORE_SENDING_NOT_SYNCED_RECORDS;

  std::unique_ptr<base::RepeatingTimer> timer_;
  base::TimeDelta unsynced_send_interval_;
  uint64_t initial_sync_records_remaining_;
  bookmarks::BookmarkModel* bookmark_model_;
  scoped_refptr<base::SequencedTaskRunner> task_runner_;
  SEQUENCE_CHECKER(sequence_checker_);

  ScopedObserver<ExtensionRegistry, ExtensionRegistryObserver>
    extension_registry_observer_;

  base::WeakPtrFactory<BraveSyncServiceImpl> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(BraveSyncServiceImpl);
};

} // namespace brave_sync

#endif //BRAVE_COMPONENTS_SYNC_BRAVE_SYNC_SERVICE_IMPL_H_
