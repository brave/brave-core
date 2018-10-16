/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/brave_sync_service_impl.h"

// #include <sstream>

#include "base/strings/utf_string_conversions.h"
#include "base/task/post_task.h"
#include "brave/browser/ui/webui/sync/sync_ui.h"
#include "brave/common/extensions/extension_constants.h"
#include "brave/components/brave_sync/bookmark_order_util.h"
#include "brave/components/brave_sync/brave_sync_prefs.h"
#include "brave/components/brave_sync/brave_sync_service_observer.h"
#include "brave/components/brave_sync/client/brave_sync_client.h"
#include "brave/components/brave_sync/client/brave_sync_client_factory.h"
#include "brave/components/brave_sync/sync_devices.h"
#include "brave/components/brave_sync/history.h"
#include "brave/components/brave_sync/jslib_const.h"
#include "brave/components/brave_sync/jslib_messages.h"
#include "brave/components/brave_sync/settings.h"
#include "brave/components/brave_sync/tools.h"
#include "brave/components/brave_sync/values_conv.h"
#include "brave/vendor/bip39wally-core-native/include/wally_bip39.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_thread.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_storage.h"
#include "components/bookmarks/browser/bookmark_utils.h"
#include "extensions/browser/extension_registry.h"
#include "ui/base/models/tree_node_iterator.h"

namespace brave_sync {

namespace {

int64_t deleted_node_id = -1;
bookmarks::BookmarkNode* deleted_node_root;

void GetOrder(const bookmarks::BookmarkNode* parent,
              int index,
              std::string* prev_order,
              std::string* next_order,
              std::string* parent_order) {
  auto* prev_node = index == 0 ?
    nullptr :
    parent->GetChild(index - 1);
  auto* next_node = index == parent->child_count() - 1 ?
    nullptr :
    parent->GetChild(index + 1);

  if (prev_node)
    prev_node->GetMetaInfo("order", prev_order);

  if (next_node)
    next_node->GetMetaInfo("order", next_order);

  parent->GetMetaInfo("order", parent_order);
}

RecordsListPtr CreateDeviceCreationRecordExtension(
  const std::string& deviceName,
  const std::string& objectId,
  const jslib::SyncRecord::Action &action,
  const std::string& deviceId) {
  RecordsListPtr records = std::make_unique<RecordsList>();

  SyncRecordPtr record = std::make_unique<jslib::SyncRecord>();

  record->action = action;
  record->deviceId = deviceId;
  record->objectId = objectId;
  record->objectData = jslib_const::SyncObjectData_DEVICE; // "device"

  std::unique_ptr<jslib::Device> device = std::make_unique<jslib::Device>();
  device->name = deviceName;
  record->SetDevice(std::move(device));

  records->emplace_back(std::move(record));

  return records;
}

SyncRecordPtr PrepareResolvedDevice(
    SyncDevice* device,
    int action) {
  auto record = std::make_unique<jslib::SyncRecord>();

  record->action = ConvertEnum<brave_sync::jslib::SyncRecord::Action>(action,
      brave_sync::jslib::SyncRecord::Action::A_MIN,
      brave_sync::jslib::SyncRecord::Action::A_MAX,
      brave_sync::jslib::SyncRecord::Action::A_INVALID);
  record->deviceId = device->device_id_;
  record->objectId = device->object_id_;
  record->objectData = jslib_const::SyncObjectData_DEVICE; // "device"

  std::unique_ptr<jslib::Device> device_record =
      std::make_unique<jslib::Device>();
  device_record->name = device->name_;
  record->SetDevice(std::move(device_record));

  return record;
}

}  // namespace

bool IsSyncManagedNode(const bookmarks::BookmarkPermanentNode* node) {
  return node->id() == deleted_node_id;
}

bookmarks::BookmarkPermanentNodeList
LoadExtraNodes(bookmarks::LoadExtraCallback callback,
               int64_t* next_node_id) {
  bookmarks::BookmarkPermanentNodeList extra_nodes;
  if (callback)
    extra_nodes = std::move(callback).Run(next_node_id);

  auto node = std::make_unique<bookmarks::BookmarkPermanentNode>(*next_node_id);
  deleted_node_id = *next_node_id;
  *next_node_id = deleted_node_id + 1;
  node->set_type(bookmarks::BookmarkNode::FOLDER);
  node->set_visible(false);
  node->SetTitle(base::UTF8ToUTF16("Deleted Bookmarks"));

  extra_nodes.push_back(std::move(node));

  return extra_nodes;
}

BraveSyncServiceImpl::BraveSyncServiceImpl(Profile *profile) :
    sync_client_(BraveSyncClientFactory::GetForBrowserContext(profile)),
    sync_initialized_(false),
    sync_words_(std::string()),
    profile_(profile),
    timer_(std::make_unique<base::RepeatingTimer>()),
    unsynced_send_interval_(base::TimeDelta::FromMinutes(10)),
    initial_sync_records_remaining_(0),
    bookmark_model_(BookmarkModelFactory::GetForBrowserContext(profile)),
    extension_registry_observer_(this),
    weak_ptr_factory_(this) {
  CHECK(bookmark_model_);
  extension_registry_observer_.Add(ExtensionRegistry::Get(profile));

  sync_prefs_ = std::make_unique<brave_sync::prefs::Prefs>(profile);

  if (!sync_prefs_->GetSeed().empty() &&
      !sync_prefs_->GetThisDeviceName().empty()) {
    sync_configured_ = true;
  }

  sync_client_->SetSyncToBrowserHandler(this);
}

BraveSyncServiceImpl::~BraveSyncServiceImpl() {}

bookmarks::BookmarkNode* BraveSyncServiceImpl::GetDeletedNodeRoot() {
  if (!deleted_node_root)
    deleted_node_root = const_cast<bookmarks::BookmarkNode*>(
        bookmarks::GetBookmarkNodeByID(bookmark_model_, deleted_node_id));

  return deleted_node_root;
}

bool BraveSyncServiceImpl::IsSyncConfigured() {
  return sync_configured_;
}

bool BraveSyncServiceImpl::IsSyncInitialized() {
  return sync_initialized_;
}

void BraveSyncServiceImpl::Shutdown() {
  StopLoop();
}

void BraveSyncServiceImpl::OnSetupSyncHaveCode(const std::string& sync_words,
    const std::string& device_name) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (sync_words.empty() || device_name.empty()) {
    OnSyncSetupError("missing sync words or device name");
    return;
  }

  if (initializing_) {
    NotifyLogMessage("currently initializing");
    return;
  }

  if (IsSyncConfigured()) {
    NotifyLogMessage("already configured");
    return;
  }

  sync_prefs_->SetThisDeviceName(device_name);
  initializing_ = true;

  sync_prefs_->SetSyncEnabled(true);
  sync_words_ = sync_words;
}

void BraveSyncServiceImpl::OnSetupSyncNewToSync(
    const std::string& device_name) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (device_name.empty()) {
    OnSyncSetupError("missing device name");
    return;
  }

  if (initializing_) {
    NotifyLogMessage("currently initializing");
    return;
  }

  if (IsSyncConfigured()) {
    NotifyLogMessage("already configured");
    return;
  }

  sync_prefs_->SetThisDeviceName(device_name);
  initializing_ = true;

  sync_prefs_->SetSyncEnabled(true);
}

void BraveSyncServiceImpl::OnDeleteDevice(const std::string& device_id) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  auto sync_devices = sync_prefs_->GetSyncDevices();

  const SyncDevice *device = sync_devices->GetByDeviceId(device_id);
  if (device) {
    const std::string device_name = device->name_;
    const std::string object_id = device->object_id_;
    SendDeviceSyncRecord(
        jslib::SyncRecord::Action::DELETE, device_name, device_id, object_id);
  }
}

void BraveSyncServiceImpl::OnResetSync() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  const std::string device_id = sync_prefs_->GetThisDeviceId();

  OnResetBookmarks();

  OnDeleteDevice(device_id);

  sync_prefs_->Clear();

  sync_configured_ = false;
  sync_initialized_ = false;

  sync_prefs_->SetSyncEnabled(false);

  NotifySyncStateChanged();
}

void BraveSyncServiceImpl::GetSettingsAndDevices(
    const GetSettingsAndDevicesCallback& callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  auto settings = sync_prefs_->GetBraveSyncSettings();
  auto devices = sync_prefs_->GetSyncDevices();
  callback.Run(std::move(settings), std::move(devices));
}

void BraveSyncServiceImpl::GetSyncWords() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  // Ask sync client
  DCHECK(sync_client_);
  std::string seed = sync_prefs_->GetSeed();
  sync_client_->NeedSyncWords(seed);
}

std::string BraveSyncServiceImpl::GetSeed() {
  return sync_prefs_->GetSeed();
}

void BraveSyncServiceImpl::OnSetSyncEnabled(const bool sync_this_device) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  sync_prefs_->SetSyncEnabled(sync_this_device);
  NotifySyncStateChanged();
}

void BraveSyncServiceImpl::OnSetSyncBookmarks(const bool sync_bookmarks) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  sync_prefs_->SetSyncBookmarksEnabled(sync_bookmarks);
  NotifySyncStateChanged();
}

void BraveSyncServiceImpl::OnSetSyncBrowsingHistory(
    const bool sync_browsing_history) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  sync_prefs_->SetSyncHistoryEnabled(sync_browsing_history);
  NotifySyncStateChanged();
}

void BraveSyncServiceImpl::OnSetSyncSavedSiteSettings(
    const bool sync_saved_site_settings) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  sync_prefs_->SetSyncSiteSettingsEnabled(sync_saved_site_settings);
  NotifySyncStateChanged();
}

// SyncLibToBrowserHandler overrides
void BraveSyncServiceImpl::OnSyncDebug(const std::string& message) {
  NotifyLogMessage(message);
}

void BraveSyncServiceImpl::OnSyncSetupError(const std::string& error) {
  initializing_ = false;
  if (!sync_initialized_) {
    sync_prefs_->Clear();
  }
  OnSyncDebug(error);
}

void BraveSyncServiceImpl::OnGetInitData(const std::string& sync_version) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  Uint8Array seed;
  if (!sync_words_.empty()) {
    VLOG(1) << "[Brave Sync] Init from sync words";
  } else if (!sync_prefs_->GetSeed().empty()) {
    seed = Uint8ArrayFromString(sync_prefs_->GetSeed());
    VLOG(1) << "[Brave Sync] Init from prefs";
  } else {
    VLOG(1) << "[Brave Sync] Init new chain";
  }

  Uint8Array device_id;
  if (!sync_prefs_->GetThisDeviceId().empty()) {
    device_id = Uint8ArrayFromString(sync_prefs_->GetThisDeviceId());
    VLOG(1) << "[Brave Sync] Init device id from prefs: " <<
        StrFromUint8Array(device_id);
  } else {
    VLOG(1) << "[Brave Sync] Init empty device id";
  }

  DCHECK(!sync_version.empty());
  // TODO(bridiver) - this seems broken because using the version we get back
  // from the server (currently v1.4.2) causes things to break. What is the
  // the point of having this value?
  sync_prefs_->SetApiVersion("0");

  brave_sync::client_data::Config config;
  config.api_version = sync_prefs_->GetApiVersion();
#if defined(OFFICIAL_BUILD)
  config.server_url = "https://sync.brave.com";
#else
  config.server_url = "https://sync-staging.brave.com";
#endif
  config.debug = true;
  sync_client_->SendGotInitData(seed, device_id, config, sync_words_);
}

void BraveSyncServiceImpl::OnSaveInitData(const Uint8Array& seed,
                                          const Uint8Array& device_id) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  DCHECK(!sync_initialized_);
  DCHECK(initializing_);

  std::string seed_str = StrFromUint8Array(seed);
  std::string device_id_str = StrFromUint8Array(device_id);

  sync_words_.clear();
  DCHECK(!seed_str.empty());
  sync_prefs_->SetSeed(seed_str);
  sync_prefs_->SetThisDeviceId(device_id_str);

  sync_configured_ = true;

  sync_prefs_->SetSyncBookmarksEnabled(true);
  sync_prefs_->SetSyncSiteSettingsEnabled(true);
  // TODO(bridiver) - re-enable this when we add history
  sync_prefs_->SetSyncHistoryEnabled(false);

  initializing_ = false;
}

void BraveSyncServiceImpl::OnSyncReady() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  const std::string bookmarks_base_order = sync_prefs_->GetBookmarksBaseOrder();
  if (bookmarks_base_order.empty()) {
    std::string platform = tools::GetPlatformName();
    sync_client_->SendGetBookmarksBaseOrder(sync_prefs_->GetThisDeviceId(),
                                            platform);
    // OnSyncReady will be called by OnSaveBookmarksBaseOrder
    return;
  }

  DCHECK(false == sync_initialized_);
  sync_initialized_ = true;

  NotifySyncStateChanged();

  // fetch the records
  RequestSyncData();
}

void BraveSyncServiceImpl::OnResetBookmarks() {
  ui::TreeNodeIterator<const bookmarks::BookmarkNode>
      iterator(bookmark_model_->root_node());
  bookmark_model_->BeginExtensiveChanges();
  while (iterator.has_next()) {
    const bookmarks::BookmarkNode* node = iterator.Next();
    bookmark_model_->DeleteNodeMetaInfo(node, "object_id");
    bookmark_model_->DeleteNodeMetaInfo(node, "order");
    bookmark_model_->DeleteNodeMetaInfo(node, "sync_timestamp");
    bookmark_model_->DeleteNodeMetaInfo(node, "last_send_time");
  }
  auto* deleted_node = GetDeletedNodeRoot();
  CHECK(deleted_node);
  deleted_node->DeleteAll();
  bookmark_model_->EndExtensiveChanges();
}

void BraveSyncServiceImpl::OnGetExistingObjects(
    const std::string& category_name,
    std::unique_ptr<RecordsList> records,
    const base::Time &last_record_time_stamp,
    const bool is_truncated) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  // TODO(bridiver) - what do we do with is_truncated ?
  // It appears to be ignored in b-l
  if (!tools::IsTimeEmpty(last_record_time_stamp)) {
    sync_prefs_->SetLatestRecordTime(last_record_time_stamp);
  }

  if (category_name == jslib_const::kBookmarks) {
    auto records_and_existing_objects =
        std::make_unique<SyncRecordAndExistingList>();
    GetExistingBookmarks(*records.get(), records_and_existing_objects.get());
    sync_client_->SendResolveSyncRecords(
        category_name, std::move(records_and_existing_objects));
  } else if (category_name == brave_sync::jslib_const::kPreferences) {
    auto existing_records = PrepareResolvedPreferences(*records.get());
    sync_client_->SendResolveSyncRecords(
        category_name, std::move(existing_records));
  }
}

void BraveSyncServiceImpl::OnResolvedSyncRecords(
    const std::string& category_name,
    std::unique_ptr<RecordsList> records) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (category_name == brave_sync::jslib_const::kPreferences) {
    OnResolvedPreferences(*records.get());
  } else if (category_name == brave_sync::jslib_const::kBookmarks) {
    OnResolvedBookmarks(*records.get());
  } else if (category_name == brave_sync::jslib_const::kHistorySites) {
    NOTIMPLEMENTED();
  }

  SendUnsyncedBookmarks();
}

std::unique_ptr<SyncRecordAndExistingList>
BraveSyncServiceImpl::PrepareResolvedPreferences(const RecordsList& records) {
  auto sync_devices = sync_prefs_->GetSyncDevices();

  auto records_and_existing_objects =
        std::make_unique<SyncRecordAndExistingList>();

  for (const SyncRecordPtr& record : records) {
    auto resolved_record = std::make_unique<SyncRecordAndExisting>();
    resolved_record->first = jslib::SyncRecord::Clone(*record);
    auto* device = sync_devices->GetByObjectId(record->objectId);
    if (device)
      resolved_record->second = PrepareResolvedDevice(device, record->action);
    records_and_existing_objects->emplace_back(std::move(resolved_record));
  }

  return records_and_existing_objects;
}

void BraveSyncServiceImpl::OnResolvedPreferences(const RecordsList& records) {
  const std::string this_device_id = sync_prefs_->GetThisDeviceId();
  bool this_device_deleted = false;

  auto sync_devices = sync_prefs_->GetSyncDevices();
  for (const auto &record : records) {
    DCHECK(record->has_device() || record->has_sitesetting());
    if (record->has_device()) {
      bool actually_merged = false;
      sync_devices->Merge(
          SyncDevice(record->GetDevice().name,
          record->objectId,
          record->deviceId,
          record->syncTimestamp.ToJsTime()),
          record->action,
          &actually_merged);
      this_device_deleted = this_device_deleted ||
        (record->deviceId == this_device_id &&
          record->action == jslib::SyncRecord::Action::DELETE &&
          actually_merged);
    }
  } // for each device

  sync_prefs_->SetSyncDevices(*sync_devices);

  if (this_device_deleted)
    OnResetSync();
  else
    NotifySyncStateChanged();
}

const bookmarks::BookmarkNode* FindByObjectId(bookmarks::BookmarkModel* model,
                                        const std::string& object_id) {
  ui::TreeNodeIterator<const bookmarks::BookmarkNode>
      iterator(model->root_node());
  while (iterator.has_next()) {
    const bookmarks::BookmarkNode* node = iterator.Next();
    std::string node_object_id;
    node->GetMetaInfo("object_id", &node_object_id);

    if (node_object_id.empty())
      continue;

    if (object_id == node_object_id)
      return node;
  }
  return nullptr;
}

uint64_t GetIndex(const bookmarks::BookmarkNode* root_node,
                  const jslib::Bookmark& record) {
  uint64_t index = 0;
  ui::TreeNodeIterator<const bookmarks::BookmarkNode> iterator(root_node);
  while (iterator.has_next()) {
    const bookmarks::BookmarkNode* node = iterator.Next();
    std::string node_order;
    node->GetMetaInfo("order", &node_order);

    if (!node_order.empty() &&
        brave_sync::CompareOrder(record.order, node_order))
      return index;

    ++index;
  }
  return index;
}

// this should only be called for resolved records we get from the server
void UpdateNode(bookmarks::BookmarkModel* model,
                const bookmarks::BookmarkNode* node,
                const jslib::SyncRecord* record) {
  auto bookmark = record->GetBookmark();
  if (bookmark.isFolder) {
    // SetDateFolderModified
  } else {
    model->SetURL(node, GURL(bookmark.site.location));
    // TODO, AB: apply these:
    // sync_bookmark.site.customTitle
    // sync_bookmark.site.lastAccessedTime
    // sync_bookmark.site.favicon
  }

  model->SetTitle(node,
      base::UTF8ToUTF16(bookmark.site.title));
  model->SetDateAdded(node, bookmark.site.creationTime);
  model->SetNodeMetaInfo(node, "object_id", record->objectId);
  model->SetNodeMetaInfo(node, "order", bookmark.order);

  // updating the sync_timestamp marks this record as synced
  std::string sync_timestamp;
  node->GetMetaInfo("sync_timestamp", &sync_timestamp);
  if (sync_timestamp.empty()) {
    model->SetNodeMetaInfo(node,
        "sync_timestamp",
        std::to_string(record->syncTimestamp.ToJsTime()));
    model->DeleteNodeMetaInfo(node, "last_send_time");
  }
}

void BraveSyncServiceImpl::OnResolvedBookmarks(const RecordsList &records) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  bookmark_model_->BeginExtensiveChanges();
  for (const auto& sync_record : records) {
    DCHECK(sync_record->has_bookmark());
    DCHECK(!sync_record->objectId.empty());

    auto* node = FindByObjectId(bookmark_model_, sync_record->objectId);
    auto bookmark_record = sync_record->GetBookmark();

    if (node && sync_record->action == jslib::SyncRecord::Action::UPDATE) {
      UpdateNode(bookmark_model_, node, sync_record.get());
      int64_t old_parent_local_id = node->parent()->id();
      const bookmarks::BookmarkNode* old_parent_node =
          bookmarks::GetBookmarkNodeByID(bookmark_model_, old_parent_local_id);

      std::string old_parent_object_id;
      if (old_parent_node) {
        old_parent_node->GetMetaInfo("object_id", &old_parent_object_id);
      }

      const bookmarks::BookmarkNode* new_parent_node;
      if (bookmark_record.parentFolderObjectId != old_parent_object_id) {
        new_parent_node = FindByObjectId(bookmark_model_,
                                         bookmark_record.parentFolderObjectId);
        // TODO(bridiver) - what if new_parent_node doesn't exist yet?
        DCHECK(new_parent_node);
      } else {
        new_parent_node = nullptr;
      }

      if (new_parent_node) {
        int64_t index = GetIndex(new_parent_node, bookmark_record);
        bookmark_model_->Move(node, new_parent_node, index);
      }
    } else if (node &&
               sync_record->action == jslib::SyncRecord::Action::DELETE) {
      if (node->parent() == GetDeletedNodeRoot()) {
        // this is a deleted node so remove without firing events
        int index = GetDeletedNodeRoot()->GetIndexOf(node);
        GetDeletedNodeRoot()->Remove(index);
      } else {
        // normal remove
        bookmark_model_->Remove(node);
      }
    } else if (!node) {
      // TODO(bridiver) (make sure there isn't an existing record for objectId)

      const bookmarks::BookmarkNode* parent_node =
          FindByObjectId(bookmark_model_, bookmark_record.parentFolderObjectId);

      if (!parent_node) {
        if (!bookmark_record.order.empty() &&
            bookmark_record.order.at(0) == '2') {
          // mobile generated bookmarks go in the mobile folder so they don't
          // get so we don't get m.xxx.xxx domains in the normal bookmarks
          parent_node = bookmark_model_->mobile_node();
        } else if (!bookmark_record.hideInToolbar) {
          // this flag is a bit odd, but if the node doesn't have a parent and
          // hideInToolbar is false, then this bookmark should go in the
          // toolbar root. We don't care about this flag for records with
          // a parent id because they will be inserted into the correct
          // parent folder
          parent_node = bookmark_model_->bookmark_bar_node();
        } else {
          parent_node = bookmark_model_->other_node();
        }
      }

      if (bookmark_record.isFolder) {
        node = bookmark_model_->AddFolder(
                        parent_node,
                        GetIndex(parent_node, bookmark_record),
                        base::UTF8ToUTF16(bookmark_record.site.title));
      } else {
        node = bookmark_model_->AddURL(parent_node,
                              GetIndex(parent_node, bookmark_record),
                              base::UTF8ToUTF16(bookmark_record.site.title),
                              GURL(bookmark_record.site.location));
      }
      UpdateNode(bookmark_model_, node, sync_record.get());
    }
  }
  bookmark_model_->EndExtensiveChanges();
}

void BraveSyncServiceImpl::OnDeletedSyncUser() {
  NOTIMPLEMENTED();
}

void BraveSyncServiceImpl::OnDeleteSyncSiteSettings()  {
  NOTIMPLEMENTED();
}

void BraveSyncServiceImpl::OnSaveBookmarksBaseOrder(const std::string& order)  {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(!order.empty());
  sync_prefs_->SetBookmarksBaseOrder(order);
  OnSyncReady();
}

void BraveSyncServiceImpl::OnSaveBookmarkOrder(const std::string& order,
                                              const std::string& prev_order,
                                              const std::string& next_order,
                                              const std::string& parent_order) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(!order.empty());

  int64_t between_order_rr_context_node_id = -1;
  int action = -1;

  PopRRContext(prev_order, next_order, parent_order,
      between_order_rr_context_node_id, &action);

  DCHECK(between_order_rr_context_node_id != -1);
  DCHECK(action != -1);

  auto* bookmark_node = bookmarks::GetBookmarkNodeByID(
      bookmark_model_, between_order_rr_context_node_id);

  if (bookmark_node) {
    bookmark_model_->SetNodeMetaInfo(bookmark_node, "order", order);
    BookmarkNodeChanged(bookmark_model_, bookmark_node);
  }

  base::Time last_fetch_time = sync_prefs_->GetLastFetchTime();
  if (tools::IsTimeEmpty(last_fetch_time)) {
    --initial_sync_records_remaining_;
    if (initial_sync_records_remaining_ == 0) {
      sync_prefs_->SetLastFetchTime(base::Time::Now());
      RequestSyncData();
    }
  }
}

void BraveSyncServiceImpl::PushRRContext(const std::string& prev_order,
                                         const std::string& next_order,
                                         const std::string& parent_order,
                                         const int64_t& node_id,
                                         const int action) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  std::string key(prev_order + "-" + next_order + "-" + parent_order);
  DCHECK(rr_map_.find(key) == rr_map_.end());
  rr_map_[key] = std::make_tuple(node_id, action);
}

void BraveSyncServiceImpl::PopRRContext(const std::string& prev_order,
                                        const std::string& next_order,
                                        const std::string& parent_order,
                                        int64_t& node_id,
                                        int* action) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  std::string key(prev_order + "-" + next_order + "-" + parent_order);
  auto it = rr_map_.find(key);
  DCHECK(it != rr_map_.end());
  node_id = std::get<0>(it->second);
  *action = std::get<1>(it->second);
  rr_map_.erase(it);
}

void BraveSyncServiceImpl::OnSyncWordsPrepared(const std::string& words) {
  NotifyHaveSyncWords(words);
}

// Here we query sync lib for the records after initialization (or again later)
void BraveSyncServiceImpl::RequestSyncData() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  // still sending sync records
  if (initial_sync_records_remaining_ > 0)
    return;

  const bool bookmarks = sync_prefs_->GetSyncBookmarksEnabled();
  const bool history = sync_prefs_->GetSyncHistoryEnabled();
  const bool preferences = sync_prefs_->GetSyncSiteSettingsEnabled();

  if (!bookmarks && !history && !preferences)
    return;

  base::Time last_fetch_time = sync_prefs_->GetLastFetchTime();

  if (tools::IsTimeEmpty(last_fetch_time)) {
    SendCreateDevice();

    auto* deleted_node = GetDeletedNodeRoot();
    CHECK(deleted_node);
    std::vector<const bookmarks::BookmarkNode*> root_nodes = {
      bookmark_model_->other_node(),
      bookmark_model_->bookmark_bar_node(),
      deleted_node
    };

    std::vector<const bookmarks::BookmarkNode*> sync_nodes;
    for (const auto* root_node : root_nodes) {
      ui::TreeNodeIterator<const bookmarks::BookmarkNode>
          iterator(root_node);
      while (iterator.has_next())
        sync_nodes.push_back(iterator.Next());
    }

    initial_sync_records_remaining_ = sync_nodes.size();

    for (auto* node : sync_nodes)
      BookmarkNodeAdded(bookmark_model_,
                        node->parent(),
                        node->parent()->GetIndexOf(node));

    if (initial_sync_records_remaining_ > 0)
      return;
  }

  FetchSyncRecords(bookmarks, history, preferences, 1000);
}

void BraveSyncServiceImpl::FetchSyncRecords(const bool bookmarks,
  const bool history, const bool preferences, int max_records) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(bookmarks || history || preferences);
  if (!(bookmarks || history || preferences)) {
    return;
  }

  std::vector<std::string> category_names;
  using namespace brave_sync::jslib_const;
  if (history) {
    category_names.push_back(kHistorySites); // "HISTORY_SITES";
  }
  if (bookmarks) {
    category_names.push_back(kBookmarks);//"BOOKMARKS";
  }
  if (preferences) {
    category_names.push_back(kPreferences);//"PREFERENCES";
  }

  DCHECK(sync_client_);
  sync_prefs_->SetLastFetchTime(base::Time::Now());

  base::Time start_at_time = sync_prefs_->GetLatestRecordTime();
  sync_client_->SendFetchSyncRecords(
    category_names,
    start_at_time,
    max_records);
}

void BraveSyncServiceImpl::SendCreateDevice() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  std::string device_name = sync_prefs_->GetThisDeviceName();
  std::string object_id = brave_sync::tools::GenerateObjectId();
  std::string device_id = sync_prefs_->GetThisDeviceId();
  CHECK(!device_id.empty());

  SendDeviceSyncRecord(
      jslib::SyncRecord::Action::CREATE,
      device_name,
      device_id,
      object_id);
}

void BraveSyncServiceImpl::SendDeviceSyncRecord(
    const int action,
    const std::string& device_name,
    const std::string& device_id,
    const std::string& object_id) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  RecordsListPtr records = CreateDeviceCreationRecordExtension(
      device_name,
      object_id,
      static_cast<jslib::SyncRecord::Action>(action),
      device_id);
  sync_client_->SendSyncRecords(
      jslib_const::SyncRecordType_PREFERENCES, *records);
}

std::unique_ptr<jslib::SyncRecord>
BraveSyncServiceImpl::BookmarkNodeToSyncBookmark(
    const bookmarks::BookmarkNode* node) {
  if (node->is_permanent_node() || !node->parent())
    return std::unique_ptr<jslib::SyncRecord>();

  auto record = std::make_unique<jslib::SyncRecord>();
  record->deviceId = sync_prefs_->GetThisDeviceId();
  record->objectData = jslib_const::SyncObjectData_BOOKMARK;

  auto bookmark = std::make_unique<jslib::Bookmark>();
  bookmark->site.location = node->url().spec();
  bookmark->site.title = base::UTF16ToUTF8(node->GetTitledUrlNodeTitle());
  bookmark->site.customTitle = base::UTF16ToUTF8(node->GetTitle());
  //bookmark->site.lastAccessedTime - ignored
  bookmark->site.creationTime = node->date_added();
  bookmark->site.favicon = node->icon_url() ? node->icon_url()->spec() : "";
  bookmark->isFolder = node->is_folder();
  bookmark->hideInToolbar =
      !node->HasAncestor(bookmark_model_->bookmark_bar_node());

  // these will be empty for unsynced nodes
  std::string sync_timestamp;
  node->GetMetaInfo("sync_timestamp", &sync_timestamp);
  if (!sync_timestamp.empty())
    record->syncTimestamp = base::Time::FromJsTime(std::stod(sync_timestamp));

  std::string object_id;
  node->GetMetaInfo("object_id", &object_id);
  record->objectId = object_id;

  std::string parent_object_id;
  node->parent()->GetMetaInfo("object_id", &parent_object_id);
  bookmark->parentFolderObjectId = parent_object_id;

  std::string order;
  node->GetMetaInfo("order", &order);
  bookmark->order = order;
  DCHECK(!order.empty());

  auto* deleted_node = GetDeletedNodeRoot();
  CHECK(deleted_node);

  if (record->objectId.empty()) {
    record->objectId = tools::GenerateObjectId();
    record->action = jslib::SyncRecord::Action::CREATE;
    bookmark_model_->SetNodeMetaInfo(node, "object_id", record->objectId);
  } else if (node->HasAncestor(deleted_node)) {
   record->action = jslib::SyncRecord::Action::DELETE;
  } else {
    record->action = jslib::SyncRecord::Action::UPDATE;
    DCHECK(!bookmark->order.empty());
    DCHECK(!record->objectId.empty());
  }

  record->SetBookmark(std::move(bookmark));

  return record;
}

void BraveSyncServiceImpl::GetExistingBookmarks(
    const std::vector<std::unique_ptr<jslib::SyncRecord>>& records,
    SyncRecordAndExistingList* records_and_existing_objects) {
  for (const auto& record : records){
    auto resolved_record = std::make_unique<SyncRecordAndExisting>();
    resolved_record->first = jslib::SyncRecord::Clone(*record);
    auto* node = FindByObjectId(bookmark_model_, record->objectId);
    if (node)
      resolved_record->second = BookmarkNodeToSyncBookmark(node);

    records_and_existing_objects->push_back(std::move(resolved_record));
  }
}

void BraveSyncServiceImpl::SendUnsyncedBookmarks() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  std::vector<std::unique_ptr<jslib::SyncRecord>> records;

  auto* deleted_node = GetDeletedNodeRoot();
  CHECK(deleted_node);
  std::vector<const bookmarks::BookmarkNode*> root_nodes = {
    bookmark_model_->other_node(),
    bookmark_model_->bookmark_bar_node(),
    deleted_node
  };

  for (const auto* root_node : root_nodes) {
    ui::TreeNodeIterator<const bookmarks::BookmarkNode>
        iterator(root_node);
    while (iterator.has_next()) {
      const bookmarks::BookmarkNode* node = iterator.Next();

      // only send unsynced records
      std::string sync_timestamp;
      node->GetMetaInfo("sync_timestamp", &sync_timestamp);
      if (!sync_timestamp.empty())
        continue;

      std::string last_send_time;
      node->GetMetaInfo("last_send_time", &last_send_time);
      if (!last_send_time.empty() &&
          // don't send more often than unsynced_send_interval_
          (base::Time::Now() -
              base::Time::FromJsTime(std::stod(last_send_time))) <
          unsynced_send_interval_)
        continue;

      bookmark_model_->SetNodeMetaInfo(node,
          "last_send_time", std::to_string(base::Time::Now().ToJsTime()));
      auto record = BookmarkNodeToSyncBookmark(node);
      if (record)
        records.push_back(std::move(record));

      if (records.size() == 1000) {
        sync_client_->SendSyncRecords(
            jslib_const::SyncRecordType_BOOKMARKS, records);
        records.clear();
      }
    }
  }
  if (!records.empty()) {
    sync_client_->SendSyncRecords(
      jslib_const::SyncRecordType_BOOKMARKS, records);
    records.clear();
  }
}

void BraveSyncServiceImpl::BookmarkModelLoaded(
    bookmarks::BookmarkModel* model,
    bool ids_reassigned) {
  // TODO(bridiver) - do we need to handle ids_reassigned?
}

void BraveSyncServiceImpl::BookmarkNodeFaviconChanged(
    bookmarks::BookmarkModel* model,
    const bookmarks::BookmarkNode* node) {
  BookmarkNodeChanged(model, node);
}

void BraveSyncServiceImpl::BookmarkNodeChildrenReordered(
    bookmarks::BookmarkModel* model,
    const bookmarks::BookmarkNode* node) {
  // this should be safe to ignore as it's only called for managed bookmarks
}

void BraveSyncServiceImpl::BookmarkAllUserNodesRemoved(
    bookmarks::BookmarkModel* model,
    const std::set<GURL>& removed_urls) {
  // this only happens on profile deletion and we don't want
  // to wipe out the remote store when that happens
}

void BraveSyncServiceImpl::BookmarkNodeMoved(
    bookmarks::BookmarkModel* model,
    const bookmarks::BookmarkNode* old_parent,
    int old_index,
    const bookmarks::BookmarkNode* new_parent,
    int new_index) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  auto* node = new_parent->GetChild(new_index);

  std::string prev_node_order;
  std::string next_node_order;
  std::string parent_node_order;
  GetOrder(new_parent, new_index,
           &prev_node_order, &next_node_order, &parent_node_order);

  PushRRContext(
      prev_node_order, next_node_order, parent_node_order,
      node->id(), jslib_const::kActionUpdate);
  sync_client_->SendGetBookmarkOrder(
      prev_node_order, next_node_order, parent_node_order);
  // responds in OnSaveBookmarkOrder
}

void BraveSyncServiceImpl::BookmarkNodeAdded(bookmarks::BookmarkModel* model,
                                          const bookmarks::BookmarkNode* parent,
                                          int index) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  auto* node = parent->GetChild(index);

  std::string prev_node_order;
  std::string next_node_order;
  std::string parent_node_order;
  GetOrder(parent, index,
           &prev_node_order, &next_node_order, &parent_node_order);

  // this is a giant hack and have an empty value for all 3 should
  // be handled in the sync js lib
  if (parent_node_order.empty())
    parent_node_order =
        sync_prefs_->GetBookmarksBaseOrder() + std::to_string(index);

  PushRRContext(
      prev_node_order, next_node_order, parent_node_order,
      node->id(), jslib_const::kActionCreate);
  sync_client_->SendGetBookmarkOrder(
      prev_node_order, next_node_order, parent_node_order);
  // responds in OnSaveBookmarkOrder
}

void BraveSyncServiceImpl::CloneBookmarkNodeForDeleteImpl(
    const bookmarks::BookmarkNodeData::Element& element,
    bookmarks::BookmarkNode* parent,
    int index) {
  auto cloned_node =
      std::make_unique<bookmarks::BookmarkNode>(element.id(), element.url);
  if (!element.is_url) {
    cloned_node->set_type(bookmarks::BookmarkNode::FOLDER);
    for (int i = 0; i < static_cast<int>(element.children.size()); ++i)
      CloneBookmarkNodeForDeleteImpl(element.children[i], cloned_node.get(), i);
  }
  cloned_node->SetTitle(element.title);

  // clear sync timestsamp so this sends in unsynced records
  bookmarks::BookmarkNode::MetaInfoMap meta_info_map = element.meta_info_map;
  meta_info_map.erase("sync_timestamp");
  cloned_node->SetMetaInfoMap(meta_info_map);

  auto* cloned_node_ptr = cloned_node.get();
  parent->Add(std::move(cloned_node), index);
  // we call `Changed` here because we don't want to update the order
  BookmarkNodeChanged(bookmark_model_, cloned_node_ptr);
}

void BraveSyncServiceImpl::CloneBookmarkNodeForDelete(
    const std::vector<bookmarks::BookmarkNodeData::Element>& elements,
    bookmarks::BookmarkNode* parent,
    int index) {
  for (size_t i = 0; i < elements.size(); ++i) {
    CloneBookmarkNodeForDeleteImpl(
        elements[i], parent, index + static_cast<int>(i));
  }
}

void BraveSyncServiceImpl::BookmarkNodeRemoved(
    bookmarks::BookmarkModel* model,
    const bookmarks::BookmarkNode* parent,
    int old_index,
    const bookmarks::BookmarkNode* node,
    const std::set<GURL>& no_longer_bookmarked) {
  // copy into the deleted node tree without firing any events
  auto* deleted_node = GetDeletedNodeRoot();
  CHECK(deleted_node);
  bookmarks::BookmarkNodeData data(node);
  CloneBookmarkNodeForDelete(
      data.elements, deleted_node, deleted_node->child_count());
}

void BraveSyncServiceImpl::BookmarkNodeChanged(bookmarks::BookmarkModel* model,
                                          const bookmarks::BookmarkNode* node) {
  // clearing the sync_timestamp will put the record back in the `Unsynced` list
  model->DeleteNodeMetaInfo(node, "sync_timestamp");
  // also clear the last send time because this is a new change
  model->DeleteNodeMetaInfo(node, "last_send_time");
}

static const int64_t kCheckUpdatesIntervalSec = 60;

void BraveSyncServiceImpl::StartLoop() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  bookmark_model_->AddObserver(this);
  timer_->Start(FROM_HERE,
                  base::TimeDelta::FromSeconds(kCheckUpdatesIntervalSec),
                  this,
                  &BraveSyncServiceImpl::LoopProc);
}

void BraveSyncServiceImpl::StopLoop() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  timer_->Stop();
  bookmark_model_->RemoveObserver(this);
}

void BraveSyncServiceImpl::LoopProc() {
  content::BrowserThread::GetTaskRunnerForThread(
      content::BrowserThread::UI)->PostTask(
          FROM_HERE,
          base::BindOnce(&BraveSyncServiceImpl::LoopProcThreadAligned,
              // the timer will always be destroyed before the service
              base::Unretained(this)));
}

void BraveSyncServiceImpl::LoopProcThreadAligned() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!sync_initialized_) {
    return;
  }

  RequestSyncData();
}

void BraveSyncServiceImpl::NotifyLogMessage(const std::string& message) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  for (auto& observer : observers_)
    observer.OnLogMessage(this, message);
}

void BraveSyncServiceImpl::NotifySyncStateChanged() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  for (auto& observer : observers_)
    observer.OnSyncStateChanged(this);
}

void BraveSyncServiceImpl::NotifyHaveSyncWords(
    const std::string& sync_words) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  for (auto& observer : observers_)
    observer.OnHaveSyncWords(this, sync_words);
}

void BraveSyncServiceImpl::OnExtensionReady(
    content::BrowserContext* browser_context,
    const extensions::Extension* extension) {
  if (extension->id() == brave_sync_extension_id) {
    StartLoop();
  }
}

void BraveSyncServiceImpl::OnExtensionUnloaded(
    content::BrowserContext* browser_context,
    const extensions::Extension* extension,
    extensions::UnloadedExtensionReason reason) {
  if (extension->id() == brave_sync_extension_id) {
    StopLoop();
  }
}

} // namespace brave_sync
