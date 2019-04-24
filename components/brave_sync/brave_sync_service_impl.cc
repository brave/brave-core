/* Copyright 2016 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/brave_sync_service_impl.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/task/post_task.h"
#include "brave/browser/ui/webui/sync/sync_ui.h"
#include "brave/components/brave_sync/bookmark_order_util.h"
#include "brave/components/brave_sync/brave_sync_prefs.h"
#include "brave/components/brave_sync/brave_sync_service_observer.h"
#include "brave/components/brave_sync/client/bookmark_change_processor.h"
#include "brave/components/brave_sync/client/brave_sync_client_impl.h"
#include "brave/components/brave_sync/sync_devices.h"
#include "brave/components/brave_sync/jslib_const.h"
#include "brave/components/brave_sync/jslib_messages.h"
#include "brave/components/brave_sync/settings.h"
#include "brave/components/brave_sync/tools.h"
#include "brave/components/brave_sync/values_conv.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "net/base/network_interfaces.h"

namespace brave_sync {

namespace {

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
  record->objectData = jslib_const::SyncObjectData_DEVICE;  // "device"

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
  record->objectData = jslib_const::SyncObjectData_DEVICE;  // "device"

  std::unique_ptr<jslib::Device> device_record =
      std::make_unique<jslib::Device>();
  device_record->name = device->name_;
  record->SetDevice(std::move(device_record));

  return record;
}

}  // namespace

BraveSyncServiceImpl::BraveSyncServiceImpl(Profile* profile) :
    sync_client_(BraveSyncClient::Create(this, profile)),
    sync_initialized_(false),
    sync_words_(std::string()),
    profile_(profile),
    sync_prefs_(new brave_sync::prefs::Prefs(profile->GetPrefs())),
    bookmark_change_processor_(BookmarkChangeProcessor::Create(
        profile,
        sync_client_.get(),
        sync_prefs_.get())),
    timer_(std::make_unique<base::RepeatingTimer>()),
    unsynced_send_interval_(base::TimeDelta::FromMinutes(10)) {
  // Moniter syncs prefs required in GetSettingsAndDevices
  profile_pref_change_registrar_.Init(profile->GetPrefs());
  profile_pref_change_registrar_.Add(
      prefs::kSyncEnabled,
      base::Bind(&BraveSyncServiceImpl::OnSyncPrefsChanged,
                 base::Unretained(this)));
  profile_pref_change_registrar_.Add(
      prefs::kSyncDeviceName,
      base::Bind(&BraveSyncServiceImpl::OnSyncPrefsChanged,
                 base::Unretained(this)));
  profile_pref_change_registrar_.Add(
      prefs::kSyncDeviceList,
      base::Bind(&BraveSyncServiceImpl::OnSyncPrefsChanged,
                 base::Unretained(this)));
  profile_pref_change_registrar_.Add(
      prefs::kSyncBookmarksEnabled,
      base::Bind(&BraveSyncServiceImpl::OnSyncPrefsChanged,
                 base::Unretained(this)));
  profile_pref_change_registrar_.Add(
      prefs::kSyncSiteSettingsEnabled,
      base::Bind(&BraveSyncServiceImpl::OnSyncPrefsChanged,
                 base::Unretained(this)));
  profile_pref_change_registrar_.Add(
      prefs::kSyncHistoryEnabled,
      base::Bind(&BraveSyncServiceImpl::OnSyncPrefsChanged,
                 base::Unretained(this)));

  if (!sync_prefs_->GetSeed().empty() &&
      !sync_prefs_->GetThisDeviceName().empty()) {
    sync_configured_ = true;
  }
}

BraveSyncServiceImpl::~BraveSyncServiceImpl() {
}

BraveSyncClient* BraveSyncServiceImpl::GetSyncClient() {
  return sync_client_.get();
}

bool BraveSyncServiceImpl::IsSyncConfigured() {
  return sync_configured_;
}

bool BraveSyncServiceImpl::IsSyncInitialized() {
  return sync_initialized_;
}

void BraveSyncServiceImpl::Shutdown() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  bookmark_change_processor_->Stop();

  StopLoop();
}

void BraveSyncServiceImpl::OnSetupSyncHaveCode(const std::string& sync_words,
    const std::string& device_name) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (sync_words.empty()) {
    OnSyncSetupError("ERR_SYNC_WRONG_WORDS");
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

  SetDeviceName(device_name);
  initializing_ = true;

  sync_prefs_->SetSyncEnabled(true);
  sync_words_ = sync_words;
}

void BraveSyncServiceImpl::OnSetupSyncNewToSync(
    const std::string& device_name) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (initializing_) {
    NotifyLogMessage("currently initializing");
    return;
  }

  if (IsSyncConfigured()) {
    NotifyLogMessage("already configured");
    return;
  }

  sync_words_.clear();  // If the previous attempt was connect to sync chain
                        // and failed to receive save-init-data
  SetDeviceName(device_name);
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
        jslib::SyncRecord::Action::A_DELETE, device_name, device_id, object_id);
  }
}

void BraveSyncServiceImpl::OnResetSync() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  auto sync_devices = sync_prefs_->GetSyncDevices();

  if (sync_devices->size() == 0) {
    // Fail safe option
    VLOG(2) << "[Sync] " << __func__ << " unexpected zero device size";
    ResetSyncInternal();
  } else {
    // We have to send delete record and wait for library deleted response then
    // we can reset it by ResetInternal()
    const std::string device_id = sync_prefs_->GetThisDeviceId();
    OnDeleteDevice(device_id);
  }
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
}

void BraveSyncServiceImpl::OnSetSyncBookmarks(const bool sync_bookmarks) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  sync_prefs_->SetSyncBookmarksEnabled(sync_bookmarks);
}

void BraveSyncServiceImpl::OnSetSyncBrowsingHistory(
    const bool sync_browsing_history) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  sync_prefs_->SetSyncHistoryEnabled(sync_browsing_history);
}

void BraveSyncServiceImpl::OnSetSyncSavedSiteSettings(
    const bool sync_saved_site_settings) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  sync_prefs_->SetSyncSiteSettingsEnabled(sync_saved_site_settings);
}

// SyncLibToBrowserHandler overrides
void BraveSyncServiceImpl::BackgroundSyncStarted(bool startup) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (startup)
    bookmark_change_processor_->Start();

  StartLoop();
}

void BraveSyncServiceImpl::BackgroundSyncStopped(bool shutdown) {
  if (shutdown)
    Shutdown();
  else
    StopLoop();
}

void BraveSyncServiceImpl::OnSyncDebug(const std::string& message) {
  NotifyLogMessage(message);
}

void BraveSyncServiceImpl::OnSyncSetupError(const std::string& error) {
  if (initializing_) {
    sync_prefs_->Clear();
    initializing_ = false;
  }
  NotifySyncSetupError(error);
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
  config.server_url = "https://sync.brave.com";
  config.debug = true;
  sync_client_->SendGotInitData(seed, device_id, config, sync_words_);
}

void BraveSyncServiceImpl::OnSaveInitData(const Uint8Array& seed,
                                          const Uint8Array& device_id) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  DCHECK(!sync_initialized_);
  // If we are here and initializing_ is false, we have came
  // not from OnSetupSyncNewToSync or OnSetupSyncHaveCode.
  // One case is we put wrong code words and then restarted before cleared
  // kSyncEnabled pref. This should not happen.
  DCHECK(initializing_);

  std::string seed_str = StrFromUint8Array(seed);
  std::string device_id_str = StrFromUint8Array(device_id);

  std::string prev_seed_str = sync_prefs_->GetPrevSeed();

  sync_words_.clear();
  DCHECK(!seed_str.empty());

  if (prev_seed_str == seed_str) {  // reconnecting to previous sync chain
    sync_prefs_->SetPrevSeed(std::string());
  } else if (!prev_seed_str.empty()) {  // connect/create to new sync chain
    bookmark_change_processor_->Reset(true);
    sync_prefs_->SetPrevSeed(std::string());
  } else {
    // This is not required, because when there is no previous seed, bookmarks
    // should not have a metadata. However, this is done by intention, to be
    // a remedy for cases when sync had been reset and prev_seed_str had been
    // cleared when it shouldn't (brave-browser#3188).
    bookmark_change_processor_->Reset(true);
  }

  sync_prefs_->SetSeed(seed_str);
  sync_prefs_->SetThisDeviceId(device_id_str);

  sync_configured_ = true;

  sync_prefs_->SetSyncBookmarksEnabled(true);
  // TODO(bridiver) - re-enable these when we add history, site settings
  sync_prefs_->SetSyncSiteSettingsEnabled(false);
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

  // fetch the records
  RequestSyncData();
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
    bookmark_change_processor_->GetAllSyncData(
        *records.get(), records_and_existing_objects.get());
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
    bookmark_change_processor_->ApplyChangesFromSyncModel(*records.get());
    bookmark_change_processor_->SendUnsynced(unsynced_send_interval_);
  } else if (category_name == brave_sync::jslib_const::kHistorySites) {
    NOTIMPLEMENTED();
  }
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
  bool contains_only_one_device = false;

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
          record->action == jslib::SyncRecord::Action::A_DELETE &&
          actually_merged);
      contains_only_one_device = sync_devices->size() < 2 &&
        record->action == jslib::SyncRecord::Action::A_DELETE &&
          actually_merged;
    }
  }  // for each device

  sync_prefs_->SetSyncDevices(*sync_devices);

  if (this_device_deleted) {
    ResetSyncInternal();
  } else if (contains_only_one_device) {
    // We see amount of devices had been decreased to 1 and it is not this
    // device had been deleted. So call OnResetSync which will send DELETE
    // record for this device
    OnResetSync();
  }
}

void BraveSyncServiceImpl::OnSyncPrefsChanged(const std::string& pref) {
  if (pref == prefs::kSyncEnabled) {
    sync_client_->OnSyncEnabledChanged();
    if (!sync_prefs_->GetSyncEnabled())
      sync_initialized_ = false;
  }
  NotifySyncStateChanged();
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

void BraveSyncServiceImpl::OnSaveBookmarkOrder(const std::string& object_id,
                                               const std::string& order)  {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(!order.empty());
  bookmark_change_processor_->ApplyOrder(object_id, order);
}

void BraveSyncServiceImpl::OnSyncWordsPrepared(const std::string& words) {
  NotifyHaveSyncWords(words);
}

// Here we query sync lib for the records after initialization (or again later)
void BraveSyncServiceImpl::RequestSyncData() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  const bool bookmarks = sync_prefs_->GetSyncBookmarksEnabled();
  const bool history = sync_prefs_->GetSyncHistoryEnabled();
  const bool preferences = sync_prefs_->GetSyncSiteSettingsEnabled();

  if (!bookmarks && !history && !preferences)
    return;

  base::Time last_fetch_time = sync_prefs_->GetLastFetchTime();

  if (tools::IsTimeEmpty(last_fetch_time)) {
    SendCreateDevice();
  }

  sync_client_->SendFetchSyncDevices();

  if (sync_prefs_->GetSyncDevices()->size() <= 1) {
    // No sense to fetch or sync bookmarks when there no at least two devices
    // in chain
    // Set last fetch time here because we had fetched devices at least
    sync_prefs_->SetLastFetchTime(base::Time::Now());
    return;
  }

  if (tools::IsTimeEmpty(last_fetch_time)) {
    bookmark_change_processor_->InitialSync();
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
  using brave_sync::jslib_const::kHistorySites;
  using brave_sync::jslib_const::kBookmarks;
  using brave_sync::jslib_const::kPreferences;
  if (history) {
    category_names.push_back(kHistorySites);  // "HISTORY_SITES";
  }
  if (bookmarks) {
    category_names.push_back(kBookmarks);    // "BOOKMARKS";
  }
  if (preferences) {
    category_names.push_back(kPreferences);  // "PREFERENCES";
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
      jslib::SyncRecord::Action::A_CREATE,
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

static const int64_t kCheckUpdatesIntervalSec = 60;

void BraveSyncServiceImpl::StartLoop() {
  timer_->Start(FROM_HERE,
                  base::TimeDelta::FromSeconds(kCheckUpdatesIntervalSec),
                  this,
                  &BraveSyncServiceImpl::LoopProc);
}

void BraveSyncServiceImpl::StopLoop() {
  timer_->Stop();
}

void BraveSyncServiceImpl::LoopProc() {
  base::CreateSingleThreadTaskRunnerWithTraits(
    {content::BrowserThread::UI})->PostTask(
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
  DLOG(INFO) << message;
}

void BraveSyncServiceImpl::NotifySyncSetupError(const std::string& error) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  for (auto& observer : observers_)
    observer.OnSyncSetupError(this, error);
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

void BraveSyncServiceImpl::ResetSyncInternal() {
  bookmark_change_processor_->Reset(false);

  sync_prefs_->SetPrevSeed(sync_prefs_->GetSeed());

  sync_prefs_->Clear();

  sync_configured_ = false;
  sync_initialized_ = false;

  sync_prefs_->SetSyncEnabled(false);
}

void BraveSyncServiceImpl::SetDeviceName(const std::string& name) {
  if (name.empty()) {
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
    sync_prefs_->SetThisDeviceName(hostname);
  } else {
    sync_prefs_->SetThisDeviceName(name);
  }
}

}  // namespace brave_sync
