/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/brave_profile_sync_service_impl.h"

#include <algorithm>
#include <map>
#include <set>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_sync/brave_sync_prefs.h"
#include "brave/components/brave_sync/brave_sync_service_observer.h"
#include "brave/components/brave_sync/client/brave_sync_client_impl.h"
#include "brave/components/brave_sync/crypto/crypto.h"
#include "brave/components/brave_sync/jslib_const.h"
#include "brave/components/brave_sync/jslib_messages.h"
#include "brave/components/brave_sync/settings.h"
#include "brave/components/brave_sync/sync_devices.h"
#include "brave/components/brave_sync/syncer_helper.h"
#include "brave/components/brave_sync/tools.h"
#include "brave/components/brave_sync/values_conv.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sync/chrome_sync_client.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/prefs/pref_service.h"
#include "components/signin/public/identity_manager/account_info.h"
#include "components/sync/engine_impl/syncer.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/network_interfaces.h"
#include "ui/base/models/tree_node_iterator.h"

namespace brave_sync {

using browser_sync::ChromeSyncClient;
using jslib::Device;
using jslib::MetaInfo;
using jslib::SyncRecord;
using jslib_const::kBookmarks;
using jslib_const::kHistorySites;
using jslib_const::kPreferences;
using jslib_const::SyncObjectData_DEVICE;
using jslib_const::SyncRecordType_PREFERENCES;
using tools::IsTimeEmpty;

const std::vector<unsigned> BraveProfileSyncServiceImpl::kExponentialWaits = {
    10, 20, 40, 80};
const int BraveProfileSyncServiceImpl::kMaxSendRetries =
    BraveProfileSyncServiceImpl::kExponentialWaits.size() - 1;

namespace {

AccountInfo GetDummyAccountInfo() {
  AccountInfo account_info;
  account_info.account_id = CoreAccountId::FromString("dummy_account_id");
  return account_info;
}

void NotifyLogMessage(const std::string& message) {
  DLOG(INFO) << message;
}

std::string GetDeviceName() {
  std::string hostname = net::GetHostName();
  if (hostname.empty()) {
#if defined(OS_MACOSX)
    hostname = std::string("Mac Desktop");
#elif defined(OS_LINUX)
    hostname = std::string("Linux Desktop");
#elif defined(OS_WIN)
    hostname = std::string("Windows Desktop");
#endif
  }
  return hostname;
}

RecordsListPtr CreateDeviceRecord(const std::string& device_name,
                                  const std::string& object_id,
                                  const SyncRecord::Action& action,
                                  const std::string& device_id,
                                  const std::string& device_id_v2) {
  RecordsListPtr records = std::make_unique<RecordsList>();

  SyncRecordPtr record = std::make_unique<SyncRecord>();

  record->action = action;
  record->deviceId = device_id;
  record->objectId = object_id;
  record->objectData = SyncObjectData_DEVICE;  // "device"

  std::unique_ptr<Device> device = std::make_unique<Device>();
  device->name = device_name;
  device->deviceIdV2 = device_id_v2;
  record->SetDevice(std::move(device));

  records->emplace_back(std::move(record));

  return records;
}

const bookmarks::BookmarkNode* FindByObjectId(bookmarks::BookmarkModel* model,
                                              const std::string& object_id) {
  ui::TreeNodeIterator<const bookmarks::BookmarkNode> iterator(
      model->root_node());
  while (iterator.has_next()) {
    const bookmarks::BookmarkNode* node = iterator.Next();
    std::string node_object_id;
    node->GetMetaInfo("object_id", &node_object_id);

    if (!node_object_id.empty() && object_id == node_object_id)
      return node;
  }
  return nullptr;
}

std::unique_ptr<SyncRecord> CreateDeleteBookmarkByObjectId(
    const prefs::Prefs* brave_sync_prefs,
    const std::string& object_id) {
  auto record = std::make_unique<SyncRecord>();
  record->deviceId = brave_sync_prefs->GetThisDeviceId();
  record->objectData = jslib_const::SyncObjectData_BOOKMARK;
  record->objectId = object_id;
  record->action = jslib::SyncRecord::Action::A_DELETE;
  record->syncTimestamp = base::Time::Now();
  auto bookmark = std::make_unique<jslib::Bookmark>();
  record->SetBookmark(std::move(bookmark));
  return record;
}

void DoDispatchGetRecordsCallback(
    GetRecordsCallback cb,
    std::unique_ptr<brave_sync::RecordsList> records) {
  std::move(cb).Run(std::move(records));
}

void AddSyncEntityInfo(jslib::Bookmark* bookmark,
                       const bookmarks::BookmarkNode* node,
                       const std::string& key) {
  std::string value;
  if (node->GetMetaInfo(key, &value)) {
    MetaInfo metaInfo;
    metaInfo.key = key;
    metaInfo.value = value;
    bookmark->metaInfo.push_back(metaInfo);
  }
}

SyncRecordPtr PrepareResolvedDevice(SyncDevice* device,
                                    SyncRecord::Action action) {
  auto record = std::make_unique<jslib::SyncRecord>();
  record->action = action;
  record->deviceId = device->device_id_;
  record->objectId = device->object_id_;
  record->objectData = jslib_const::SyncObjectData_DEVICE;  // "device"
  std::unique_ptr<jslib::Device> device_record =
      std::make_unique<jslib::Device>();
  device_record->name = device->name_;
  device_record->deviceIdV2 = device->device_id_v2_;
  record->SetDevice(std::move(device_record));
  return record;
}

using NodesSet = std::set<const bookmarks::BookmarkNode*>;
using ObjectIdToNodes = std::map<std::string, NodesSet>;

void FillObjectsMap(const bookmarks::BookmarkNode* parent,
                    ObjectIdToNodes* object_id_nodes) {
  for (size_t i = 0; i < parent->children().size(); ++i) {
    const bookmarks::BookmarkNode* current_child = parent->children()[i].get();
    std::string object_id;
    if (current_child->GetMetaInfo("object_id", &object_id) &&
        !object_id.empty()) {
      (*object_id_nodes)[object_id].insert(current_child);
    }
    if (current_child->is_folder()) {
      FillObjectsMap(current_child, object_id_nodes);
    }
  }
}

void AddDeletedChildren(const BookmarkNode* node, NodesSet* deleted_nodes) {
  for (const auto& child : node->children()) {
    deleted_nodes->insert(child.get());
    if (node->is_folder()) {
      AddDeletedChildren(child.get(), deleted_nodes);
    }
  }
}

void ClearDuplicatedNodes(ObjectIdToNodes* object_id_nodes,
                          bookmarks::BookmarkModel* model) {
  size_t nodes_recreated = 0;
  NodesSet nodes_with_duplicates;
  for (ObjectIdToNodes::iterator it_object_id = object_id_nodes->begin();
       it_object_id != object_id_nodes->end(); ++it_object_id) {
    const NodesSet& nodes = it_object_id->second;
    if (nodes.size() > 1) {
      nodes_with_duplicates.insert(nodes.begin(), nodes.end());
    }
  }

  NodesSet deleted_nodes;
  for (const bookmarks::BookmarkNode* node : nodes_with_duplicates) {
    if (deleted_nodes.find(node) != deleted_nodes.end()) {
      // Node already has been deleted
      continue;
    }

    deleted_nodes.insert(node);
    if (node->is_folder()) {
      AddDeletedChildren(node, &deleted_nodes);
    }

    const auto* parent = node->parent();
    size_t original_index = parent->GetIndexOf(node);
    VLOG(1) << "[BraveSync] " << __func__
            << " Copying node into index=" << original_index;
    model->Copy(node, parent, original_index);
    VLOG(1) << "[BraveSync] " << __func__ << " Removing original node";
    model->Remove(node);
    nodes_recreated++;
  }

  VLOG(1) << "[BraveSync] " << __func__
          << " done nodes_recreated=" << nodes_recreated;
}

}  // namespace

BraveProfileSyncServiceImpl::BraveProfileSyncServiceImpl(Profile* profile,
                                                         InitParams init_params)
    : BraveProfileSyncService(std::move(init_params)),
      brave_sync_client_(BraveSyncClient::Create(this, profile)) {
  brave_sync_prefs_ =
      std::make_unique<prefs::Prefs>(sync_client_->GetPrefService());

  // Monitor syncs prefs required in GetSettingsAndDevices
  brave_pref_change_registrar_.Init(sync_client_->GetPrefService());
  brave_pref_change_registrar_.Add(
      prefs::kSyncEnabled,
      base::Bind(&BraveProfileSyncServiceImpl::OnBraveSyncPrefsChanged,
                 base::Unretained(this)));
  brave_pref_change_registrar_.Add(
      prefs::kSyncDeviceName,
      base::Bind(&BraveProfileSyncServiceImpl::OnBraveSyncPrefsChanged,
                 base::Unretained(this)));
  brave_pref_change_registrar_.Add(
      prefs::kSyncDeviceList,
      base::Bind(&BraveProfileSyncServiceImpl::OnBraveSyncPrefsChanged,
                 base::Unretained(this)));
  brave_pref_change_registrar_.Add(
      prefs::kSyncBookmarksEnabled,
      base::Bind(&BraveProfileSyncServiceImpl::OnBraveSyncPrefsChanged,
                 base::Unretained(this)));
  brave_pref_change_registrar_.Add(
      prefs::kSyncSiteSettingsEnabled,
      base::Bind(&BraveProfileSyncServiceImpl::OnBraveSyncPrefsChanged,
                 base::Unretained(this)));
  brave_pref_change_registrar_.Add(
      prefs::kSyncHistoryEnabled,
      base::Bind(&BraveProfileSyncServiceImpl::OnBraveSyncPrefsChanged,
                 base::Unretained(this)));

  model_ = BookmarkModelFactory::GetForBrowserContext(profile);
  // model_ can be null in some tests

  network_connection_tracker_->AddNetworkConnectionObserver(this);
  RecordSyncStateP3A();
}

void BraveProfileSyncServiceImpl::BookmarkModelLoaded(BookmarkModel* model,
                                                      bool ids_reassigned) {
  VLOG(2) << "[BraveSync] bookmarks model just loaded, "
          << "resuming pending sync ready callback";
  OnSyncReadyBookmarksModelLoaded();
}

void BraveProfileSyncServiceImpl::OnNudgeSyncCycle(RecordsListPtr records) {
  if (!brave_sync_prefs_->GetSyncEnabled())
    return;

  for (auto& record : *records) {
    record->deviceId = brave_sync_prefs_->GetThisDeviceId();
    CheckOtherBookmarkRecord(record.get());
    CheckOtherBookmarkChildRecord(record.get());
  }
  if (!records->empty()) {
    SendSyncRecords(jslib_const::SyncRecordType_BOOKMARKS, std::move(records));
  }
}

BraveProfileSyncServiceImpl::~BraveProfileSyncServiceImpl() {
  network_connection_tracker_->RemoveNetworkConnectionObserver(this);
  // Tests which use ProfileSyncService and are not configured to run on UI
  // thread, fire DCHECK on BookmarkModel::RemoveObserver at a wrong sequence.
  // Remove observer only if we have set it.
  if (is_model_loaded_observer_set_) {
      model_->RemoveObserver(this);
  }
}

void BraveProfileSyncServiceImpl::OnSetupSyncHaveCode(
    const std::string& sync_words,
    const std::string& device_name) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (sync_words.empty()) {
    OnSyncSetupError("ERR_SYNC_WRONG_WORDS");
    return;
  }

  Uint8Array seed;
  if (!crypto::PassphraseToBytes32(sync_words, &seed)) {
    OnSyncSetupError("ERR_SYNC_WRONG_WORDS");
    return;
  }

  if (brave_sync_initializing_) {
    NotifyLogMessage("currently initializing");
    return;
  }

  if (!brave_sync_prefs_->GetSeed().empty()) {
    NotifyLogMessage("already configured");
    return;
  }

  DCHECK(!brave_sync_prefs_->GetSyncEnabled());

  if (device_name.empty())
    brave_sync_prefs_->SetThisDeviceName(GetDeviceName());
  else
    brave_sync_prefs_->SetThisDeviceName(device_name);
  brave_sync_initializing_ = true;
  brave_sync_prefs_->SetSyncEnabled(true);
  seed_ = seed;
}

void BraveProfileSyncServiceImpl::OnSetupSyncNewToSync(
    const std::string& device_name) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (brave_sync_initializing_) {
    NotifyLogMessage("currently initializing");
    return;
  }

  if (!brave_sync_prefs_->GetSeed().empty()) {
    NotifyLogMessage("already configured");
    return;
  }

  DCHECK(!brave_sync_prefs_->GetSyncEnabled());

  if (device_name.empty())
    brave_sync_prefs_->SetThisDeviceName(GetDeviceName());
  else
    brave_sync_prefs_->SetThisDeviceName(device_name);

  brave_sync_initializing_ = true;

  brave_sync_prefs_->SetSyncEnabled(true);
}

void BraveProfileSyncServiceImpl::OnDeleteDevice(
    const std::string& device_id_v2) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  auto sync_devices = brave_sync_prefs_->GetSyncDevices();

  const SyncDevice* device = sync_devices->GetByDeviceIdV2(device_id_v2);
  if (device) {
    const std::string device_name = device->name_;
    const std::string device_id = device->device_id_;
    const std::string object_id = device->object_id_;
    SendDeviceSyncRecord(SyncRecord::Action::A_DELETE, device_name, device_id,
                         device_id_v2, object_id);
    if (device_id_v2 == brave_sync_prefs_->GetThisDeviceIdV2()) {
      // Mark state we have sent DELETE for own device and we are going to
      // call ResetSyncInternal() at OnRecordsSent after ensuring we had made
      // a proper attemp to send the record
      pending_self_reset_ = true;
    }
    FetchDevices();
  }
}

void BraveProfileSyncServiceImpl::OnResetSync() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  auto sync_devices = brave_sync_prefs_->GetSyncDevices();

  if (sync_devices->size() == 0) {
    // Fail safe option
    VLOG(2) << "[Sync] " << __func__ << " unexpected zero device size";
    ResetSyncInternal();
  } else {
    // We have to send delete record and wait for library deleted response then
    // we can reset it by ResetSyncInternal()
    const std::string device_id_v2 = brave_sync_prefs_->GetThisDeviceIdV2();
    OnDeleteDevice(device_id_v2);
  }
}

void BraveProfileSyncServiceImpl::GetSettingsAndDevices(
    const GetSettingsAndDevicesCallback& callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  auto settings = brave_sync_prefs_->GetBraveSyncSettings();
  auto devices = brave_sync_prefs_->GetSyncDevices();
  callback.Run(std::move(settings), std::move(devices));
}

void BraveProfileSyncServiceImpl::GetSyncWords() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  Uint8Array seed = Uint8ArrayFromString(brave_sync_prefs_->GetSeed());
  NotifyHaveSyncWords(crypto::PassphraseFromBytes32(seed));
}

std::string BraveProfileSyncServiceImpl::GetSeed() {
  return brave_sync_prefs_->GetSeed();
}

void BraveProfileSyncServiceImpl::OnSetSyncEnabled(
    const bool sync_this_device) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  brave_sync_prefs_->SetSyncEnabled(sync_this_device);
}

void BraveProfileSyncServiceImpl::OnSetSyncBookmarks(
    const bool sync_bookmarks) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  syncer::UserSelectableTypeSet type_set =
      ProfileSyncService::GetUserSettings()->GetSelectedTypes();
  if (sync_bookmarks)
    type_set.Put(syncer::UserSelectableType::kBookmarks);
  else
    type_set.Remove(syncer::UserSelectableType::kBookmarks);
  ProfileSyncService::GetUserSettings()->SetSelectedTypes(false, type_set);
  if (brave_sync_prefs_->GetSyncBookmarksEnabled() != sync_bookmarks)
    brave_sync_prefs_->SetSyncBookmarksEnabled(sync_bookmarks);
}

void BraveProfileSyncServiceImpl::OnSetSyncBrowsingHistory(
    const bool sync_browsing_history) {
  brave_sync_prefs_->SetSyncHistoryEnabled(sync_browsing_history);
}

void BraveProfileSyncServiceImpl::OnSetSyncSavedSiteSettings(
    const bool sync_saved_site_settings) {
  brave_sync_prefs_->SetSyncSiteSettingsEnabled(sync_saved_site_settings);
}

void BraveProfileSyncServiceImpl::BackgroundSyncStarted(bool startup) {}

void BraveProfileSyncServiceImpl::BackgroundSyncStopped(bool shutdown) {}

void BraveProfileSyncServiceImpl::OnSyncDebug(const std::string& message) {
  NotifyLogMessage(message);
}

void BraveProfileSyncServiceImpl::OnSyncSetupError(const std::string& error) {
  if (brave_sync_initializing_) {
    brave_sync_prefs_->Clear();
    brave_sync_initializing_ = false;
  }
  NotifySyncSetupError(error);
}

void BraveProfileSyncServiceImpl::OnGetInitData(
    const std::string& sync_version) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  Uint8Array seed;
  if (!seed_.empty()) {
    seed = seed_;
  } else if (!brave_sync_prefs_->GetSeed().empty()) {
    seed = Uint8ArrayFromString(brave_sync_prefs_->GetSeed());
    VLOG(1) << "[Brave Sync] Init from prefs";
  } else {
    VLOG(1) << "[Brave Sync] Init new chain";
  }

  Uint8Array device_id;
  if (!brave_sync_prefs_->GetThisDeviceId().empty()) {
    device_id = Uint8ArrayFromString(brave_sync_prefs_->GetThisDeviceId());
    VLOG(1) << "[Brave Sync] Init device id from prefs: "
            << StrFromUint8Array(device_id);
  } else {
    VLOG(1) << "[Brave Sync] Init empty device id";
  }

  std::string device_id_v2;
  if (!brave_sync_prefs_->GetThisDeviceIdV2().empty()) {
    device_id_v2 = brave_sync_prefs_->GetThisDeviceIdV2();
    VLOG(1) << "[Brave Sync] Init device id_v2 from prefs: " << device_id_v2;
  } else {
    VLOG(1) << "[Brave Sync] Init empty device id_v2";
  }

  DCHECK(!sync_version.empty());
  // TODO(bridiver) - this seems broken because using the version we get back
  // from the server (currently v1.4.2) causes things to break. What is the
  // the point of having this value?
  brave_sync_prefs_->SetApiVersion("0");

  client_data::Config config;
  config.api_version = brave_sync_prefs_->GetApiVersion();
  config.server_url = BRAVE_SYNC_ENDPOINT;
  config.debug = true;
  brave_sync_client_->SendGotInitData(seed, device_id, config, device_id_v2);
}

void BraveProfileSyncServiceImpl::OnSaveInitData(
    const Uint8Array& seed,
    const Uint8Array& device_id,
    const std::string& device_id_v2) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(!brave_sync_ready_);
  // OnSaveInitData will not only be triggered by OnSetupSyncNewToSync or
  // OnSetupSyncHaveCode, we use it to migrate device which doesn't have
  // deviceIdV2

  std::string seed_str = StrFromUint8Array(seed);
  std::string device_id_str = StrFromUint8Array(device_id);

  seed_.clear();
  DCHECK(!seed_str.empty());

  brave_sync_prefs_->SetSeed(seed_str);
  brave_sync_prefs_->SetThisDeviceId(device_id_str);
  if (!brave_sync_initializing_ &&
      brave_sync_prefs_->GetThisDeviceIdV2().empty())
    send_device_id_v2_update_ = true;
  brave_sync_prefs_->SetThisDeviceIdV2(device_id_v2);

  brave_sync_initializing_ = false;
}

void BraveProfileSyncServiceImpl::OnSyncReady() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  const std::string bookmarks_base_order =
      brave_sync_prefs_->GetBookmarksBaseOrder();
  if (bookmarks_base_order.empty()) {
    std::string platform = tools::GetPlatformName();
    brave_sync_client_->SendGetBookmarksBaseOrder(
        brave_sync_prefs_->GetThisDeviceId(), platform);
    // OnSyncReady will be called by OnSaveBookmarksBaseOrder
    return;
  }

  DCHECK(false == brave_sync_ready_);
  brave_sync_ready_ = true;

  DCHECK(model_);
  if (model_->loaded()) {
    OnSyncReadyBookmarksModelLoaded();
  } else {
    // Will call OnSyncReadyBookmarksModelLoaded once model is loaded
    VLOG(2) << "[BraveSync] bookmarks model is not yet loaded, "
            << "OnSyncReady will be delayed";
    model_->AddObserver(this);
    is_model_loaded_observer_set_ = true;
  }
}

void BraveProfileSyncServiceImpl::OnSyncReadyBookmarksModelLoaded() {
  // For launching from legacy sync profile and also brand new profile
  if (brave_sync_prefs_->GetMigratedBookmarksVersion() < 2)
    SetPermanentNodesOrder(brave_sync_prefs_->GetBookmarksBaseOrder());

  syncer::SyncPrefs sync_prefs(sync_client_->GetPrefService());
  // first time setup sync or migrated from legacy sync
  if (sync_prefs.GetLastSyncedTime().is_null()) {
    ProfileSyncService::GetUserSettings()->SetSelectedTypes(
        false, syncer::UserSelectableTypeSet());
    // default enable bookmark
    // this is important, don't change
    // to brave_sync_prefs_->SetSyncBookmarksEnabled(true);
    OnSetSyncBookmarks(true);
    ProfileSyncService::GetUserSettings()->SetSyncRequested(true);
  }

  if (!sync_client_->GetPrefService()->GetBoolean(kOtherBookmarksMigrated)) {
    BraveMigrateOtherNodeFolder(model_);
    sync_client_->GetPrefService()->SetBoolean(kOtherBookmarksMigrated, true);
  }
}

// static
void BraveProfileSyncServiceImpl::AddNonClonedBookmarkKeys(
    BookmarkModel* model) {
  DCHECK(model);
  DCHECK(model->loaded());
  model->AddNonClonedKey("object_id");
  model->AddNonClonedKey("order");
  model->AddNonClonedKey("sync_timestamp");
  model->AddNonClonedKey("version");
}

syncer::ModelTypeSet BraveProfileSyncServiceImpl::GetPreferredDataTypes()
    const {
  // Force DEVICE_INFO type to have nudge cycle each time to fetch
  // Brave sync devices.
  // Will be picked up by ProfileSyncService::ConfigureDataTypeManager
  return Union(ProfileSyncService::GetPreferredDataTypes(),
               {syncer::DEVICE_INFO});
}

std::unique_ptr<SyncRecordAndExistingList>
BraveProfileSyncServiceImpl::PrepareResolvedPreferences(
    const RecordsList& records) {
  auto sync_devices = brave_sync_prefs_->GetSyncDevices();
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

void BraveProfileSyncServiceImpl::OnGetExistingObjects(
    const std::string& category_name,
    std::unique_ptr<RecordsList> records,
    const base::Time& last_record_time_stamp,
    const bool is_truncated) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  // TODO(bridiver) - what do we do with is_truncated ?
  // It appears to be ignored in b-l
  if (category_name == kBookmarks) {
    DCHECK(model_->loaded());
    if (!IsTimeEmpty(last_record_time_stamp)) {
      brave_sync_prefs_->SetLatestRecordTime(last_record_time_stamp);
    }
    auto records_and_existing_objects =
        std::make_unique<SyncRecordAndExistingList>();
    CreateResolveList(*records.get(), records_and_existing_objects.get());
    brave_sync_client_->SendResolveSyncRecords(
        category_name, std::move(records_and_existing_objects));
  } else if (category_name == kPreferences) {
    if (!tools::IsTimeEmpty(last_record_time_stamp)) {
      brave_sync_prefs_->SetLatestDeviceRecordTime(last_record_time_stamp);
    }
    auto existing_records = PrepareResolvedPreferences(*records.get());
    brave_sync_client_->SendResolveSyncRecords(category_name,
                                               std::move(existing_records));
  }
}

void BraveProfileSyncServiceImpl::OnResolvedSyncRecords(
    const std::string& category_name,
    std::unique_ptr<RecordsList> records) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (category_name == jslib_const::kPreferences) {
    OnResolvedPreferences(*records.get());
  } else if (category_name == kBookmarks) {
    for (auto& record : *records) {
      if (IsOtherBookmarksFolder(record.get())) {
          bool pass_to_syncer = false;
          ProcessOtherBookmarksFolder(record.get(), &pass_to_syncer);
          if (!pass_to_syncer) {
            // We don't process "Other Bookmarks" folder in syncer when
            // "Other Bookmaks" doesn't need to be remapped.
            std::move(record);
            continue;
          }
      }
      ProcessOtherBookmarksChildren(record.get());
      LoadSyncEntityInfo(record.get());
      // We have to cache records when this function is triggered during
      // non-PollCycle (ex. compaction update) and wait for next available poll
      // cycle to have valid get_record_cb_
      if (!pending_received_records_)
        pending_received_records_ = std::make_unique<RecordsList>();
      pending_received_records_->push_back(std::move(record));
    }

    // Send records to syncer
    if (get_record_cb_) {
      backend_task_runner_->PostTask(
          FROM_HERE, base::BindOnce(&DoDispatchGetRecordsCallback,
                                    std::move(get_record_cb_),
                                    std::move(pending_received_records_)));
    }
    SignalWaitableEvent();
  } else if (category_name == kHistorySites) {
    NOTIMPLEMENTED();
  }
}

void BraveProfileSyncServiceImpl::OnDeletedSyncUser() {
  NOTIMPLEMENTED();
}

void BraveProfileSyncServiceImpl::OnDeleteSyncSiteSettings() {
  NOTIMPLEMENTED();
}

void BraveProfileSyncServiceImpl::OnSaveBookmarksBaseOrder(
    const std::string& order) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(!order.empty());
  brave_sync_prefs_->SetBookmarksBaseOrder(order);
  OnSyncReady();
}

void BraveProfileSyncServiceImpl::OnCompactComplete(
    const std::string& category) {
  if (category == kBookmarks)
    brave_sync_prefs_->SetLastCompactTimeBookmarks(base::Time::Now());
}

void BraveProfileSyncServiceImpl::OnRecordsSent(
    const std::string& category,
    std::unique_ptr<brave_sync::RecordsList> records) {
  if (category == kBookmarks) {
    for (auto& record : *records) {
      // Remove Acked sent records
      brave_sync_prefs_->RemoveFromRecordsToResend(record->objectId);
    }
  } else if (category == kPreferences && pending_self_reset_) {
    ResetSyncInternal();
    pending_self_reset_ = false;
  }
}

syncer::SyncService::DisableReasonSet
BraveProfileSyncServiceImpl::GetDisableReasons() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // legacy sync only support bookmark sync so we have to wait for migration
  // complete before enable sync engine
  if (IsBraveSyncEnabled() &&
      brave_sync_prefs_->GetMigratedBookmarksVersion() >= 2)
    return syncer::SyncService::DisableReasonSet();
  // kSyncManaged is set by Brave so it will contain
  // DISABLE_REASON_ENTERPRISE_POLICY and
  // SaveCardBubbleControllerImpl::ShouldShowSignInPromo will return false.
  return ProfileSyncService::GetDisableReasons();
}

CoreAccountInfo BraveProfileSyncServiceImpl::GetAuthenticatedAccountInfo()
    const {
  return GetDummyAccountInfo();
}

bool BraveProfileSyncServiceImpl::IsAuthenticatedAccountPrimary() const {
  return true;
}

void BraveProfileSyncServiceImpl::OnConnectionChanged(
    network::mojom::ConnectionType type) {
  if (type == network::mojom::ConnectionType::CONNECTION_NONE)
    SignalWaitableEvent();
}

void BraveProfileSyncServiceImpl::Shutdown() {
  SignalWaitableEvent();
  syncer::ProfileSyncService::Shutdown();
}

void BraveProfileSyncServiceImpl::NotifySyncSetupError(
    const std::string& error) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  for (auto& observer : BraveSyncService::observers_)
    observer.OnSyncSetupError(this, error);
}

void BraveProfileSyncServiceImpl::NotifySyncStateChanged() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  for (auto& observer : BraveSyncService::observers_)
    observer.OnSyncStateChanged(this);
}

void BraveProfileSyncServiceImpl::NotifyHaveSyncWords(
    const std::string& sync_words) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  for (auto& observer : BraveSyncService::observers_)
    observer.OnHaveSyncWords(this, sync_words);
}

void BraveProfileSyncServiceImpl::ResetSyncInternal() {
  SignalWaitableEvent();
  brave_sync_prefs_->Clear();

  brave_sync_ready_ = false;

  ProfileSyncService::GetUserSettings()->SetSyncRequested(false);
  ProfileSyncService::StopAndClear();

  // brave sync doesn't support pause sync so treating every new sync chain as
  // first time setup
  syncer::SyncPrefs sync_prefs(sync_client_->GetPrefService());
  sync_prefs.SetLastSyncedTime(base::Time());
}

void BraveProfileSyncServiceImpl::SetPermanentNodesOrder(
    const std::string& base_order) {
  DCHECK(model_);
  DCHECK(model_->loaded());
  DCHECK(!base_order.empty());
  std::string order;
  model_->bookmark_bar_node()->GetMetaInfo("order", &order);
  if (order.empty()) {
    tools::AsMutable(model_->bookmark_bar_node())
      ->SetMetaInfo("order", base_order + "1");
  }
  order.clear();
  model_->other_node()->GetMetaInfo("order", &order);
  if (order.empty()) {
    tools::AsMutable(model_->other_node())->SetMetaInfo("order",
                                                        tools::kOtherNodeOrder);
  }
  brave_sync_prefs_->SetMigratedBookmarksVersion(2);
}

// static
void BraveProfileSyncServiceImpl::MigrateDuplicatedBookmarksObjectIds(
    Profile* profile,
    BookmarkModel* model) {
  DCHECK(model);
  DCHECK(model->loaded());

  int migrated_version = profile->GetPrefs()->GetInteger(
      prefs::kDuplicatedBookmarksMigrateVersion);

  if (migrated_version >= 2) {
    return;
  }

  // Copying bookmarks through brave://bookmarks page could duplicate brave sync
  // metadata, which caused crash during chromium sync run
  // Go through nodes and re-create those ones who have duplicated object_id
  ObjectIdToNodes object_id_nodes;
  FillObjectsMap(model->root_node(), &object_id_nodes);
  ClearDuplicatedNodes(&object_id_nodes, model);

  profile->GetPrefs()->SetInteger(prefs::kDuplicatedBookmarksMigrateVersion, 2);
}

std::unique_ptr<SyncRecord>
BraveProfileSyncServiceImpl::BookmarkNodeToSyncBookmark(
    const bookmarks::BookmarkNode* node) {
  if (node->is_permanent_node() || !node->parent())
    return std::unique_ptr<SyncRecord>();

  auto record = std::make_unique<SyncRecord>();
  record->deviceId = brave_sync_prefs_->GetThisDeviceId();
  record->objectData = jslib_const::SyncObjectData_BOOKMARK;

  auto bookmark = std::make_unique<jslib::Bookmark>();
  bookmark->site.location = node->url().spec();
  bookmark->site.title = base::UTF16ToUTF8(node->GetTitledUrlNodeTitle());
  bookmark->site.customTitle = base::UTF16ToUTF8(node->GetTitle());
  // bookmark->site.lastAccessedTime - ignored
  bookmark->site.creationTime = node->date_added();
  bookmark->site.favicon = node->icon_url() ? node->icon_url()->spec() : "";
  bookmark->isFolder = node->type() != bookmarks::BookmarkNode::URL;
  bookmark->hideInToolbar = node->parent() == model_->other_node();

  std::string object_id;
  node->GetMetaInfo("object_id", &object_id);
  record->objectId = object_id;

  std::string parent_object_id;
  node->parent()->GetMetaInfo("object_id", &parent_object_id);
  bookmark->parentFolderObjectId = parent_object_id;

  std::string order;
  node->GetMetaInfo("order", &order);
  DCHECK(!order.empty());
  bookmark->order = order;

  std::string sync_timestamp;
  node->GetMetaInfo("sync_timestamp", &sync_timestamp);
  DCHECK(!sync_timestamp.empty());

  record->syncTimestamp = base::Time::FromJsTime(std::stod(sync_timestamp));

  record->action = jslib::SyncRecord::Action::A_UPDATE;

  AddSyncEntityInfo(bookmark.get(), node, "version");
  AddSyncEntityInfo(bookmark.get(), node, "position_in_parent");

  record->SetBookmark(std::move(bookmark));

  return record;
}

void BraveProfileSyncServiceImpl::SaveSyncEntityInfo(
    const jslib::SyncRecord* record) {
  auto* node = FindByObjectId(model_, record->objectId);
  // no need to save for DELETE
  if (node) {
    auto& bookmark = record->GetBookmark();
    for (auto& meta_info : bookmark.metaInfo) {
      if (meta_info.key == "version") {
        // Synchronize version meta info with CommitResponse
        int64_t version;
        bool result = base::StringToInt64(meta_info.value, &version);
        DCHECK(result);
        tools::AsMutable(node)
          ->SetMetaInfo(meta_info.key, std::to_string(++version));
      } else {
        tools::AsMutable(node)
        ->SetMetaInfo(meta_info.key, meta_info.value);
      }
    }
  }
}

void BraveProfileSyncServiceImpl::LoadSyncEntityInfo(
    jslib::SyncRecord* record) {
  auto* bookmark = record->mutable_bookmark();
  if (!bookmark->metaInfo.empty())
    return;
  auto* node = FindByObjectId(model_, record->objectId);
  if (node) {
    AddSyncEntityInfo(bookmark, node, "position_in_parent");
    AddSyncEntityInfo(bookmark, node, "version");
  } else {  // Assign base version metainfo for remotely created record
    MetaInfo metaInfo;
    metaInfo.key = "version";
    metaInfo.value = "0";
    bookmark->metaInfo.push_back(metaInfo);
  }
}

bool BraveProfileSyncServiceImpl::IsOtherBookmarksFolder(
    const jslib::SyncRecord* record) const {
  auto bookmark = record->GetBookmark();
  if (!bookmark.isFolder)
    return false;

  std::string other_node_object_id;
  if (model_->other_node()->GetMetaInfo("object_id", &other_node_object_id) &&
      record->objectId == other_node_object_id)
    return true;

  if (bookmark.order == tools::kOtherNodeOrder &&
      bookmark.site.title == tools::kOtherNodeName &&
      bookmark.site.customTitle == tools::kOtherNodeName) {
    return true;
  }

  return false;
}

void BraveProfileSyncServiceImpl::ProcessOtherBookmarksFolder(
    const jslib::SyncRecord* record,
    bool* pass_to_syncer) {
  std::string other_node_object_id;
    // Save object_id for late joined desktop to catch up with current id
    // iteration
  if (!model_->other_node()->GetMetaInfo("object_id", &other_node_object_id) &&
      record->action == jslib::SyncRecord::Action::A_CREATE) {
    tools::AsMutable(model_->other_node())->SetMetaInfo("object_id",
                                                        record->objectId);
  } else {
    // Out-of-date desktop will poll remote records before commiting local
    // changes so we won't get old iteration id. That is why we always take
    // remote id when it is different than what we have to catch up with current
    // iteration
    if (other_node_object_id != record->objectId) {
      tools::AsMutable(model_->other_node())->SetMetaInfo("object_id",
                                                          record->objectId);
    }
    // DELETE won't reach here, because [DELETE, null] => [] in
    // resolve-sync-objects but children records will go through. And we don't
    // need to regenerate new object id for it.

    // Handle MOVE, RENAME
    // REORDER (move under same parent) will be ignored
    // Update will be resolved as Create because [UPDATE, null] => [CREATE]
    auto bookmark = record->GetBookmark();
    if ((bookmark.order != tools::kOtherNodeOrder &&
         !bookmark.parentFolderObjectId.empty()) ||
        bookmark.site.title != tools::kOtherNodeName ||
        bookmark.site.customTitle != tools::kOtherNodeName) {
      // Generate next iteration object id from current object_id which will be
      // used to mapped normal folder
      tools::AsMutable(
        model_->other_node())
          ->SetMetaInfo("object_id",
                        tools::GenerateObjectIdForOtherNode(
                          other_node_object_id));
      *pass_to_syncer = true;

      // Add records to move direct children of other_node to this new folder
      // with existing object id of the old "Other Bookmarks" folder
      auto records_to_send = std::make_unique<RecordsList>();
      for (size_t i = 0; i < model_->other_node()->children().size(); ++i) {
        auto sync_record =
          BookmarkNodeToSyncBookmark(model_->other_node()->children()[i].get());
        sync_record->mutable_bookmark()->parentFolderObjectId =
          record->objectId;
        sync_record->mutable_bookmark()->hideInToolbar = false;
        sync_record->mutable_bookmark()->order =
          bookmark.order + "." + std::to_string(i + 1);
        LoadSyncEntityInfo(sync_record.get());

        auto record_to_send = SyncRecord::Clone(*sync_record);

        // Append changes to remote records
        if (!pending_received_records_)
          pending_received_records_ = std::make_unique<RecordsList>();
        pending_received_records_->push_back(std::move(sync_record));

        // Send changes to other desktops
        records_to_send->push_back(std::move(record_to_send));
      }
      SendSyncRecords(jslib_const::SyncRecordType_BOOKMARKS,
                      std::move(records_to_send));
    }
  }
}

void BraveProfileSyncServiceImpl::ProcessOtherBookmarksChildren(
    jslib::SyncRecord* record) {
  std::string other_node_object_id;
  if (model_->other_node()->GetMetaInfo("object_id", &other_node_object_id) &&
      record->GetBookmark().parentFolderObjectId == other_node_object_id) {
    record->mutable_bookmark()->hideInToolbar = true;
  }
}
void BraveProfileSyncServiceImpl::CheckOtherBookmarkRecord(
    jslib::SyncRecord* record) {
  if (!IsOtherBookmarksFolder(record))
    return;
  // Check if record has latest object id before sending
  std::string other_node_object_id;
  if (!model_->other_node()->GetMetaInfo("object_id", &other_node_object_id)) {
    // first iteration
    other_node_object_id = tools::GenerateObjectIdForOtherNode(std::string());
    tools::AsMutable(model_->other_node())->SetMetaInfo("object_id",
                                                        other_node_object_id);
  }
  DCHECK(!other_node_object_id.empty());
  if (record->objectId != other_node_object_id)
    record->objectId = other_node_object_id;
}

void BraveProfileSyncServiceImpl::CheckOtherBookmarkChildRecord(
    jslib::SyncRecord* record) {
  if (record->GetBookmark().hideInToolbar &&
      record->GetBookmark().parentFolderObjectId.empty()) {
    std::string other_node_object_id;
    model_->other_node()->GetMetaInfo("object_id", &other_node_object_id);
    DCHECK(!other_node_object_id.empty());
    record->mutable_bookmark()->parentFolderObjectId = other_node_object_id;
  }
}

void BraveProfileSyncServiceImpl::CreateResolveList(
    const std::vector<std::unique_ptr<SyncRecord>>& records,
    SyncRecordAndExistingList* records_and_existing_objects) {
  DCHECK(model_);
  DCHECK(model_->loaded());
  const auto& this_device_id = brave_sync_prefs_->GetThisDeviceId();
  for (const auto& record : records) {
    // Ignore records from ourselves to avoid mess on merge
    if (record->deviceId == this_device_id) {
      continue;
    }
    auto resolved_record = std::make_unique<SyncRecordAndExisting>();
    resolved_record->first = SyncRecord::Clone(*record);
    auto* node = FindByObjectId(model_, record->objectId);
    if (node) {
      resolved_record->second = BookmarkNodeToSyncBookmark(node);
    }

    records_and_existing_objects->push_back(std::move(resolved_record));
  }
}

bool BraveProfileSyncServiceImpl::IsSQSReady() const {
  // During 70 sec after device connected to chain use start_at parameter
  // of empty to force fetch from S3.
  // We need this to handle the case:
  // 1) deviceB connected to chain, created own queues
  // 2) deviceB made the first fetch from S3, got the records and set own
  //    non-empty latest_bookmark_record_time, so the next fetches would
  //    be done through SQS
  // 3) deviceA sends record
  // 4) lambda enumerates queues and could not discover queues from deviceB,
  //    because there is gap ~10~30 sec
  // 5) record does not arrive to queue of deviceB and is lost for deviceB
  // Any possibility of duplication will be eliminated by alreadySeenFromS3
  // checks in brave_sync/extension/brave-sync/lib/s3Helper.js.
  // Default Chromium fetch interval is 60 sec.
  // So during 70 sec after device connected to chain we forcing use S3ю
  if (tools::IsTimeEmpty(this_device_created_time_) ||
      (base::Time::Now() - this_device_created_time_).InSeconds() >= 70u) {
    return true;
  } else {
    return false;
  }
}

void BraveProfileSyncServiceImpl::FetchSyncRecords(const bool bookmarks,
                                                   const bool history,
                                                   const bool preferences,
                                                   int max_records) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(bookmarks || history || preferences);
  if (!(bookmarks || history || preferences)) {
    return;
  }

  std::vector<std::string> category_names;
  if (history) {
    category_names.push_back(kHistorySites);  // "HISTORY_SITES";
  }
  if (bookmarks) {
    category_names.push_back(kBookmarks);  // "BOOKMARKS";

    base::Time last_compact_time =
        brave_sync_prefs_->GetLastCompactTimeBookmarks();
    if (tools::IsTimeEmpty(last_compact_time) ||
        base::Time::Now() - last_compact_time >
            base::TimeDelta::FromDays(kCompactPeriodInDays)) {
      brave_sync_client_->SendCompact(kBookmarks);
    }
  }
  if (preferences) {
    category_names.push_back(kPreferences);  // "PREFERENCES";
  }

  base::Time start_at_time =
      IsSQSReady() ? brave_sync_prefs_->GetLatestRecordTime() : base::Time();

  brave_sync_client_->SendFetchSyncRecords(category_names, start_at_time,
                                           max_records);
}

void BraveProfileSyncServiceImpl::SendCreateDevice() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  std::string device_name = brave_sync_prefs_->GetThisDeviceName();
  std::string object_id = tools::GenerateObjectId();
  brave_sync_prefs_->SetThisDeviceObjectId(object_id);
  std::string device_id = brave_sync_prefs_->GetThisDeviceId();
  std::string device_id_v2 = brave_sync_prefs_->GetThisDeviceIdV2();
  DCHECK(!device_id_v2.empty());

  SendDeviceSyncRecord(SyncRecord::Action::A_CREATE, device_name, device_id,
                       device_id_v2, object_id);
}

void BraveProfileSyncServiceImpl::SendDeleteDevice() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  std::string device_name = brave_sync_prefs_->GetThisDeviceName();
  std::string object_id = brave_sync_prefs_->GetThisDeviceObjectId();
  std::string device_id = brave_sync_prefs_->GetThisDeviceId();
  std::string device_id_v2 = brave_sync_prefs_->GetThisDeviceIdV2();
  if (object_id.empty()) {
    auto sync_devices = brave_sync_prefs_->GetSyncDevices();
    std::vector<const SyncDevice*> devices =
        sync_devices->GetByDeviceId(device_id);
    for (auto* device : devices) {
      if (device) {
        object_id = device->object_id_;
      }
      SendDeviceSyncRecord(SyncRecord::Action::A_DELETE, device_name, device_id,
                           device_id_v2, object_id);
    }
    DCHECK(!object_id.empty());
  } else {
    DCHECK(!device_id_v2.empty());

    SendDeviceSyncRecord(SyncRecord::Action::A_DELETE, device_name, device_id,
                         device_id_v2, object_id);
  }
}

void BraveProfileSyncServiceImpl::SendDeviceSyncRecord(
    const int action,
    const std::string& device_name,
    const std::string& device_id,
    const std::string& device_id_v2,
    const std::string& object_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  RecordsListPtr records = CreateDeviceRecord(
      device_name, object_id, static_cast<SyncRecord::Action>(action),
      device_id, device_id_v2);
  SendSyncRecords(SyncRecordType_PREFERENCES, std::move(records));
}

void BraveProfileSyncServiceImpl::OnResolvedPreferences(
    const RecordsList& records) {
  const std::string this_device_object_id =
      brave_sync_prefs_->GetThisDeviceObjectId();
  const std::string this_device_id_v2 =
      brave_sync_prefs_->GetThisDeviceIdV2();
  bool this_device_deleted = false;

  auto sync_devices = brave_sync_prefs_->GetSyncDevices();
  for (const auto& record : records) {
    DCHECK(record->has_device() || record->has_sitesetting());
    if (record->has_device()) {
      bool actually_merged = false;
      auto& device = record->GetDevice();
      sync_devices->Merge(SyncDevice(record->GetDevice().name, record->objectId,
                                     record->deviceId, device.deviceIdV2,
                                     record->syncTimestamp.ToJsTime()),
                          record->action, &actually_merged);
      // We check object id here specifically because device which doesn't have
      // device id v2 also doesn't have this object id stored. So we use this
      // trait for migration.
      this_device_deleted =
          this_device_deleted ||
          (record->objectId == this_device_object_id &&
           device.deviceIdV2 == this_device_id_v2 &&
           record->action == SyncRecord::Action::A_DELETE && actually_merged);
    }
  }  // for each device

  brave_sync_prefs_->SetSyncDevices(*sync_devices);
  if (this_device_deleted) {
    ResetSyncInternal();
  }
}

void BraveProfileSyncServiceImpl::OnBraveSyncPrefsChanged(
    const std::string& pref) {
  if (pref == prefs::kSyncEnabled) {
    brave_sync_client_->OnSyncEnabledChanged();
    RecordSyncStateP3A();
  } else if (pref == prefs::kSyncDeviceList) {
    RecordSyncStateP3A();
  }
  NotifySyncStateChanged();
}

#if BUILDFLAG(ENABLE_EXTENSIONS)
BraveSyncClient* BraveProfileSyncServiceImpl::GetBraveSyncClient() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return brave_sync_client_.get();
}
#endif

bool BraveProfileSyncServiceImpl::IsBraveSyncEnabled() const {
  return brave_sync_prefs_->GetSyncEnabled();
}

void BraveProfileSyncServiceImpl::FetchDevices() {
  DCHECK(sync_client_);
  brave_sync_prefs_->SetLastFetchTime(base::Time::Now());

  base::Time start_at_time =
      IsSQSReady() ? brave_sync_prefs_->GetLatestDeviceRecordTime()
                   : base::Time();

  brave_sync_client_->SendFetchSyncRecords(
      {brave_sync::jslib_const::kPreferences}, start_at_time, 1000);
}

void BraveProfileSyncServiceImpl::OnPollSyncCycle(GetRecordsCallback cb,
                                                  base::WaitableEvent* wevent) {
  if (!brave_sync_prefs_->GetSyncEnabled())
    return;

  if (IsTimeEmpty(brave_sync_prefs_->GetLastFetchTime())) {
    SendCreateDevice();
    this_device_created_time_ = base::Time::Now();
  }
  if (send_device_id_v2_update_) {
    // Because device id might get duplicated and we didn't save object id for
    // this device so there is no way to send update to propagate device id v2,
    // We have to delete previous device records by device id and create a new
    // one.
    SendDeleteDevice();
    SendCreateDevice();
    send_device_id_v2_update_ = false;
  }

  FetchDevices();

  if (!brave_sync_ready_) {
    wevent->Signal();
    return;
  }

  get_record_cb_ = std::move(cb);
  wevent_ = wevent;

  const bool bookmarks = brave_sync_prefs_->GetSyncBookmarksEnabled();
  const bool history = brave_sync_prefs_->GetSyncHistoryEnabled();
  const bool preferences = brave_sync_prefs_->GetSyncSiteSettingsEnabled();
  FetchSyncRecords(bookmarks, history, preferences, 1000);
  ResendSyncRecords(jslib_const::SyncRecordType_BOOKMARKS);
}

void BraveProfileSyncServiceImpl::SignalWaitableEvent() {
  std::move(get_record_cb_);
  if (wevent_ && !wevent_->IsSignaled()) {
    wevent_->Signal();
    wevent_ = nullptr;
  }
}

BraveSyncService* BraveProfileSyncServiceImpl::GetSyncService() const {
  return static_cast<BraveSyncService*>(
      const_cast<BraveProfileSyncServiceImpl*>(this));
}

void BraveProfileSyncServiceImpl::SendSyncRecords(
    const std::string& category_name,
    RecordsListPtr records) {
  DCHECK(brave_sync_client_);
  brave_sync_client_->SendSyncRecords(category_name, *records);
  if (category_name == kBookmarks) {
    DCHECK(model_->loaded());
    for (auto& record : *records) {
      SaveSyncEntityInfo(record.get());
      std::unique_ptr<base::DictionaryValue> meta =
          std::make_unique<base::DictionaryValue>();
      meta->SetInteger("send_retry_number", 0);
      meta->SetDouble("sync_timestamp", record->syncTimestamp.ToJsTime());
      brave_sync_prefs_->AddToRecordsToResend(record->objectId,
                                              std::move(meta));
    }
  }
}

void BraveProfileSyncServiceImpl::ResendSyncRecords(
    const std::string& category_name) {
  if (category_name == kBookmarks) {
    RecordsListPtr records = std::make_unique<RecordsList>();
    std::vector<std::string> records_to_resend =
        brave_sync_prefs_->GetRecordsToResend();
    if (records_to_resend.empty())
      return;

    DCHECK(model_);
    DCHECK(model_->loaded());

    for (auto& object_id : records_to_resend) {
      auto* node = FindByObjectId(model_, object_id);

      // Check resend interval
      const base::DictionaryValue* meta =
          brave_sync_prefs_->GetRecordToResendMeta(object_id);
      DCHECK(meta);
      int current_retry_number = kMaxSendRetries;
      meta->GetInteger("send_retry_number", &current_retry_number);
      DCHECK_GE(current_retry_number, 0);
      double sync_timestamp = 0;
      meta->GetDouble("sync_timestamp", &sync_timestamp);
      DCHECK(!base::Time::FromJsTime(sync_timestamp).is_null());

      if ((base::Time::Now() - base::Time::FromJsTime(sync_timestamp)) <
          GetRetryExponentialWaitAmount(current_retry_number))
        continue;

      // Increase retry number
      if (++current_retry_number > kMaxSendRetries)
        current_retry_number = kMaxSendRetries;
      std::unique_ptr<base::DictionaryValue> new_meta =
          base::DictionaryValue::From(
              std::make_unique<base::Value>(meta->Clone()));
      new_meta->SetInteger("send_retry_number", current_retry_number);
      new_meta->SetDouble("sync_timestamp", base::Time::Now().ToJsTime());
      brave_sync_prefs_->SetRecordToResendMeta(object_id, std::move(new_meta));

      if (node) {
        records->push_back(BookmarkNodeToSyncBookmark(node));
      } else {
        records->push_back(
            CreateDeleteBookmarkByObjectId(brave_sync_prefs_.get(), object_id));
      }
    }
    if (!records->empty())
      brave_sync_client_->SendSyncRecords(category_name, *records);
  }
}

// static
base::TimeDelta BraveProfileSyncServiceImpl::GetRetryExponentialWaitAmount(
    int retry_number) {
  DCHECK_LE(retry_number, kMaxSendRetries);

  if (retry_number > kMaxSendRetries) {
    retry_number = kMaxSendRetries;
  }
  return base::TimeDelta::FromMinutes(kExponentialWaits[retry_number]);
}

// static
std::vector<unsigned>
BraveProfileSyncServiceImpl::GetExponentialWaitsForTests() {
  return kExponentialWaits;
}

void BraveProfileSyncServiceImpl::RecordSyncStateP3A() const {
  int result = 0;
  if (brave_sync_prefs_->GetSyncEnabled()) {
    unsigned long device_count =     // NOLINT
        static_cast<unsigned long>(  // NOLINT
            brave_sync_prefs_->GetSyncDevices()->size());
    // Answers are zero-based.
    result = std::min(device_count, 3UL) - 1UL;
  }
  UMA_HISTOGRAM_EXACT_LINEAR("Brave.Sync.Status", result, 2);
}

}  // namespace brave_sync
