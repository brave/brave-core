/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/brave_profile_sync_service.h"

#include "base/bind.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/brave_sync/values_conv.h"
#include "brave/components/brave_sync/brave_sync_prefs.h"
#include "brave/components/brave_sync/brave_sync_service_observer.h"
#include "brave/components/brave_sync/jslib_const.h"
#include "brave/components/brave_sync/jslib_messages.h"
#include "brave/components/brave_sync/settings.h"
#include "brave/components/brave_sync/syncer_helper.h"
#include "brave/components/brave_sync/sync_devices.h"
#include "brave/components/brave_sync/tools.h"
#include "chrome/browser/sync/chrome_sync_client.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/signin/core/browser/account_info.h"
#include "components/sync/engine_impl/syncer.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/network_interfaces.h"
#include "ui/base/models/tree_node_iterator.h"

namespace brave_sync {

using browser_sync::ChromeSyncClient;
using jslib::SyncRecord;
using jslib_const::kBookmarks;
using jslib_const::kHistorySites;
using jslib_const::kPreferences;
using jslib_const::SyncObjectData_DEVICE;
using jslib_const::SyncRecordType_PREFERENCES;
using jslib::Device;
using tools::IsTimeEmpty;


namespace {

AccountInfo GetDummyAccountInfo() {
  AccountInfo account_info;
  account_info.account_id = "dummy_account_id";
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

RecordsListPtr CreateDeviceCreationRecordExtension(
  const std::string& deviceName,
  const std::string& objectId,
  const SyncRecord::Action &action,
  const std::string& deviceId) {
  RecordsListPtr records = std::make_unique<RecordsList>();

  SyncRecordPtr record = std::make_unique<SyncRecord>();

  record->action = action;
  record->deviceId = deviceId;
  record->objectId = objectId;
  record->objectData = SyncObjectData_DEVICE;  // "device"

  std::unique_ptr<Device> device = std::make_unique<Device>();
  device->name = deviceName;
  record->SetDevice(std::move(device));

  records->emplace_back(std::move(record));

  return records;
}

const bookmarks::BookmarkNode* FindByObjectId(bookmarks::BookmarkModel* model,
                                        const std::string& object_id) {
  ui::TreeNodeIterator<const bookmarks::BookmarkNode>
      iterator(model->root_node());
  while (iterator.has_next()) {
    const bookmarks::BookmarkNode* node = iterator.Next();
    std::string node_object_id;
    node->GetMetaInfo("object_id", &node_object_id);

    if (!node_object_id.empty() && object_id == node_object_id)
      return node;
  }
  return nullptr;
}

std::unique_ptr<SyncRecord> BookmarkNodeToSyncBookmark(
    bookmarks::BookmarkModel* model,
    prefs::Prefs* brave_sync_prefs,
    const bookmarks::BookmarkNode* node,
    const SyncRecord::Action& action) {
  if (node->is_permanent_node() || !node->parent())
    return std::unique_ptr<SyncRecord>();

  auto record = std::make_unique<SyncRecord>();
  record->deviceId = brave_sync_prefs->GetThisDeviceId();
  record->objectData = jslib_const::SyncObjectData_BOOKMARK;

  auto bookmark = std::make_unique<jslib::Bookmark>();
  bookmark->site.location = node->url().spec();
  bookmark->site.title = base::UTF16ToUTF8(node->GetTitledUrlNodeTitle());
  bookmark->site.customTitle = base::UTF16ToUTF8(node->GetTitle());
  // bookmark->site.lastAccessedTime - ignored
  bookmark->site.creationTime = node->date_added();
  bookmark->site.favicon = node->icon_url() ? node->icon_url()->spec() : "";
  // Url may have type OTHER_NODE if it is in Deleted Bookmarks
  bookmark->isFolder = (node->type() != bookmarks::BookmarkNode::URL &&
                           node->type() != bookmarks::BookmarkNode::OTHER_NODE);
  bookmark->hideInToolbar =
      node->parent() != model->bookmark_bar_node();

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

  record->SetBookmark(std::move(bookmark));

  return record;
}

void CreateResolveList(
    const std::vector<std::unique_ptr<SyncRecord>>& records,
    SyncRecordAndExistingList* records_and_existing_objects,
    bookmarks::BookmarkModel* model,
    prefs::Prefs* brave_sync_prefs) {
  for (const auto& record : records) {
    auto resolved_record = std::make_unique<SyncRecordAndExisting>();
    resolved_record->first = SyncRecord::Clone(*record);
    auto* node = FindByObjectId(model, record->objectId);
    if (node) {
      resolved_record->second = BookmarkNodeToSyncBookmark(model,
                                                           brave_sync_prefs,
                                                           node,
                                                           record->action);
    }

    records_and_existing_objects->push_back(std::move(resolved_record));
  }
}

}   // namespace

BraveProfileSyncService::BraveProfileSyncService(InitParams init_params)
  : browser_sync::ProfileSyncService(std::move(init_params)) {
  brave_sync_words_ = std::string();
  brave_sync_prefs_ =
    std::make_unique<prefs::Prefs>(
      ProfileSyncService::GetSyncClient()->GetPrefService());
  GetBraveSyncClient()->set_sync_message_handler(this);

  // Moniter syncs prefs required in GetSettingsAndDevices
  brave_pref_change_registrar_.Init(
    ProfileSyncService::GetSyncClient()->GetPrefService());
  brave_pref_change_registrar_.Add(
      prefs::kSyncEnabled,
      base::Bind(&BraveProfileSyncService::OnBraveSyncPrefsChanged,
                 base::Unretained(this)));
  brave_pref_change_registrar_.Add(
      prefs::kSyncDeviceName,
      base::Bind(&BraveProfileSyncService::OnBraveSyncPrefsChanged,
                 base::Unretained(this)));
  brave_pref_change_registrar_.Add(
      prefs::kSyncDeviceList,
      base::Bind(&BraveProfileSyncService::OnBraveSyncPrefsChanged,
                 base::Unretained(this)));
  brave_pref_change_registrar_.Add(
      prefs::kSyncBookmarksEnabled,
      base::Bind(&BraveProfileSyncService::OnBraveSyncPrefsChanged,
                 base::Unretained(this)));
  brave_pref_change_registrar_.Add(
      prefs::kSyncSiteSettingsEnabled,
      base::Bind(&BraveProfileSyncService::OnBraveSyncPrefsChanged,
                 base::Unretained(this)));
  brave_pref_change_registrar_.Add(
      prefs::kSyncHistoryEnabled,
      base::Bind(&BraveProfileSyncService::OnBraveSyncPrefsChanged,
                 base::Unretained(this)));
  // TODO(darkdh): find another way to obtain bookmark model
  // change introduced in 83b9663e3814ef7e53af5009d10033b89955db44
  model_ = static_cast<ChromeSyncClient*>(
              ProfileSyncService::GetSyncClient())->GetBookmarkModel();

  if (!brave_sync_prefs_->GetSeed().empty() &&
      !brave_sync_prefs_->GetThisDeviceName().empty()) {
    brave_sync_configured_ = true;
  }

}

void BraveProfileSyncService::OnNudgeSyncCycle(
    RecordsListPtr records) {
  for (auto& record : *records) {
    record->deviceId = brave_sync_prefs_->GetThisDeviceId();
  }
  if (!records->empty())
    GetBraveSyncClient()->SendSyncRecords(
      jslib_const::SyncRecordType_BOOKMARKS, *records);
}

BraveProfileSyncService::~BraveProfileSyncService() {}

void BraveProfileSyncService::OnSetupSyncHaveCode(const std::string& sync_words,
    const std::string& device_name) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (sync_words.empty()) {
    OnSyncSetupError("ERR_SYNC_WRONG_WORDS");
    return;
  }

  if (brave_sync_initializing_) {
    NotifyLogMessage("currently initializing");
    return;
  }

  if (brave_sync_configured_) {
    NotifyLogMessage("already configured");
    return;
  }

  if (device_name.empty())
    brave_sync_prefs_->SetThisDeviceName(GetDeviceName());
  else
    brave_sync_prefs_->SetThisDeviceName(device_name);
  brave_sync_initializing_ = true;

  brave_sync_prefs_->SetSyncEnabled(true);
  brave_sync_words_ = sync_words;
}

void BraveProfileSyncService::OnSetupSyncNewToSync(
    const std::string& device_name) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (brave_sync_initializing_) {
    NotifyLogMessage("currently initializing");
    return;
  }

  if (brave_sync_configured_) {
    NotifyLogMessage("already configured");
    return;
  }

  // If the previous attempt was connect to sync chain
  // and failed to receive save-init-data
  brave_sync_words_.clear();

  if (device_name.empty())
    brave_sync_prefs_->SetThisDeviceName(GetDeviceName());
  else
    brave_sync_prefs_->SetThisDeviceName(device_name);

  brave_sync_initializing_ = true;

  brave_sync_prefs_->SetSyncEnabled(true);
}

void BraveProfileSyncService::OnDeleteDevice(const std::string& device_id) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  auto sync_devices = brave_sync_prefs_->GetSyncDevices();

  const SyncDevice *device = sync_devices->GetByDeviceId(device_id);
  if (device) {
    const std::string device_name = device->name_;
    const std::string object_id = device->object_id_;
    SendDeviceSyncRecord(
        SyncRecord::Action::A_DELETE, device_name, device_id, object_id);
  }
}

void BraveProfileSyncService::OnResetSync() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  auto sync_devices = brave_sync_prefs_->GetSyncDevices();

  if (sync_devices->size() == 0) {
    // Fail safe option
    VLOG(2) << "[Sync] " << __func__ << " unexpected zero device size";
    ResetSyncInternal();
  } else {
    // We have to send delete record and wait for library deleted response then
    // we can reset it by ResetInternal()
    const std::string device_id = brave_sync_prefs_->GetThisDeviceId();
    OnDeleteDevice(device_id);
  }
}

void BraveProfileSyncService::GetSettingsAndDevices(
    const GetSettingsAndDevicesCallback& callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  auto settings = brave_sync_prefs_->GetBraveSyncSettings();
  auto devices = brave_sync_prefs_->GetSyncDevices();
  callback.Run(std::move(settings), std::move(devices));
}

void BraveProfileSyncService::GetSyncWords() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  // Ask sync client
  std::string seed = brave_sync_prefs_->GetSeed();
  GetBraveSyncClient()->NeedSyncWords(seed);
}

std::string BraveProfileSyncService::GetSeed() {
  return brave_sync_prefs_->GetSeed();
}

void BraveProfileSyncService::OnSetSyncEnabled(const bool sync_this_device) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  brave_sync_prefs_->SetSyncEnabled(sync_this_device);
}

void BraveProfileSyncService::OnSetSyncBookmarks(const bool sync_bookmarks) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  syncer::ModelTypeSet type_set =
    ProfileSyncService::GetUserSettings()->GetChosenDataTypes();
  if (sync_bookmarks)
    type_set.Put(syncer::BOOKMARKS);
  else
    type_set.Remove(syncer::BOOKMARKS);
  ProfileSyncService::GetUserSettings()->SetChosenDataTypes(false,
                                                                type_set);
  brave_sync_prefs_->SetSyncBookmarksEnabled(sync_bookmarks);
}

void BraveProfileSyncService::OnSetSyncBrowsingHistory(
    const bool sync_browsing_history) {
  brave_sync_prefs_->SetSyncHistoryEnabled(sync_browsing_history);
}

void BraveProfileSyncService::OnSetSyncSavedSiteSettings(
    const bool sync_saved_site_settings) {
  brave_sync_prefs_->SetSyncSiteSettingsEnabled(sync_saved_site_settings);
}

void BraveProfileSyncService::BackgroundSyncStarted(bool startup) {
}

void BraveProfileSyncService::BackgroundSyncStopped(bool shutdown) {
}

void BraveProfileSyncService::OnSyncDebug(const std::string& message) {
  NotifyLogMessage(message);
}

void BraveProfileSyncService::OnSyncSetupError(const std::string& error) {
  if (brave_sync_initializing_) {
    brave_sync_prefs_->Clear();
    brave_sync_initializing_ = false;
  }
  NotifySyncSetupError(error);
}

void BraveProfileSyncService::OnGetInitData(const std::string& sync_version) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  Uint8Array seed;
  if (!brave_sync_words_.empty()) {
    VLOG(1) << "[Brave Sync] Init from sync words";
  } else if (!brave_sync_prefs_->GetSeed().empty()) {
    seed = Uint8ArrayFromString(brave_sync_prefs_->GetSeed());
    VLOG(1) << "[Brave Sync] Init from prefs";
  } else {
    VLOG(1) << "[Brave Sync] Init new chain";
  }

  Uint8Array device_id;
  if (!brave_sync_prefs_->GetThisDeviceId().empty()) {
    device_id = Uint8ArrayFromString(brave_sync_prefs_->GetThisDeviceId());
    VLOG(1) << "[Brave Sync] Init device id from prefs: " <<
        StrFromUint8Array(device_id);
  } else {
    VLOG(1) << "[Brave Sync] Init empty device id";
  }

  DCHECK(!sync_version.empty());
  // TODO(bridiver) - this seems broken because using the version we get back
  // from the server (currently v1.4.2) causes things to break. What is the
  // the point of having this value?
  brave_sync_prefs_->SetApiVersion("0");

  client_data::Config config;
  config.api_version = brave_sync_prefs_->GetApiVersion();
  config.server_url = "https://sync.brave.com";
  config.debug = true;
  GetBraveSyncClient()->SendGotInitData(seed, device_id, config,
                                        brave_sync_words_);
}

void BraveProfileSyncService::OnSaveInitData(const Uint8Array& seed,
                                        const Uint8Array& device_id) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(!brave_sync_initialized_);
  // If we are here and brave_sync_initializing_ is false, we have came
  // not from OnSetupSyncNewToSync or OnSetupSyncHaveCode.
  // One case is we put wrong code words and then restarted before cleared
  // kSyncEnabled pref. This should not happen.
  DCHECK(brave_sync_initializing_);

  std::string seed_str = StrFromUint8Array(seed);
  std::string device_id_str = StrFromUint8Array(device_id);

  std::string prev_seed_str = brave_sync_prefs_->GetPrevSeed();

  brave_sync_words_.clear();
  DCHECK(!seed_str.empty());

  if (prev_seed_str == seed_str) {  // reconnecting to previous sync chain
    brave_sync_prefs_->SetPrevSeed(std::string());
  } else if (!prev_seed_str.empty()) {  // connect/create to new sync chain
    // bookmark_change_processor_->Reset(true);
    brave_sync_prefs_->SetPrevSeed(std::string());
  } else {
    // This is not required, because when there is no previous seed, bookmarks
    // should not have a metadata. However, this is done by intention, to be
    // a remedy for cases when sync had been reset and prev_seed_str had been
    // cleared when it shouldn't (brave-browser#3188).
    // bookmark_change_processor_->Reset(true);
  }

  brave_sync_prefs_->SetSeed(seed_str);
  brave_sync_prefs_->SetThisDeviceId(device_id_str);

  brave_sync_configured_ = true;

  brave_sync_initializing_ = false;
}

void BraveProfileSyncService::OnSyncReady() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  const std::string bookmarks_base_order =
    brave_sync_prefs_->GetBookmarksBaseOrder();
  if (bookmarks_base_order.empty()) {
    std::string platform = tools::GetPlatformName();
    GetBraveSyncClient()->SendGetBookmarksBaseOrder(
                            brave_sync_prefs_->GetThisDeviceId(),
                            platform);
    // OnSyncReady will be called by OnSaveBookmarksBaseOrder
    return;
  }

  DCHECK(false == brave_sync_initialized_);
  brave_sync_initialized_ = true;

  // For launching from legacy sync profile and also brand new profile
  if (brave_sync_prefs_->GetMigratedBookmarksVersion() < 2)
    SetPermanentNodesOrder(brave_sync_prefs_->GetBookmarksBaseOrder());

  syncer::SyncPrefs sync_prefs(ProfileSyncService::GetSyncClient()
                                ->GetPrefService());
  // first time setup sync or migrated from legacy sync
  if (sync_prefs.GetLastSyncedTime().is_null()) {
    ProfileSyncService::GetUserSettings()
      ->SetChosenDataTypes(false, syncer::ModelTypeSet());
    // default enable bookmark
    OnSetSyncBookmarks(true);
    ProfileSyncService::GetUserSettings()->SetSyncRequested(true);
  }
}

void BraveProfileSyncService::OnGetExistingObjects(
    const std::string& category_name,
    std::unique_ptr<RecordsList> records,
    const base::Time &last_record_time_stamp,
    const bool is_truncated) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  // TODO(bridiver) - what do we do with is_truncated ?
  // It appears to be ignored in b-l
  if (!IsTimeEmpty(last_record_time_stamp)) {
    brave_sync_prefs_->SetLatestRecordTime(last_record_time_stamp);
  }

  if (category_name == kBookmarks) {
    auto records_and_existing_objects =
        std::make_unique<SyncRecordAndExistingList>();
    CreateResolveList(
        *records.get(), records_and_existing_objects.get(),
        model_,
        brave_sync_prefs_.get());
    GetBraveSyncClient()->SendResolveSyncRecords(
        category_name, std::move(records_and_existing_objects));
  }
}

void BraveProfileSyncService::OnResolvedSyncRecords(
    const std::string& category_name,
    std::unique_ptr<RecordsList> records) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (category_name == jslib_const::kPreferences) {
    OnResolvedPreferences(*records.get());
  } else if (category_name == kBookmarks) {
    // Send records to syncer
    if (get_record_cb_)
      ProfileSyncService::GetSyncEngine()->DispatchGetRecordsCallback(
                                            get_record_cb_, std::move(records));
    SignalWaitableEvent();
  } else if (category_name == kHistorySites) {
    NOTIMPLEMENTED();
  }
}

void BraveProfileSyncService::OnDeletedSyncUser() {
  NOTIMPLEMENTED();
}

void BraveProfileSyncService::OnDeleteSyncSiteSettings()  {
  NOTIMPLEMENTED();
}

void BraveProfileSyncService::OnSaveBookmarksBaseOrder(
    const std::string& order) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(!order.empty());
  brave_sync_prefs_->SetBookmarksBaseOrder(order);
  OnSyncReady();
}

void BraveProfileSyncService::OnSyncWordsPrepared(const std::string& words) {
  NotifyHaveSyncWords(words);
}

int BraveProfileSyncService::GetDisableReasons() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // legacy sync only support bookmark sync so we have to wait for migration
  // complete before enable sync engine
  if (IsBraveSyncEnabled() &&
      brave_sync_prefs_->GetMigratedBookmarksVersion() >= 2)
    return syncer::SyncService::DISABLE_REASON_NONE;
  // kSyncManaged is disabled by us
  return ProfileSyncService::GetDisableReasons();
}

CoreAccountInfo BraveProfileSyncService::GetAuthenticatedAccountInfo() const {
  return GetDummyAccountInfo();
}

bool BraveProfileSyncService::IsAuthenticatedAccountPrimary() const {
  return true;
}

void BraveProfileSyncService::Shutdown() {
  SignalWaitableEvent();
  browser_sync::ProfileSyncService::Shutdown();
}

void BraveProfileSyncService::NotifySyncSetupError(const std::string& error) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  for (auto& observer : BraveSyncService::observers_)
    observer.OnSyncSetupError(this, error);
}

void BraveProfileSyncService::NotifySyncStateChanged() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  for (auto& observer : BraveSyncService::observers_)
    observer.OnSyncStateChanged(this);
}

void BraveProfileSyncService::NotifyHaveSyncWords(
    const std::string& sync_words) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  for (auto& observer : BraveSyncService::observers_)
    observer.OnHaveSyncWords(this, sync_words);
}

void BraveProfileSyncService::ResetSyncInternal() {
  brave_sync_prefs_->SetPrevSeed(brave_sync_prefs_->GetSeed());

  brave_sync_prefs_->Clear();

  brave_sync_configured_ = false;
  brave_sync_initialized_ = false;

  brave_sync_prefs_->SetSyncEnabled(false);
}

void BraveProfileSyncService::SetPermanentNodesOrder(
    const std::string& base_order) {
  DCHECK(model_);
  DCHECK(!base_order.empty());
  std::string order;
  model_->bookmark_bar_node()->GetMetaInfo("order", &order);
  if (order.empty()) {
    bookmarks::BookmarkNode* mutable_bookmark_bar_node =
      const_cast<bookmarks::BookmarkNode*>(model_->bookmark_bar_node());
    model_->SetNodeMetaInfo(mutable_bookmark_bar_node, "order",
                            base_order + "1");
  }
  order.clear();
  model_->other_node()->GetMetaInfo("order", &order);
  if (order.empty()) {
    bookmarks::BookmarkNode* mutable_other_node =
      const_cast<bookmarks::BookmarkNode*>(model_->other_node());
    model_->SetNodeMetaInfo(mutable_other_node, "order", base_order + "2");
  }
  brave_sync_prefs_->SetMigratedBookmarksVersion(2);
}


void BraveProfileSyncService::FetchSyncRecords(const bool bookmarks,
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
    category_names.push_back(kBookmarks);    // "BOOKMARKS";
  }
  if (preferences) {
    category_names.push_back(kPreferences);  // "PREFERENCES";
  }

  brave_sync_prefs_->SetLastFetchTime(base::Time::Now());

  base::Time start_at_time = brave_sync_prefs_->GetLatestRecordTime();
  GetBraveSyncClient()->SendFetchSyncRecords(
    category_names,
    start_at_time,
    max_records);
}

void BraveProfileSyncService::SendCreateDevice() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  std::string device_name = brave_sync_prefs_->GetThisDeviceName();
  std::string object_id = tools::GenerateObjectId();
  std::string device_id = brave_sync_prefs_->GetThisDeviceId();
  CHECK(!device_id.empty());

  SendDeviceSyncRecord(
      SyncRecord::Action::A_CREATE,
      device_name,
      device_id,
      object_id);
}

void BraveProfileSyncService::SendDeviceSyncRecord(
    const int action,
    const std::string& device_name,
    const std::string& device_id,
    const std::string& object_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  RecordsListPtr records = CreateDeviceCreationRecordExtension(
      device_name,
      object_id,
      static_cast<SyncRecord::Action>(action),
      device_id);
  GetBraveSyncClient()->SendSyncRecords(
      SyncRecordType_PREFERENCES, *records);
}

void BraveProfileSyncService::OnResolvedPreferences(
    const RecordsList& records) {
  const std::string this_device_id = brave_sync_prefs_->GetThisDeviceId();
  bool this_device_deleted = false;
  bool contains_only_one_device = false;

  auto sync_devices = brave_sync_prefs_->GetSyncDevices();
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
          record->action == SyncRecord::Action::A_DELETE &&
          actually_merged);
      contains_only_one_device = sync_devices->size() < 2 &&
        record->action == SyncRecord::Action::A_DELETE &&
          actually_merged;
    }
  }  // for each device

  brave_sync_prefs_->SetSyncDevices(*sync_devices);

  if (this_device_deleted) {
    ResetSyncInternal();
  } else if (contains_only_one_device) {
    // We see amount of devices had been decreased to 1 and it is not this
    // device had been deleted. So call OnResetSync which will send DELETE
    // record for this device
    OnResetSync();
  }
}

void BraveProfileSyncService::OnBraveSyncPrefsChanged(const std::string& pref) {
  if (pref == prefs::kSyncEnabled) {
    GetBraveSyncClient()->OnSyncEnabledChanged();
    if (!brave_sync_prefs_->GetSyncEnabled()) {
      brave_sync_initialized_ = false;
      ProfileSyncService::GetUserSettings()->SetSyncRequested(false);
    }
  }
  NotifySyncStateChanged();
}

BraveSyncClient* BraveProfileSyncService::GetBraveSyncClient() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return ProfileSyncService::GetSyncClient()->GetBraveSyncClient();
}

bool BraveProfileSyncService::IsBraveSyncEnabled() const{
  return brave_sync_prefs_->GetSyncEnabled();
}

bool BraveProfileSyncService::IsBraveSyncInitialized() const {
  return brave_sync_initialized_;
}

bool BraveProfileSyncService::IsBraveSyncConfigured() const {
  return brave_sync_configured_;
}

void BraveProfileSyncService::OnPollSyncCycle(GetRecordsCallback cb,
                                         base::WaitableEvent* wevent) {
  if (IsTimeEmpty(brave_sync_prefs_->GetLastFetchTime()))
    SendCreateDevice();
  GetBraveSyncClient()->SendFetchSyncDevices();

  if (!brave_sync_initialized_) {
    wevent->Signal();
    return;
  }

  get_record_cb_ = cb;
  wevent_ = wevent;

  const bool bookmarks = brave_sync_prefs_->GetSyncBookmarksEnabled();
  const bool history = brave_sync_prefs_->GetSyncHistoryEnabled();
  const bool preferences = brave_sync_prefs_->GetSyncSiteSettingsEnabled();
  FetchSyncRecords(bookmarks, history, preferences, 1000);
}

void BraveProfileSyncService::SignalWaitableEvent() {
  if (wevent_) {
    wevent_->Signal();
    wevent_ = nullptr;
  }
}

}   // namespace brave_sync
