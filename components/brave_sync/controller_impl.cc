/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/controller_impl.h"

#include <sstream>

#include "base/debug/stack_trace.h"
#include "base/task_runner.h"
#include "base/task/post_task.h"

#include "brave/browser/ui/brave_pages.h"
#include "brave/browser/ui/webui/sync/sync_ui.h"
#include "brave/components/brave_sync/bookmarks.h"
#include "brave/components/brave_sync/client/client_ext_impl.h"
#include "brave/components/brave_sync/client/client_factory.h"
#include "brave/components/brave_sync/controller_observer.h"
#include "brave/components/brave_sync/debug.h"
#include "brave/components/brave_sync/devices.h"
#include "brave/components/brave_sync/history.h"
#include "brave/components/brave_sync/jslib_const.h"
#include "brave/components/brave_sync/jslib_messages.h"
#include "brave/components/brave_sync/object_map.h"
#include "brave/components/brave_sync/profile_prefs.h"
#include "brave/components/brave_sync/settings.h"
#include "brave/components/brave_sync/tools.h"
#include "brave/components/brave_sync/values_conv.h"
#include "brave/components/brave_sync/value_debug.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser.h"
#include "content/public/browser/browser_thread.h"

namespace brave_sync {

ControllerImpl::TempStorage::TempStorage() {
}

ControllerImpl::TempStorage::~TempStorage() {
}

ControllerImpl::ControllerImpl(Profile *profile) :
  sync_client_(nullptr),
  sync_initialized_(false),
  profile_(nullptr),
  timer_(std::make_unique<base::RepeatingTimer>()) {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::ControllerImpl CTOR";
  LOG(ERROR) << base::debug::StackTrace().ToString();
  LOG(ERROR) << "TAGAB ---------------------";

  DETACH_FROM_SEQUENCE(sequence_checker_);

  SetProfile(profile);
}

ControllerImpl::~ControllerImpl() {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::~ControllerImpl DTOR";
}

bool ControllerImpl::IsSyncConfigured() {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::IsSyncConfigured will return sync_configured_="<<sync_configured_;
  return sync_configured_;
}

bool ControllerImpl::IsSyncInitialized() {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::IsSyncConfigured will return sync_initialized_="<<sync_initialized_;
  return sync_initialized_;
}

// Usually initialized at BraveSyncExtensionLoadedFunction::Run
void ControllerImpl::SetProfile(Profile *profile) {
  LOG(ERROR) << "TAGAB  ControllerImpl::SetProfile profile="<<profile;
  // LOG(ERROR) << base::debug::StackTrace().ToString();
  // LOG(ERROR) << "TAGAB ---------------------";

  DCHECK(profile);
  DCHECK(!profile_);

  sync_prefs_.reset(new brave_sync::prefs::Prefs(profile));

  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::ControllerImpl sync_prefs_->GetSeed()=<" << sync_prefs_->GetSeed() <<">";
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::ControllerImpl sync_prefs_->GetThisDeviceName()=<" << sync_prefs_->GetThisDeviceName() <<">";

  task_runner_ = base::CreateSequencedTaskRunnerWithTraits(
      {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
       base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN} );

  sync_obj_map_ = std::make_unique<storage::ObjectMap>(profile->GetPath());

  bookmarks_ = std::make_unique<brave_sync::Bookmarks>(this);
  bookmarks_->SetProfile(profile);
  if (!sync_prefs_->GetThisDeviceId().empty()) {
    bookmarks_->SetThisDeviceId(sync_prefs_->GetThisDeviceId());
  }
  bookmarks_->SetObjectMap(sync_obj_map_.get());

  history_ = std::make_unique<brave_sync::History>(/*this, */profile, this);
  if (!sync_prefs_->GetThisDeviceId().empty()) {
    history_->SetThisDeviceId(sync_prefs_->GetThisDeviceId());
  }
  history_->SetObjectMap(sync_obj_map_.get());


  if (!sync_prefs_->GetSeed().empty() && !sync_prefs_->GetThisDeviceName().empty()) {
    LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::ControllerImpl sync is configured";
    sync_configured_ = true;
  } else {
    LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::ControllerImpl sync is NOT configured";
    LOG(ERROR) << "TAGAB sync_prefs_->GetSeed()=<" << sync_prefs_->GetSeed() << ">";
    LOG(ERROR) << "TAGAB sync_prefs_->GetThisDeviceName()=<" << sync_prefs_->GetThisDeviceName() << ">";
  }

  profile_ = profile;

  if (!sync_client_) {
    sync_client_ = BraveSyncClientFactory::GetForBrowserContext(profile);
    sync_client_->SetSyncToBrowserHandler(this);
  }

  LOG(ERROR) << "TAGAB  ControllerImpl::SetProfile sync_client_="<<sync_client_;

  LOG(ERROR) << "TAGAB  ControllerImpl::SetProfile sync_client_ null, post in UI ControllerImpl::InitJsLib";
  content::BrowserThread::GetTaskRunnerForThread(content::BrowserThread::UI)->PostTask(
    FROM_HERE, base::Bind(&ControllerImpl::InitJsLib,
         base::Unretained(this), false));

  StartLoop();
}

void ControllerImpl::Shutdown() {
  LOG(ERROR) << "TAGAB ControllerImpl::Shutdown";

  StopLoop();

  bookmarks_.reset();
  history_.reset();

  task_runner_->PostTask(
    FROM_HERE,
    base::Bind(&ControllerImpl::ShutdownFileWork, base::Unretained(this))
  );
}

void ControllerImpl::ShutdownFileWork() {
  sync_obj_map_.reset();
}

void ControllerImpl::OnSetupSyncHaveCode(const std::string &sync_words,
  const std::string &device_name) {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnSetupSyncHaveCode";
  LOG(ERROR) << "TAGAB sync_words=" << sync_words;
  LOG(ERROR) << "TAGAB device_name=" << device_name;
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (sync_words.empty() || device_name.empty()) {
    OnSyncSetupError("missing sync words or device name");
    return;
  }

  if (temp_storage_.currently_initializing_guard_) {
    TriggerOnLogMessage("currently initializing");
    return;
  }

  if (IsSyncConfigured()) {
    TriggerOnLogMessage("already configured");
    return;
  }

  temp_storage_.device_name_ = device_name; // Fill here, but save in OnSaveInitData
  temp_storage_.currently_initializing_guard_ = true;

  DCHECK(sync_client_);
  sync_client_->NeedBytesFromSyncWords(sync_words);
}

void ControllerImpl::OnSetupSyncNewToSync(const std::string &device_name) {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnSetupSyncNewToSync";
  LOG(ERROR) << "TAGAB device_name="<<device_name;
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (device_name.empty()) {
    OnSyncSetupError("missing device name");
    return;
  }

  if (temp_storage_.currently_initializing_guard_) {
    TriggerOnLogMessage("currently initializing");
    return;
  }

  if (IsSyncConfigured()) {
    TriggerOnLogMessage("already configured");
    return;
  }

  temp_storage_.device_name_ = device_name; // Fill here, but save in OnSaveInitData
  temp_storage_.currently_initializing_guard_ = true;

  InitJsLib(true); // Init will cause load of the Script
  // Then we will got GOT_INIT_DATA and SAVE_INIT_DATA, where we will save the seed and device id
  // Then when we will receive sync_ready, we should display web page with sync settings
}

void ControllerImpl::OnDeleteDevice(const std::string &device_id) {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnDeleteDevice";
  LOG(ERROR) << "TAGAB device_id="<<device_id;
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  CHECK(sync_client_ != nullptr);
  CHECK(sync_initialized_);

  task_runner_->PostTask(
    FROM_HERE,
    base::Bind(&ControllerImpl::OnDeleteDeviceFileWork, base::Unretained(this), device_id)
  );
}

void ControllerImpl::OnDeleteDeviceFileWork(const std::string &device_id) {
  LOG(ERROR) << "TAGAB  ControllerImpl::OnDeleteDeviceFileWork";
  std::string json = sync_obj_map_->GetSpecialJSONByLocalId(jslib_const::DEVICES_NAMES);
  SyncDevices syncDevices;
  syncDevices.FromJson(json);
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnDeleteDeviceFileWork json="<<json;

  const SyncDevice *device = syncDevices.GetByDeviceId(device_id);
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnDeleteDeviceFileWork device="<<device;
  //DCHECK(device); // once I saw it nullptr
  if (device) {
    const std::string device_name = device->name_;
    const std::string object_id = device->object_id_;
    LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnDeleteDeviceFileWork device_name="<<device_name;
    LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnDeleteDeviceFileWork object_id="<<object_id;

    SendDeviceSyncRecord(jslib::SyncRecord::Action::DELETE,
      device_name,
      device_id,
      object_id);
  }
}

void ControllerImpl::OnResetSync() {
  LOG(ERROR) << "TAGAB  ControllerImpl::OnResetSync";
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  DCHECK(sync_client_ != nullptr);
  //DCHECK(sync_initialized_);

  const std::string device_id = sync_prefs_->GetThisDeviceId();
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnResetSync device_id="<<device_id;

  task_runner_->PostTask(
    FROM_HERE,
    base::Bind(&ControllerImpl::OnResetSyncFileWork, base::Unretained(this), device_id)
  );
}

void ControllerImpl::OnResetSyncFileWork(const std::string &device_id) {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnResetSyncFileWork";
  OnDeleteDeviceFileWork(device_id);
  sync_obj_map_->DestroyDB();

  content::BrowserThread::PostTask(content::BrowserThread::UI, FROM_HERE,
    base::Bind(&ControllerImpl::OnResetSyncPostFileUiWork, base::Unretained(this))
  );
}

void ControllerImpl::OnResetSyncPostFileUiWork() {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnResetSyncPostFileUiWork";
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  sync_prefs_->Clear();

  sync_configured_ = false;

  TriggerOnSyncStateChanged();
}

void ControllerImpl::GetSettingsAndDevices(const GetSettingsAndDevicesCallback &callback) {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::GetSettingsAndDevices " << GetThreadInfoString();
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  // pref service should be queried on UI thread in anyway
  std::unique_ptr<brave_sync::Settings> settings = sync_prefs_->GetBraveSyncSettings();

  // Jump to task runner thread to perform FILE operation and then back to UI
  task_runner_->PostTask(
    FROM_HERE,
    base::Bind(&ControllerImpl::GetSettingsAndDevicesImpl, base::Unretained(this), base::Passed(std::move(settings)), callback)
  );
}

void ControllerImpl::GetSettingsAndDevicesImpl(std::unique_ptr<brave_sync::Settings> settings, const GetSettingsAndDevicesCallback &callback) {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::GetSettingsAndDevicesImpl " << GetThreadInfoString();

  std::unique_ptr<brave_sync::SyncDevices> devices = std::make_unique<brave_sync::SyncDevices>();
  std::string json = sync_obj_map_->GetSpecialJSONByLocalId(jslib_const::DEVICES_NAMES);
  devices->FromJson(json);
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::GetSettingsAndDevicesImpl json="<<json;

  // Jump back to UI with an answer
  content::BrowserThread::PostTask(content::BrowserThread::UI, FROM_HERE,
    base::Bind(callback, base::Passed(std::move(settings)), base::Passed(std::move(devices)))
  );
}

void ControllerImpl::GetSyncWords() {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::GetSyncWords";

  // Ask sync client
  DCHECK(sync_client_);
  std::string seed = sync_prefs_->GetSeed();
  sync_client_->NeedSyncWords(seed);
}

std::string ControllerImpl::GetSeed() {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::GetSeed";
  std::string seed = sync_prefs_->GetSeed();
  return seed;
}

void ControllerImpl::OnSetSyncThisDevice(const bool &sync_this_device) {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnSetSyncThisDevice " << sync_this_device;
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  sync_prefs_->SetSyncThisDevice(sync_this_device);
}

void ControllerImpl::OnSetSyncBookmarks(const bool &sync_bookmarks) {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnSetSyncBookmarks " << sync_bookmarks;
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  sync_prefs_->SetSyncBookmarksEnabled(sync_bookmarks);
}

void ControllerImpl::OnSetSyncBrowsingHistory(const bool &sync_browsing_history) {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnSetSyncBrowsingHistory " << sync_browsing_history;
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  sync_prefs_->SetSyncHistoryEnabled(sync_browsing_history);
}

void ControllerImpl::OnSetSyncSavedSiteSettings(const bool &sync_saved_site_settings) {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnSyncSavedSiteSettings " << sync_saved_site_settings;
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  sync_prefs_->SetSyncSiteSettingsEnabled(sync_saved_site_settings);
}

void ControllerImpl::InitJsLib(const bool &setup_new_sync) {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::InitJsLib " << GetThreadInfoString();


  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::InitJsLib sync_client_=" << sync_client_;

  DCHECK(sync_client_);
  if ( !seen_get_init_data_ && ( (!sync_prefs_->GetSeed().empty() && !sync_prefs_->GetThisDeviceName().empty()) ||
      setup_new_sync) ) {
    LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::InitJsLib sync is active or setup_new_sync";
    sync_client_->LoadClient();
  }
}

void ControllerImpl::OnMessageFromSyncReceived() {
  ;
}

// SyncLibToBrowserHandler overrides
void ControllerImpl::OnSyncDebug(const std::string &message) {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnSyncDebug: message=<" << message << ">";
  TriggerOnLogMessage(message);
}

void ControllerImpl::OnSyncSetupError(const std::string &error) {
  temp_storage_.currently_initializing_guard_ = false;
  OnSyncDebug(error);
}

void ControllerImpl::OnGetInitData(const std::string &sync_version) {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnGetInitData sync_version="<<sync_version<<"----------";
  LOG(ERROR) << base::debug::StackTrace().ToString();

  seen_get_init_data_ = true;

  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnGetInitData temp_storage_.seed_str_=<" << temp_storage_.seed_str_ << ">";
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnGetInitData sync_prefs_->GetSeed()=<" << sync_prefs_->GetSeed() << ">";

  Uint8Array seed;
  if (!temp_storage_.seed_str_.empty()) {
    seed = Uint8ArrayFromString(temp_storage_.seed_str_);
    LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnGetInitData take seed from temp store";
  } else if (!sync_prefs_->GetSeed().empty()) {
    seed = Uint8ArrayFromString(sync_prefs_->GetSeed());
    LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnGetInitData take seed from prefs store";
  } else {
    // We are starting a new chain, so we don't know neither seed nor device id
    LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnGetInitData starting new chain, use no seeds";
  }

  Uint8Array device_id;
  if (!sync_prefs_->GetThisDeviceId().empty()) {
    device_id = Uint8ArrayFromString(sync_prefs_->GetThisDeviceId());
    LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnGetInitData use device id from prefs StrFromUint8Array(device_id)="<<StrFromUint8Array(device_id);
  } else {
    LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnGetInitData use empty device id";
  }

  DCHECK(!sync_version.empty());
  sync_version_ = sync_version;
  sync_obj_map_->SetApiVersion("0");

  brave_sync::client_data::Config config;
  config.api_version = "0";
  config.server_url = "https://sync-staging.brave.com";
  config.debug = true;
  sync_client_->SendGotInitData(seed, device_id, config);

  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnGetInitData called sync_client_->SendGotInitData---";
}

void ControllerImpl::OnSaveInitData(const Uint8Array &seed, const Uint8Array &device_id) {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnSaveInitData:";
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  DCHECK(false == sync_initialized_);
  DCHECK(temp_storage_.currently_initializing_guard_);

  std::string seed_str = StrFromUint8Array(seed);
  std::string device_id_str = StrFromUint8Array(device_id);

  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnSaveInitData: seed=<"<<seed_str<<">";
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnSaveInitData: device_id="<<device_id_str<<">";

  if (temp_storage_.seed_str_.empty() && !seed_str.empty()) {
    temp_storage_.seed_str_ = seed_str;
  }

  // Check existing values
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnSaveInitData: GetThisDeviceId()="<<sync_prefs_->GetThisDeviceId();
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnSaveInitData: GetSeed()="<<sync_prefs_->GetSeed();
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnSaveInitData: GetThisDeviceName()="<<sync_prefs_->GetThisDeviceName();

  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnSaveInitData: temp_storage_.seed_str_="<<temp_storage_.seed_str_;

  if (temp_storage_.device_name_.empty()) {
    temp_storage_.device_name_ = sync_prefs_->GetThisDeviceName();
  }

  //Save
  sync_prefs_->SetThisDeviceId(device_id_str);
  bookmarks_->SetThisDeviceId(device_id_str);
  // If we have already initialized sync earlier we don't receive seed again
  // and do not save it
  if (!temp_storage_.seed_str_.empty()) {
    sync_prefs_->SetSeed(temp_storage_.seed_str_);
  }
  DCHECK(!temp_storage_.device_name_.empty());
  sync_prefs_->SetThisDeviceName(temp_storage_.device_name_);
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnSaveInitData: saved device_id="<<device_id_str;
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnSaveInitData: saved seed="<<temp_storage_.seed_str_;
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnSaveInitData: saved temp_storage_.device_name_="<<temp_storage_.device_name_;

  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnSaveInitData: sync_configured_ = true";
  sync_configured_ = true;

  sync_prefs_->SetSyncThisDevice(true);

  sync_prefs_->SetSyncBookmarksEnabled(true);
  sync_prefs_->SetSyncSiteSettingsEnabled(true);
  sync_prefs_->SetSyncHistoryEnabled(true);

  temp_storage_.currently_initializing_guard_ = false;
}

void ControllerImpl::OnSyncReady() {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnSyncReady:";
  const std::string bookmarks_base_order = sync_prefs_->GetBookmarksBaseOrder();
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnSyncReady: bookmarks_base_order="<<bookmarks_base_order;
  if (bookmarks_base_order.empty()) {
    std::string platform = tools::GetPlatformName();
    LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnSyncReady: platform=" << platform;
    LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnSyncReady: sync_prefs_->GetThisDeviceId()=" << sync_prefs_->GetThisDeviceId();
    sync_client_->SendGetBookmarksBaseOrder(sync_prefs_->GetThisDeviceId(), platform);
    return;
  }

  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnSyncReady: about to call SetBaseOrder " << bookmarks_base_order;
  bookmarks_->SetBaseOrder(bookmarks_base_order);
  DCHECK(false == sync_initialized_);
  sync_initialized_ = true;

  TriggerOnSyncStateChanged();

  // fetch the records
  RequestSyncData();
}

void ControllerImpl::OnGetExistingObjects(const std::string &category_name,
  std::unique_ptr<RecordsList> records,
  const base::Time &last_record_time_stamp,
  const bool &is_truncated) {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnGetExistingObjects:";
  LOG(ERROR) << "TAGAB category_name=" << category_name;
  LOG(ERROR) << "TAGAB records.size()=" << records->size();
  LOG(ERROR) << "TAGAB last_record_time_stamp=" << last_record_time_stamp;
  LOG(ERROR) << "TAGAB is_truncated=" << is_truncated;
  for(const auto & r : *records) {
    if (r->has_bookmark()) {
      LOG(ERROR) << "TAGAB title           =" << r->GetBookmark().site.title;
      LOG(ERROR) << "TAGAB syncTimestamp   =" << r->syncTimestamp;
      LOG(ERROR) << "TAGAB syncTimestampJS =" << static_cast<int64_t>(r->syncTimestamp.ToJsTime());
      LOG(ERROR) << "TAGAB -----";
    }
  }
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (!tools::IsTimeEmpty(last_record_time_stamp)) {
    sync_prefs_->SetLatestRecordTime(last_record_time_stamp);
  }

  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnGetExistingObjects: last_time_fetch_sent_=" << last_time_fetch_sent_;
  DCHECK(!tools::IsTimeEmpty(last_time_fetch_sent_));
  sync_prefs_->SetLastFetchTime(last_time_fetch_sent_);

  // Jump to task runner thread to perform FILE operation and then back to UI
  task_runner_->PostTask(
    FROM_HERE,
    base::Bind(&ControllerImpl::OnGetExistingObjectsFileWork, base::Unretained(this),
    category_name, base::Passed(std::move(records)), last_record_time_stamp, is_truncated )
  );
}

void ControllerImpl::OnGetExistingObjectsFileWork(const std::string &category_name,
  std::unique_ptr<RecordsList> records,
  const base::Time &last_record_time_stamp,
  const bool &is_truncated) {
  // Runs in task_runner_

  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnGetExistingObjectsFileWork:";
  LOG(ERROR) << "TAGAB category_name=" << category_name;
  LOG(ERROR) << "TAGAB records.size()=" << records->size();
  LOG(ERROR) << "TAGAB last_record_time_stamp=" << last_record_time_stamp;
  LOG(ERROR) << "TAGAB is_truncated=" << is_truncated;

  if (category_name == jslib_const::kBookmarks || category_name == jslib_const::kPreferences) {
    SyncRecordAndExistingList records_and_existing_objects = PrepareResolvedResponse(category_name, *records.get());
    SendResolveSyncRecords(category_name, records_and_existing_objects);
  } else if (category_name == jslib_const::kHistorySites) {
    // Queries to history are asynchronous, juggle the threads
    // The same further for obj db
    GetExistingHistoryObjects(*records.get(), last_record_time_stamp, is_truncated);
  } else {
    // Not reached
    NOTREACHED();
  }
}

void ControllerImpl::GetExistingHistoryObjects(
  const RecordsList &records,
  const base::Time &last_record_time_stamp,
  const bool &is_truncated ) {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::GetExistingHistoryObjects:";

  return;

  // Get IDs we need
  // Post HistoryDB query
  // On query response fill resolved records

  std::vector<int64_t> ids_to_get_history(records.size());

  for (const SyncRecordPtr &record : records ) {
    auto local_id = sync_obj_map_->GetLocalIdByObjectId(storage::ObjectMap::Type::History, record->objectId);
    int64_t i_local_id = 0;
    if (base::StringToInt64(local_id, &i_local_id)) {
      ids_to_get_history.push_back(i_local_id);
    } else {
      DCHECK(false) << "Could not convert <" << local_id << "> to int64_t";
    }
  }

  DCHECK(!ids_to_get_history.empty());

  // TODO, AB: use brave_sync::History and history::HistoryService::QueryHistoryByIds
}

SyncRecordAndExistingList ControllerImpl::PrepareResolvedResponse(
  const std::string &category_name,
  const RecordsList &records) {
  SyncRecordAndExistingList resolvedResponse;

  for (const SyncRecordPtr &record : records ) {
    SyncRecordAndExistingPtr resolved_record = std::make_unique<SyncRecordAndExisting>();
    resolved_record->first = jslib::SyncRecord::Clone(*record);

    std::string object_id = record->objectId;

    LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::PrepareResolvedResponse_ object_id=" << object_id;
    LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::PrepareResolvedResponse_ record->action=" << record->action;

    if (category_name == jslib_const::kBookmarks) {
      //"BOOKMARKS"
      LOG(ERROR) << "TAGAB PRR record->GetBookmark().site.title=" << record->GetBookmark().site.title;
      LOG(ERROR) << "TAGAB PRR record->GetBookmark().site.location=" << record->GetBookmark().site.location;
      LOG(ERROR) << "TAGAB PRR record->GetBookmark().order=<" << record->GetBookmark().order << ">";
      resolved_record->second = bookmarks_->GetResolvedBookmarkValue(object_id/*, record->GetBookmark().order*/, record->action);
      LOG(ERROR) << "TAGAB PRR resolved_record->second.get()=" << resolved_record->second.get();
      if (resolved_record->second.get()) {
        LOG(ERROR) << "TAGAB PRR objectData=" << resolved_record->second->objectData;
        DCHECK(!resolved_record->second->objectData.empty());
        LOG(ERROR) << "TAGAB PRR action=" << resolved_record->second->action;
        LOG(ERROR) << "TAGAB PRR has_bookmark=" << resolved_record->second->has_bookmark();
        LOG(ERROR) << "TAGAB PRR title=" << resolved_record->second->GetBookmark().site.title;
        LOG(ERROR) << "TAGAB PRR location=" << resolved_record->second->GetBookmark().site.location;
      } else {
        LOG(ERROR) << "TAGAB PRR second is NULL";
      }

    } else if (category_name == jslib_const::kHistorySites) {
      //"HISTORY_SITES";
      resolved_record->second = history_->GetResolvedHistoryValue(object_id);
    } else if (category_name == jslib_const::kPreferences) {
      //"PREFERENCES"
      LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::PrepareResolvedResponse_: resolving device";
      resolved_record->second = PrepareResolvedDevice(object_id);
      LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::PrepareResolvedResponse_: -----------------";
    }

    resolvedResponse.emplace_back(std::move(resolved_record));
  }

  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::PrepareResolvedResponse_ -----------------------------";
  return resolvedResponse;
}

SyncRecordPtr ControllerImpl::PrepareResolvedDevice(const std::string &object_id) {
  // std::string json = sync_obj_map_->GetSpecialJSONByLocalId(jslib_const::DEVICES_NAMES);
  // SyncDevices devices;
  // devices.FromJson(json);
  //
  // SyncDevice* device = devices.GetByObjectId(object_id);
  // LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::PrepareResolvedResponse device=" << device;
  // if (device) {
  //   LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::PrepareResolvedResponse will ret value";
  //   return device->ToValue();
  // } else {
  //   LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::PrepareResolvedResponse will ret none";
  //   return std::make_unique<base::Value>(base::Value::Type::NONE);
  // }

  return nullptr;
}

void ControllerImpl::SendResolveSyncRecords(const std::string &category_name,
  const SyncRecordAndExistingList& records_and_existing_objects) {
  DCHECK(sync_client_);

  sync_client_->SendResolveSyncRecords(category_name, records_and_existing_objects);
}

void ControllerImpl::OnResolvedSyncRecords(const std::string &category_name,
  std::unique_ptr<RecordsList> records) {
  LOG(ERROR) << "TAGAB OnResolvedSyncRecords records->size()" << records->size();
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  // Jump to thread allowed perform file operations
  task_runner_->PostTask(
    FROM_HERE,
    base::Bind(&ControllerImpl::OnResolvedSyncRecordsFileWork, base::Unretained(this),
    category_name,
    base::Passed(std::move(records)))
  );
}

void ControllerImpl::OnResolvedSyncRecordsFileWork(const std::string &category_name,
  std::unique_ptr<RecordsList> records) {
  if (category_name == brave_sync::jslib_const::kPreferences) {
    OnResolvedPreferences(*records.get());
  } else if (category_name == brave_sync::jslib_const::kBookmarks) {
    OnResolvedBookmarks(*records.get());
  } else if (category_name == brave_sync::jslib_const::kHistorySites) {
    OnResolvedHistorySites(*records.get());
  }
}

void ControllerImpl::OnResolvedPreferences(const RecordsList &records) {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnResolvedPreferences:";

  SyncDevices existing_sync_devices;
  std::string json = sync_obj_map_->GetSpecialJSONByLocalId(jslib_const::DEVICES_NAMES);
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnResolvedPreferences: existing json=<" << json << ">";
  existing_sync_devices.FromJson(json);

  //SyncDevices sync_devices;

  //then merge to existing

  // std::unique_ptr<base::Value> root = std::make_unique<base::Value>(base::Value::Type::DICTIONARY);
  // std::unique_ptr<base::Value> arr_devices = std::make_unique<base::Value>(base::Value::Type::LIST);

  for (const auto &record : records) {
    DCHECK(record->has_device() || record->has_sitesetting());

    if (record->has_device()) {
      LOG(ERROR) << "TAGAB OnResolvedPreferences record->GetDevice().name=" << record->GetDevice().name;
      LOG(ERROR) << "TAGAB OnResolvedPreferences record->deviceId=" << record->deviceId;
      LOG(ERROR) << "TAGAB OnResolvedPreferences record->objectId=" << record->objectId;

      LOG(ERROR) << "TAGAB OnResolvedPreferences record->action=" << record->action;

      existing_sync_devices.Merge(SyncDevice(record->GetDevice().name,
          record->objectId, record->deviceId, record->syncTimestamp.ToJsTime()), record->action);
    }
  } // for each device


  DCHECK(existing_sync_devices.devices_.size() > 0);

  std::string sync_devices_json = existing_sync_devices.ToJson();
  LOG(ERROR) << "TAGAB OnResolvedPreferences sync_devices_json="<<sync_devices_json;

  LOG(ERROR) << "TAGAB OnResolvedPreferences before SaveObjectId";
  sync_obj_map_->SaveSpecialJson(jslib_const::DEVICES_NAMES, sync_devices_json);
  LOG(ERROR) << "TAGAB OnResolvedPreferences SaveObjectId done";

  // Inform devices list chain has been changed
  LOG(ERROR) << "TAGAB OnResolvedPreferences OnSyncStateChanged()";

  content::BrowserThread::GetTaskRunnerForThread(content::BrowserThread::UI)->PostTask(
    FROM_HERE, base::Bind(&ControllerImpl::TriggerOnSyncStateChanged,
         base::Unretained(this)));


  LOG(ERROR) << "TAGAB OnResolvedPreferences OnSyncStateChanged() done";
}

void ControllerImpl::OnResolvedBookmarks(const RecordsList &records) {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnResolvedBookmarks: ";
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnResolvedBookmarks: " << GetThreadInfoString();

  for (const auto &sync_record : records) {
    DCHECK(sync_record->has_bookmark());
    LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnResolvedBookmarks: title=<" << sync_record->GetBookmark().site.title << ">";
    LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnResolvedBookmarks: sync_record->objectId=<" << sync_record->objectId << ">";
    LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnResolvedBookmarks: order=<" << sync_record->GetBookmark().order << ">";
    DCHECK(!sync_record->objectId.empty());
    std::string local_id = sync_obj_map_->GetLocalIdByObjectId(storage::ObjectMap::Type::Bookmark, sync_record->objectId);
    LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnResolvedBookmarks: local_id=<" << local_id << ">";

    if (sync_record->action == jslib::SyncRecord::Action::CREATE && local_id.empty()) {
      bookmarks_->AddBookmark(*sync_record);
    } else if (sync_record->action == jslib::SyncRecord::Action::DELETE && !local_id.empty()) {
      bookmarks_->DeleteBookmark(*sync_record);
    } else if (sync_record->action == jslib::SyncRecord::Action::UPDATE && !local_id.empty()) {
      bookmarks_->UpdateBookmark(*sync_record);
    }
    // Abnormal cases
    if (sync_record->action == jslib::SyncRecord::Action::DELETE && local_id.empty()) {
      DCHECK(false) << "TAGAB received request to delete bookmark which we don't have";
    } else if (sync_record->action == jslib::SyncRecord::Action::CREATE && !local_id.empty()) {
      DCHECK(false) << "TAGAB received request to create bookmark which already exists";
    } else if (sync_record->action == jslib::SyncRecord::Action::UPDATE && local_id.empty()) {
      DCHECK(false) << "TAGAB received request to update bookmark which we don't have";
    }
  }
}

void ControllerImpl::OnResolvedHistorySites(const RecordsList &records) {
  NOTIMPLEMENTED();
}

void ControllerImpl::OnDeletedSyncUser() {
  NOTIMPLEMENTED();
}

void ControllerImpl::OnDeleteSyncSiteSettings()  {
  NOTIMPLEMENTED();
}

void ControllerImpl::OnSaveBookmarksBaseOrder(const std::string &order)  {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnSaveBookmarksBaseOrder order=<" << order << ">";
  DCHECK(!order.empty());
  sync_prefs_->SetBookmarksBaseOrder(order);
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnSaveBookmarksBaseOrder: forced call of OnSyncReady";
  OnSyncReady();
}

void ControllerImpl::OnSaveBookmarkOrder(const std::string &order,
  const std::string &prev_order, const std::string &next_order) {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnSaveBookmarkOrder";
  LOG(ERROR) << "TAGAB order=" << order;
  LOG(ERROR) << "TAGAB prev_order=" << prev_order;
  LOG(ERROR) << "TAGAB next_order=" << next_order;
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(!prev_order.empty() || !next_order.empty());

  // As I have sent task in UI and receievd the responce in UI,
  // then safe to use per-class storage <prev_order,next_order> => context

  // I need to findout the node local_id somehow
  // And then:
  //            1. apply new order string in obj map
  //            2. send updated bookmark

  int64_t between_order_rr_context_node_id = -1;
  int action = -1;

  PopRRContext(prev_order, next_order, between_order_rr_context_node_id, action);

  LOG(ERROR) << "TAGAB between_order_rr_context_node_id=" << between_order_rr_context_node_id;
  LOG(ERROR) << "TAGAB action=" << action;
  DCHECK(between_order_rr_context_node_id != -1);
  DCHECK(action != -1);

  OnSaveBookmarkOrderInternal(order, between_order_rr_context_node_id, action);
}

void ControllerImpl::OnSaveBookmarkOrderInternal(const std::string &order,
  const int64_t &node_id, const int &action) {

  task_runner_->PostTask(
    FROM_HERE,
    base::Bind(&ControllerImpl::OnSaveBookmarkOrderOrNodeAddedFileWork, base::Unretained(this),
    node_id,
    order,
    action)
  );
}

void ControllerImpl::OnSaveBookmarkOrderOrNodeAddedFileWork(const int64_t &bookmark_local_id,
  const std::string &order, const int &action) {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnSaveBookmarkOrderOrNodeAddedFileWork";
  LOG(ERROR) << "TAGAB bookmark_local_id=" << bookmark_local_id;
  LOG(ERROR) << "TAGAB order=" << order;
  LOG(ERROR) << "TAGAB action=" << action;

  if (action == jslib_const::kActionUpdate) {
    sync_obj_map_->UpdateOrderByLocalObjectId(storage::ObjectMap::Type::Bookmark,
      std::to_string(bookmark_local_id), order);
  } else if (action == jslib_const::kActionCreate) {
    sync_obj_map_->CreateOrderByLocalObjectId(storage::ObjectMap::Type::Bookmark,
      std::to_string(bookmark_local_id), tools::GenerateObjectId(), order);
  } else {
    NOTREACHED();
  }

  const bookmarks::BookmarkNode* node = bookmarks_->GetNodeById(bookmark_local_id);
  LOG(ERROR) << "TAGAB node=" << node;
  DCHECK(node);
  if (!node) {
    return;
  }

  DCHECK(bookmarks_);
  std::unique_ptr<RecordsList> records = bookmarks_->NativeBookmarksToSyncRecords(
    {InitialBookmarkNodeInfo(node, true)},
    std::map<const bookmarks::BookmarkNode*, std::string>(),
    action //jslib_const::kActionUpdate
  );

  DCHECK(records);
  LOG(ERROR) << "TAGAB records->size()=" << records->size();
  DCHECK(records->size() == 1);

  sync_client_->SendSyncRecords(jslib_const::SyncRecordType_BOOKMARKS, *records);
}

void ControllerImpl::PushRRContext(const std::string &prev_order, const std::string &next_order, const int64_t &node_id, const int &action) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  std::tuple<std::string, std::string> key(prev_order, next_order);
  DCHECK(rr_map_.find(key) == rr_map_.end());
  rr_map_[key] = std::make_tuple(node_id, action);
}

void ControllerImpl::PopRRContext(const std::string &prev_order, const std::string &next_order, int64_t &node_id, int &action) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  std::tuple<std::string, std::string> key(prev_order, next_order);
  auto it = rr_map_.find(key);
  DCHECK(it != rr_map_.end());
  node_id = std::get<0>(it->second);
  action = std::get<1>(it->second);
  rr_map_.erase(it);
}

void ControllerImpl::OnSyncWordsPrepared(const std::string &words) {
  TriggerOnHaveSyncWords(words);
}

void ControllerImpl::OnBytesFromSyncWordsPrepared(const Uint8Array &bytes,
  const std::string &error_message) {
  //OnWordsToBytesDone
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnBytesFromSyncWordsPrepared";
  LOG(ERROR) << "TAGAB bytes.size()=" << bytes.size();
  LOG(ERROR) << "TAGAB error_message=" << error_message;

  if (!bytes.empty()) {
    //DCHECK(temp_storage_.seed_str_.empty()); can be not epmty if try to do twice with error on first time
    temp_storage_.seed_str_ = StrFromUint8Array(bytes);
    LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnBytesFromSyncWordsPrepared temp_storage_.seed_str_=<" << temp_storage_.seed_str_ << ">";
    LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnWordsToBytesDone: call InitJsLib";
    InitJsLib(true);//Init will cause load of the Script;
  } else {
    LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnBytesFromSyncWordsPrepared failed, " << error_message;
  }
}

//bool once_done = false;

// Here we query sync lib for the records after initialization (or again later)
void ControllerImpl::RequestSyncData() {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::RequestSyncData:";

  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::RequestSyncData: sync_prefs_->GetSyncThisDevice()=" << sync_prefs_->GetSyncThisDevice();
  if (!sync_prefs_->GetSyncThisDevice()) {
    LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::RequestSyncData: sync is not enabled for this device";
    return;
  }

  const bool bookmarks = sync_prefs_->GetSyncBookmarksEnabled();
  const bool history = sync_prefs_->GetSyncHistoryEnabled();
  const bool preferences = sync_prefs_->GetSyncSiteSettingsEnabled();

  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::RequestSyncData: bookmarks="<<bookmarks;
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::RequestSyncData: history="<<history;
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::RequestSyncData: preferences="<<preferences;

  if (!bookmarks && !history && !preferences) {
    LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::RequestSyncData: none of option is enabled, abort";
    return;
  }

  const int max_records = 300;
  base::Time last_fetch_time = sync_prefs_->GetLastFetchTime();
  base::Time latest_record_time = sync_prefs_->GetLatestRecordTime();

  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::RequestSyncData: last_fetch_time="<<last_fetch_time;

  const int64_t start_at = base::checked_cast<int64_t>(latest_record_time.ToJsTime());
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::RequestSyncData: latest_record_time="<<latest_record_time;
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::RequestSyncData: start_at="<<start_at;

  //bool dbg_ignore_create_device = true; //the logic should rely upon sync_prefs_->GetTimeLastFetch() which is not saved yet
  if (tools::IsTimeEmpty(last_fetch_time)
  /*0 == start_at*/ /*&& !dbg_ignore_create_device*/) {
    //SetUpdateDeleteDeviceName(CREATE_RECORD, mDeviceName, mDeviceId, "");
    SendCreateDevice();
    SendAllLocalBookmarks();
    //SendAllLocalHistorySites();
  }

  FetchSyncRecords(bookmarks, history, preferences, start_at, max_records);
}

void ControllerImpl::FetchSyncRecords(const bool &bookmarks,
  const bool &history, const bool &preferences, int64_t start_at, int max_records) {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::FetchSyncRecords:";
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
  last_time_fetch_sent_ = base::Time::Now();
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::FetchSyncRecords: last_time_fetch_sent_=" << last_time_fetch_sent_;
  base::Time start_at_time = base::Time::FromJsTime(start_at);
  sync_client_->SendFetchSyncRecords(
    category_names,
    start_at_time,
    max_records);
}

namespace {
RecordsListPtr CreateDeviceCreationRecordExtension(
  const std::string &deviceName,
  const std::string &objectId,
  const jslib::SyncRecord::Action &action,
  const std::string &deviceId) {
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
}

void ControllerImpl::SendCreateDevice() {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::SendCreateDevice";

  std::string device_name = sync_prefs_->GetThisDeviceName();
  std::string object_id = brave_sync::tools::GenerateObjectId();
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::SendCreateDevice deviceName=" << device_name;
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::SendCreateDevice objectId=" << object_id;
  std::string device_id = sync_prefs_->GetThisDeviceId();
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::SendCreateDevice deviceId=" << device_id;
  CHECK(!device_id.empty());

  SendDeviceSyncRecord(jslib::SyncRecord::Action::CREATE,
    device_name,
    device_id,
    object_id);
}

void ControllerImpl::SendDeviceSyncRecord(const int &action,
  const std::string &device_name,
  const std::string &device_id,
  const std::string &object_id) {

  DCHECK(sync_client_);

  RecordsListPtr records = CreateDeviceCreationRecordExtension(device_name, object_id,
    static_cast<jslib::SyncRecord::Action>(action),
    device_id);
  sync_client_->SendSyncRecords(jslib_const::SyncRecordType_PREFERENCES, *records);
}

std::vector<std::string> ControllerImpl::SaveGetDeleteNotSyncedRecords(
  const std::string &recordType, const std::string &action,
  const std::vector<std::string> &ids,
  NotSyncedRecordsOperation operation) {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::SaveGetDeleteNotSyncedRecords, early quit";
  return std::vector<std::string>();
  // java SaveGetDeleteNotSyncedRecords
}

void ControllerImpl::SendAllLocalBookmarks() {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::SendAllLocalBookmarks";
  static const int SEND_RECORDS_COUNT_LIMIT = 1000;
  std::vector<InitialBookmarkNodeInfo> localBookmarks;
  std::map<const bookmarks::BookmarkNode*, std::string> order_map;
  bookmarks_->GetInitialBookmarksWithOrders(localBookmarks, order_map);

  for(size_t i = 0; i < localBookmarks.size(); i += SEND_RECORDS_COUNT_LIMIT) {
    size_t sub_list_last = std::min(localBookmarks.size(), i + SEND_RECORDS_COUNT_LIMIT);
    std::vector<InitialBookmarkNodeInfo> sub_list(localBookmarks.begin()+i, localBookmarks.begin()+sub_list_last);
    CreateUpdateDeleteBookmarks(jslib_const::kActionCreate, sub_list, order_map, true, true);
  }
}

void ControllerImpl::CreateUpdateDeleteBookmarks(
  const int &action,
  const std::vector<InitialBookmarkNodeInfo> &list,
  const std::map<const bookmarks::BookmarkNode*, std::string> &order_map,
  const bool &addIdsToNotSynced,
  const bool &isInitialSync) {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::CreateUpdateDeleteBookmarks";
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::CreateUpdateDeleteBookmarks list.empty()=" << list.empty();
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::CreateUpdateDeleteBookmarks sync_initialized_=" << sync_initialized_;
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::CreateUpdateDeleteBookmarks sync_prefs_->GetSyncBookmarksEnabled()=" << sync_prefs_->GetSyncBookmarksEnabled();

  if (list.empty() || !sync_initialized_ || !sync_prefs_->GetSyncBookmarksEnabled() ) {
    return;
  }

  task_runner_->PostTask(
    FROM_HERE,
    base::Bind(&ControllerImpl::CreateUpdateDeleteBookmarksFileWork, base::Unretained(this),
    action,
    list,
    order_map,
    addIdsToNotSynced,
    isInitialSync)
  );
}

void ControllerImpl::CreateUpdateDeleteBookmarksFileWork(
  const int &action,
  const std::vector<InitialBookmarkNodeInfo> &list,
  const std::map<const bookmarks::BookmarkNode*, std::string> &order_map,
  const bool &addIdsToNotSynced,
  const bool &isInitialSync) {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::CreateUpdateDeleteBookmarksFileWork";

  DCHECK(sync_client_);
  std::unique_ptr<RecordsList> records = bookmarks_->NativeBookmarksToSyncRecords(list, order_map, action);
  sync_client_->SendSyncRecords(jslib_const::SyncRecordType_BOOKMARKS, *records);
}

void ControllerImpl::BookmarkMoved(
  const int64_t &node_id,
  const int64_t &prev_item_id,
  const int64_t &next_item_id,
  const int64_t &parent_id) {
  // Should be invoked on FILE-enabled thread
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::BookmarkMoved";

  std::string prev_item_order;
  std::string next_item_order;
  std::string parent_folder_order;

  if (prev_item_id != -1) {
    prev_item_order = sync_obj_map_->GetOrderByLocalObjectId(
      storage::ObjectMap::Type::Bookmark, std::to_string(prev_item_id));
  }
  if (next_item_id != -1) {
    next_item_order = sync_obj_map_->GetOrderByLocalObjectId(
      storage::ObjectMap::Type::Bookmark, std::to_string(next_item_id));
  }

  if (parent_id != -1) {
    parent_folder_order = sync_obj_map_->GetOrderByLocalObjectId(
      storage::ObjectMap::Type::Bookmark, std::to_string(parent_id));
    DCHECK(!parent_folder_order.empty());
  }

  LOG(ERROR) << "TAGAB prev_item_order="<<prev_item_order;
  LOG(ERROR) << "TAGAB next_item_order="<<next_item_order;

  content::BrowserThread::GetTaskRunnerForThread(content::BrowserThread::UI)->PostTask(
    FROM_HERE, base::Bind(&ControllerImpl::BookmarkMovedQueryNewOrderUiWork,
         base::Unretained(this), node_id, prev_item_order, next_item_order,
         parent_folder_order));
}

void ControllerImpl::BookmarkMovedQueryNewOrderUiWork(
  const int64_t &node_id,
  const std::string &prev_item_order,
  const std::string &next_item_order,
  const std::string &parent_folder_order) {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::BookmarkMovedQueryNewOrderUiWork";
  LOG(ERROR) << "TAGAB node_id="<<node_id;
  LOG(ERROR) << "TAGAB prev_item_order="<<prev_item_order;
  LOG(ERROR) << "TAGAB next_item_order="<<next_item_order;
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(sync_client_);

  if (prev_item_order.empty() && next_item_order.empty()) {
    const std::string &order = parent_folder_order + ".1";
    OnSaveBookmarkOrderInternal(order, node_id, jslib_const::kActionUpdate);
  } else {
    PushRRContext(prev_item_order, next_item_order, node_id, jslib_const::kActionUpdate);

    sync_client_->SendGetBookmarkOrder(prev_item_order, next_item_order);
    // See later in OnSaveBookmarkOrder
  }
}

void ControllerImpl::BookmarkAdded(
  const int64_t &node_id,
  const int64_t &prev_item_id,
  const int64_t &next_item_id,
  const int64_t &parent_id) {
  // Should be invoked on FILE-enabled thread
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::BookmarkAdded";
  LOG(ERROR) << "TAGAB node_id="<<node_id;
  LOG(ERROR) << "TAGAB prev_item_id="<<prev_item_id;
  LOG(ERROR) << "TAGAB next_item_id="<<next_item_id;
  LOG(ERROR) << "TAGAB parent_id="<<parent_id;

  std::string prev_item_order;
  std::string next_item_order;
  std::string parent_folder_order;

  if (prev_item_id != -1) {
    prev_item_order = sync_obj_map_->GetOrderByLocalObjectId(
      storage::ObjectMap::Type::Bookmark, std::to_string(prev_item_id));
    DCHECK(!prev_item_order.empty());
  }
  if (next_item_id != -1) {
    next_item_order = sync_obj_map_->GetOrderByLocalObjectId(
      storage::ObjectMap::Type::Bookmark, std::to_string(next_item_id));
    DCHECK(!next_item_order.empty());
  }

  if (parent_id != -1) {
    parent_folder_order = sync_obj_map_->GetOrderByLocalObjectId(
      storage::ObjectMap::Type::Bookmark, std::to_string(parent_id));
    DCHECK(!parent_folder_order.empty());
  }

  LOG(ERROR) << "TAGAB prev_item_order="<<prev_item_order;
  LOG(ERROR) << "TAGAB next_item_order="<<next_item_order;
  LOG(ERROR) << "TAGAB parent_folder_order="<<parent_folder_order;

  content::BrowserThread::GetTaskRunnerForThread(content::BrowserThread::UI)->PostTask(
    FROM_HERE, base::Bind(&ControllerImpl::BookmarkAddedQueryNewOrderUiWork,
         base::Unretained(this), node_id, prev_item_order, next_item_order, parent_folder_order));
}

void ControllerImpl::BookmarkAddedQueryNewOrderUiWork(
  const int64_t &node_id,
  const std::string &prev_item_order,
  const std::string &next_item_order,
  const std::string &parent_folder_order) {

  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::BookmarkAddedQueryNewOrderUiWork";
  LOG(ERROR) << "TAGAB node_id="<<node_id;
  LOG(ERROR) << "TAGAB prev_item_order="<<prev_item_order;
  LOG(ERROR) << "TAGAB next_item_order="<<next_item_order;
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(sync_client_);

  DCHECK(!prev_item_order.empty() || !next_item_order.empty() || !parent_folder_order.empty());

  if (prev_item_order.empty() && next_item_order.empty()) {
    // Special case, both prev_item_order and next_item_order are empty
    // Can happened when sync is initialized and add bookmark into empty folder
    const std::string &order = parent_folder_order + ".1";
    OnSaveBookmarkOrderInternal(order, node_id, jslib_const::kActionCreate);
  } else {
    PushRRContext(prev_item_order, next_item_order, node_id, jslib_const::kActionCreate);
    sync_client_->SendGetBookmarkOrder(prev_item_order, next_item_order);
    // See later in OnSaveBookmarkOrder
  }
}

void ControllerImpl::SendAllLocalHistorySites() {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::SendAllLocalHistorySites";
  ;
  ///static const int SEND_RECORDS_COUNT_LIMIT = 1000;
  history_->GetAllHistory();

  // for(size_t i = 0; i < localBookmarks.size(); i += SEND_RECORDS_COUNT_LIMIT) {
  //   size_t sub_list_last = std::min(localBookmarks.size(), i + SEND_RECORDS_COUNT_LIMIT);
  //   std::vector<const bookmarks::BookmarkNode*> sub_list(localBookmarks.begin()+i, localBookmarks.begin()+sub_list_last);
  //   CreateUpdateDeleteBookmarks(jslib_const::kActionCreate, sub_list, true, true);
  // }
}

void ControllerImpl::HaveInitialHistory(history::QueryResults* results) {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::HaveInitialHistory";
  static const int SEND_RECORDS_COUNT_LIMIT = 1000;

  if (!results || results->empty() || !sync_initialized_ || !sync_prefs_->GetSyncHistoryEnabled() ) {
    return;
  }

  if (results && !results->empty()) {
    LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::HaveInitialHistory results->size()=" << results->size();
    for (const auto& item : *results){
      //history_item_vec.push_back(GetHistoryItem(item));
      //item.visit_time();
      item.visit_time();
      LOG(ERROR) << "brave_sync::ControllerImpl::HaveInitialHistory item=" << item.url().spec();
    }

    for(size_t i = 0; i < results->size(); i += SEND_RECORDS_COUNT_LIMIT) {
      size_t sub_list_last = std::min(results->size(), i + SEND_RECORDS_COUNT_LIMIT);
      history::QueryResults::URLResultVector sub_list(results->begin()+i, results->begin()+sub_list_last);

      CreateUpdateDeleteHistorySites(jslib_const::kActionCreate, sub_list, true, true);
    }
  }

  // Convert
  // Send Actually created sync
  ;

}


void ControllerImpl::CreateUpdateDeleteHistorySites(
  const int &action,
  //const history::QueryResults::URLResultVector &list,
  const std::vector<history::URLResult> &list,
  const bool &addIdsToNotSynced,
  const bool &isInitialSync) {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::CreateUpdateDeleteHistorySites";

  if (list.empty() || !sync_initialized_ || !sync_prefs_->GetSyncHistoryEnabled() ) {
    return;
  }

  DCHECK(sync_client_);
  std::unique_ptr<RecordsList> records = history_->NativeHistoryToSyncRecords(list, action);
  sync_client_->SendSyncRecords(jslib_const::SyncRecordType_HISTORY, *records);
}

static const int64_t kCheckUpdatesIntervalSec = 60;

void ControllerImpl::StartLoop() {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::StartLoop " << GetThreadInfoString();
  // Repeated task runner
  //https://chromium.googlesource.com/chromium/src/+/lkgr/docs/threading_and_tasks.md#posting-a-repeating-task-with-a-delay
  timer_->Start(FROM_HERE, base::TimeDelta::FromSeconds(kCheckUpdatesIntervalSec),
                 this, &ControllerImpl::LoopProc);
  //in UI THREAD
}

void ControllerImpl::StopLoop() {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::StopLoop " << GetThreadInfoString();
  timer_->Stop();
  //in UI THREAD
}

void ControllerImpl::LoopProc() {
//  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::LoopProc " << GetThreadInfoString();

  // task_runner_->PostTask(FROM_HERE,
  // base::Bind(&ControllerImpl::LoopProcThreadAligned,
  //     base::Unretained(this)));
  //in UI THREAD

  LoopProcThreadAligned();
  // For now cannot run LoopProcThreadAligned in a task runner because it uses
  // sync_prefs_ which should be accessed in UI thread
}

void ControllerImpl::LoopProcThreadAligned() {
//  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::LoopProcThreadAligned " << GetThreadInfoString();

  //LOG(ERROR) << "ChromeNetworkDelegate " << " PID=" << getpid() << " tid="<< gettid();`
  // Ensure not UI thread
  // Ensure on a good sequence

  //UNKNOWN THREAD
  //where Chromium runs sync tasks
  // Chromium uses
  // GetUpdatesProcessor::PrepareGetUpdates 001  tid=13213 IsThreadInitialized(UI)=1 IsThreadInitialized(IO)=1 UNKNOWN THREAD
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!sync_initialized_) {
    return;
  }

  RequestSyncData();
}

base::SequencedTaskRunner *ControllerImpl::GetTaskRunner() {
  return task_runner_.get();
}

void ControllerImpl::TriggerOnLogMessage(const std::string &message) {
  for (auto& observer : observers_)
    observer.OnLogMessage(this, message);
}

void ControllerImpl::TriggerOnSyncStateChanged() {
  for (auto& observer : observers_)
    observer.OnSyncStateChanged(this);
}

void ControllerImpl::TriggerOnHaveSyncWords(const std::string &sync_words) {
  for (auto& observer : observers_)
    observer.OnHaveSyncWords(this, sync_words);
}

} // namespace brave_sync
