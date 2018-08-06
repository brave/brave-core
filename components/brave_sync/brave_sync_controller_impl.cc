/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/brave_sync_controller_impl.h"

#include <sstream>

#include "base/debug/stack_trace.h"
#include "base/json/json_reader.h"
#include "base/numerics/safe_conversions.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/task_runner.h"
#include "base/task_scheduler/post_task.h"
#include "base/task_scheduler/scheduler_worker_pool.h"
#include "base/time/time.h"
#include "base/values.h"

#include "brave/browser/extensions/api/brave_sync/brave_sync_event_router.h"
#include "brave/browser/ui/brave_pages.h"
#include "brave/browser/ui/webui/sync/sync_ui.h"
#include "brave/components/brave_sync/brave_sync_bookmarks.h"
#include "brave/components/brave_sync/brave_sync_devices.h"
#include "brave/components/brave_sync/brave_sync_profile_prefs.h"
#include "brave/components/brave_sync/brave_sync_settings.h"
#include "brave/components/brave_sync/brave_sync_jslib_const.h"
#include "brave/components/brave_sync/brave_sync_jslib_messages.h"
#include "brave/components/brave_sync/brave_sync_obj_map.h"
#include "brave/components/brave_sync/brave_sync_tools.h"
#include "brave/components/brave_sync/debug.h"
#include "brave/components/brave_sync/values_conv.h"
#include "brave/components/brave_sync/value_debug.h"

#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser.h"
#include "content/public/browser/browser_thread.h"

namespace brave_sync {

BraveSyncControllerImpl::TempStorage::TempStorage() {
}

BraveSyncControllerImpl::TempStorage::~TempStorage() {
}

BraveSyncControllerImpl::BraveSyncControllerImpl() :
  sync_ui_(nullptr),
  sync_js_layer_(nullptr),
  sync_initialized_(false),
  timer_(std::make_unique<base::RepeatingTimer>()) {
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::BraveSyncControllerImpl CTOR";

  DETACH_FROM_SEQUENCE(sequence_checker_);

  sync_prefs_.reset(new brave_sync::prefs::BraveSyncPrefs(nullptr)); //this is wrong. TODO, AB: pass the pointer

  std::unique_ptr<BraveSyncSettings> settings_ = std::make_unique<BraveSyncSettings>();

  std::unique_ptr<BraveSyncSettings> settingsTest = sync_prefs_->GetBraveSyncSettings();
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::BraveSyncControllerImpl settingsTest";
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::BraveSyncControllerImpl settingsTest->this_device_name_=" << settingsTest->this_device_name_;
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::BraveSyncControllerImpl settingsTest->sync_this_device_=" << settingsTest->sync_this_device_;

  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::BraveSyncControllerImpl sync_prefs_->GetSeed()=<" << sync_prefs_->GetSeed() <<">";
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::BraveSyncControllerImpl sync_prefs_->GetThisDeviceName()=<" << sync_prefs_->GetThisDeviceName() <<">";

  // volatile bool b_DBG_clear_on_start = true;
  // if (b_DBG_clear_on_start) {
  //   LOG(ERROR) << "TAGAB BraveSyncControllerImpl::BraveSyncControllerImpl clearing prefs";
  //   sync_prefs_->Clear();
  // }

  // volatile bool b_DBG_setname_on_start = true;
  // if (b_DBG_setname_on_start) {
  //   LOG(ERROR) << "TAGAB BraveSyncControllerImpl::BraveSyncControllerImpl set name to cf56";
  //   sync_prefs_->SetDeviceName("cf56");
  // LOG(ERROR) << "TAGAB BraveSyncControllerImpl::BraveSyncControllerImpl sync_prefs_->GetThisDeviceName()=" << sync_prefs_->GetThisDeviceName();
  //   DCHECK(false);
  // }

  task_runner_ = base::CreateSequencedTaskRunnerWithTraits(
      {base::MayBlock(), base::TaskPriority::BACKGROUND,
       base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN} );

  sync_obj_map_ = std::make_unique<storage::BraveSyncObjMap>();

  bookmarks_ = std::make_unique<BraveSyncBookmarks>(this);

  if (!sync_prefs_->GetThisDeviceId().empty()) {
    bookmarks_->SetThisDeviceId(sync_prefs_->GetThisDeviceId());
  }
  bookmarks_->SetObjMap(sync_obj_map_.get());

  BrowserList::GetInstance()->AddObserver(this);

  if (!sync_prefs_->GetSeed().empty() && !sync_prefs_->GetThisDeviceName().empty()) {
    LOG(ERROR) << "TAGAB BraveSyncControllerImpl::BraveSyncControllerImpl sync is configured";

    task_runner_->PostTask(FROM_HERE,
    base::Bind(&BraveSyncControllerImpl::InitJsLib,
        base::Unretained(this), false));

  } else {
    LOG(ERROR) << "TAGAB BraveSyncControllerImpl::BraveSyncControllerImpl sync is NOT configured";
  }

  StartLoop();
}

BraveSyncControllerImpl::~BraveSyncControllerImpl() {
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::~BraveSyncControllerImpl DTOR";
  BrowserList::GetInstance()->RemoveObserver(this);

  StopLoop();
}

BraveSyncControllerImpl* BraveSyncControllerImpl::GetInstance() {
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::GetInstance";
  //LOG(ERROR) << base::debug::StackTrace().ToString();

  return base::Singleton<BraveSyncControllerImpl>::get();
}

void BraveSyncControllerImpl::OnSetupSyncHaveCode(const std::string &sync_words,
  const std::string &device_name) {
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::OnSetupSyncHaveCode";
  LOG(ERROR) << "TAGAB sync_words=" << sync_words;
  LOG(ERROR) << "TAGAB device_name=" << device_name;

  temp_storage_.device_name_ = device_name; // Fill here, but save in OnSaveInitData

  std::string arg1;
  arg1= "\"" + sync_words + "\"";
  CallJsLibStr("words_to_bytes", arg1, "", "", "");
}

void BraveSyncControllerImpl::OnSetupSyncNewToSync(const std::string &device_name) {
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::OnSetupSyncNewToSync";
  LOG(ERROR) << "TAGAB device_name="<<device_name;

  temp_storage_.device_name_ = device_name; // Fill here, but save in OnSaveInitData

  InitJsLib(true); // Init will cause load of the Script
  // Then we will got GOT_INIT_DATA and SAVE_INIT_DATA, where we will save the seed and device id
  // Then when we will receive sync_ready, we should display web page with sync settings
}

void BraveSyncControllerImpl::OnDeleteDevice(const std::string &device_id) {
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::OnDeleteDevice";
  LOG(ERROR) << "TAGAB device_id="<<device_id;

  CHECK(sync_js_layer_ != nullptr);
  CHECK(sync_initialized_);

  std::string json = sync_obj_map_->GetObjectIdByLocalId(jslib_const::DEVICES_NAMES);
  SyncDevices syncDevices;
  syncDevices.FromJson(json);
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::OnDeleteDevice json="<<json;

  const SyncDevice *device = syncDevices.GetByDeviceId(device_id);
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::OnDeleteDevice device="<<device;
  //DCHECK(device); // once I saw it nullptr
  if (device) {
    const std::string device_name = device->name_;
    const std::string object_id = device->object_id_;
    LOG(ERROR) << "TAGAB BraveSyncControllerImpl::OnDeleteDevice device_name="<<device_name;
    LOG(ERROR) << "TAGAB BraveSyncControllerImpl::OnDeleteDevice object_id="<<object_id;

    SetUpdateDeleteDeviceName(
      jslib_const::DELETE_RECORD,
      device_name,
      device_id,
      object_id);
  }
}

void BraveSyncControllerImpl::OnResetSync() {
  LOG(ERROR) << "TAGAB  BraveSyncControllerImpl::OnResetSync";
  CHECK(sync_js_layer_ != nullptr);
  CHECK(sync_initialized_);

  const std::string device_id = sync_prefs_->GetThisDeviceId();
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::OnResetSync device_id="<<device_id;
  OnDeleteDevice(device_id);

  sync_prefs_->Clear();

  sync_obj_map_->DestroyDB();

  if (sync_ui_) {
    sync_ui_->OnSyncStateChanged();
  }

  // Close js lib pseudo-tab
}

void BraveSyncControllerImpl::GetSettings(BraveSyncSettings &settings) {
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::GetSettings";

  std::unique_ptr<BraveSyncSettings> bss = sync_prefs_->GetBraveSyncSettings();
  settings = *std::move(bss);

  LOG(ERROR) << "TAGAB settings.this_device_name_=<" << settings.this_device_name_ << ">";
  LOG(ERROR) << "TAGAB settings.sync_this_device_=<" << settings.sync_this_device_ << ">";
  LOG(ERROR) << "TAGAB sync_prefs_->GetSeed()=<" << sync_prefs_->GetSeed() << ">";
  LOG(ERROR) << "TAGAB sync_prefs_->GetThisDeviceName()=<" << sync_prefs_->GetThisDeviceName() << ">";

  settings.sync_configured_ = !sync_prefs_->GetSeed().empty() &&
    !sync_prefs_->GetThisDeviceName().empty();

  LOG(ERROR) << "TAGAB settings.sync_configured_=<" << settings.sync_configured_ << ">";
}

void BraveSyncControllerImpl::GetDevices(SyncDevices &devices) {
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::GetDevices";

//   // fetch devices
//   FetchSyncRecords(false, false, true, 0, 300);
// LOG(ERROR) << "TAGAB BraveSyncControllerImpl::GetDevices, request FetchSyncRecords is done";
// ^ causes recursion

  std::string json = sync_obj_map_->GetObjectIdByLocalId(jslib_const::DEVICES_NAMES);
  SyncDevices syncDevices;
  syncDevices.FromJson(json);
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::GetDevices json="<<json;
  devices = syncDevices;
}

void BraveSyncControllerImpl::GetSyncWords() {
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::GetSyncWords";

  // Ask JS library
  std::string seed = sync_prefs_->GetSeed();
  std::string arg1= "\"" + seed + "\"";
  CallJsLibStr("bytes_to_words", arg1, "", "", "");
}

std::string BraveSyncControllerImpl::GetSeed() {
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::GetSeed";
  std::string seed = sync_prefs_->GetSeed();
  return seed;
}

void BraveSyncControllerImpl::LoadJsLibPseudoTab() {
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::LoadJsLibPseudoTab";

  // TODO, AB: this is not good.
  // Possible situation:
  // 1) open browser A
  // 2) create tab with js lib in tab A
  // 3) create browser B
  // 4) close browser B
  // either to move js lib into V8 or subscribe on BrowserListObserver events
  // so during BrowserListObserver::OnBrowserRemoved do re-init of sync lib
  Browser* browser = BrowserList::GetInstance()->GetLastActive();

  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::LoadJsLibPseudoTab browser=" << browser;

  if (browser) {
    brave::LoadBraveSyncJsLib(browser);
  } else {
    // Well, wait for the browser to be loaded, do work in BraveSyncControllerImpl::OnBrowserAdded
    LOG(ERROR) << "TAGAB BraveSyncControllerImpl::LoadJsLibPseudoTab browser=(NIL)!";
  }
}

void BraveSyncControllerImpl::OnBrowserAdded(Browser* browser) {
  LOG(ERROR) << "TAGAB  BraveSyncControllerImpl::OnBrowserAdded browser="<<browser;
}

void BraveSyncControllerImpl::OnBrowserSetLastActive(Browser* browser) {
  LOG(ERROR) << "TAGAB  BraveSyncControllerImpl::OnBrowserSetLastActive browser="<<browser;
  browser_ = browser;
  bookmarks_->SetBrowser(browser);

  //TODO, AB: need several profiles, BraveSyncControllerImpl per profile
  if (!brave_sync_event_router_) {
    brave_sync_event_router_ = std::make_unique<extensions::BraveSyncEventRouter>(browser_->profile());
  }

  LOG(ERROR) << "TAGAB  BraveSyncControllerImpl::OnBrowserSetLastActive sync_js_layer_="<<sync_js_layer_;
  if (sync_js_layer_) {
    return;
  }

  content::BrowserThread::GetTaskRunnerForThread(content::BrowserThread::UI)->PostTask(
    FROM_HERE, base::Bind(&BraveSyncControllerImpl::InitJsLib,
         base::Unretained(this), false));
}

void BraveSyncControllerImpl::InitJsLib(const bool &setup_new_sync) {
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::InitJsLib " << GetThreadInfoString();

  if (!sync_js_layer_) {
    LoadJsLibPseudoTab();
    return;
  }
LOG(ERROR) << "TAGAB BraveSyncControllerImpl::InitJsLib (2) " << GetThreadInfoString();
  if ( (!sync_prefs_->GetSeed().empty() && !sync_prefs_->GetThisDeviceName().empty()) ||
      setup_new_sync) {
    LOG(ERROR) << "TAGAB BraveSyncControllerImpl::InitJsLib sync is active or setup_new_sync";
    sync_js_layer_->LoadJsLibScript();
  } else {
    LOG(ERROR) << "TAGAB BraveSyncControllerImpl::InitJsLib sync is NOT active";
  }

}

void BraveSyncControllerImpl::CallJsLibBV(const base::Value &command,
  const base::Value &arg1, const base::Value &arg2, const base::Value &arg3,
  const base::Value &arg4) {
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::CallJsLibBV";
  DCHECK(nullptr != sync_js_layer_);
  if (!sync_js_layer_) {
    return;
  }

  const std::vector<const base::Value*> args = {&command, &arg1, &arg2, &arg3, &arg4};
  sync_js_layer_->RunCommandBV(args);
}

void BraveSyncControllerImpl::CallJsLibStr(const std::string &command,
  const std::string &arg1, const std::string &arg2, const std::string &arg3,
  const std::string &arg4) {
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::CallJsLibStr";
  DCHECK(nullptr != sync_js_layer_);
  if (!sync_js_layer_) {
    return;
  }

  sync_js_layer_->RunCommandStr(command, arg1, arg2, arg3, arg4);
}

void BraveSyncControllerImpl::SetupJsLayer(SyncJsLayer *sync_js_layer) {
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::SetupJsLayer sync_js_layer=" << sync_js_layer;
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::SetupJsLayer this->sync_js_layer_=" << this->sync_js_layer_;
  DCHECK(sync_js_layer);
  DCHECK(sync_js_layer_ == nullptr);

  sync_js_layer_ = sync_js_layer;
}

void BraveSyncControllerImpl::SetupUi(SyncUI *sync_ui) {
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::SetupUi sync_ui=" << sync_ui;

  DCHECK(sync_ui);
  DCHECK(sync_ui_ == nullptr);

  sync_ui_ = sync_ui;
}

void BraveSyncControllerImpl::OnJsLibMessage(const std::string &message, const base::ListValue* args) {
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::OnJsLibMessage, message=" << message;
  if (message == "words_to_bytes_done") {
    OnWordsToBytesDone(args);
  } else if (message == "bytes_to_words_done") {
    OnBytesToWordsDone(args);
  }
  else if (message == "get-init-data") {
    ;
  } else if (message == "got-init-data") {
    OnGotInitData(args);
  } else if (message == "save-init-data") {
    OnSaveInitData(args);
  } else if (message == "sync-ready") {
    OnSyncReady(args);
  } else if (message == "get-existing-objects") {
    OnGetExistingObjects(args);
  } else if (message == "resolved-sync-records") {
    OnResolvedSyncRecords(args);
  } else if (message == "sync-debug") {
    OnSyncDebug(args);
  }
}

void BraveSyncControllerImpl::OnGotInitData(const base::ListValue* args) {
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::OnGotInitData";

  //should answer to the lib with words
  //javascript:callbackList['got-init-data'](null,
  //[,,,,],
  //null,
  //{apiVersion: '0', serverUrl: 'https://sync-staging.brave.com', debug: true})
  base::DictionaryValue config;
  config.SetKey("apiVersion", base::Value("0"));
  config.SetKey("serverUrl", base::Value("https://sync-staging.brave.com"));
  config.SetKey("debug", base::Value(true));

  base::Value command("got-init-data");
  // Have 3 options:
  //   1. Start new chain
  //   2. Connect to other sync chain
  //   3. Already connected (or started) to the chain
  // For 1. and 3. get seed from temp_storage_.seed_ because we didn't save it
  // For 2. take seed from sync_prefs_->GetSeed()
  // TODO, AB: can I distinguish 3. from 1. and 2. to put DCHECKs ?

  std::unique_ptr<base::Value> lv_seed = std::make_unique<base::Value>(base::Value::Type::NONE);
  if (!temp_storage_.seed_.empty()) {
    lv_seed = VecToListValue(temp_storage_.seed_);
  } else if (!sync_prefs_->GetSeed().empty()) {
    lv_seed = BytesListFromString(sync_prefs_->GetSeed());
  } else {
    // We are starting a new chain, so we don't know neither seed nor device id
  }
  //CHECK(lv_seed->is_list());
  //CHECK(!lv_seed->GetList().empty());

  std::unique_ptr<base::Value> lv_deviceId = std::make_unique<base::Value>(base::Value::Type::NONE);
  if (!sync_prefs_->GetThisDeviceId().empty()) {
    lv_deviceId = SingleIntStrToListValue(sync_prefs_->GetThisDeviceId());
  }
  //CHECK(lv_deviceId->is_list());
  //CHECK(!lv_deviceId->GetList().empty());

  CHECK(lv_seed);
  CHECK(lv_deviceId);
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::OnGotInitData: lv_seed=" << brave::debug::ToPrintableString(*lv_seed);
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::OnGotInitData: lv_deviceId=" << brave::debug::ToPrintableString(*lv_deviceId);

  CallJsLibBV(command, base::Value(), *lv_seed, *lv_deviceId, config);
}

void BraveSyncControllerImpl::OnWordsToBytesDone(const base::ListValue* args) {
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::OnWordsToBytesDone";
  LOG(ERROR) << "TAGAB args->GetList().size()=" << args->GetList().size();

  DCHECK(temp_storage_.seed_str_.empty());
  //Data is binary
  DCHECK(args->GetList()[1].is_blob());
  const base::Value::BlobStorage& bs = args->GetList()[1].GetBlob();

  temp_storage_.seed_str_.clear();
  for (size_t i = 0; i < bs.size(); ++i) {
    temp_storage_.seed_.push_back(bs[i]);

    temp_storage_.seed_str_ += base::IntToString(/*base::checked_cast*/static_cast<uint8_t>(bs[i]));
    if (i != bs.size() - 1) {
      temp_storage_.seed_str_ += ",";
    }
  }

  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::OnWordsToBytesDone: call InitJsLib";
  InitJsLib(true);//Init will cause load of the Script;
}

void BraveSyncControllerImpl::OnBytesToWordsDone(const base::ListValue* args) {
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::OnWordsToBytesDone";
  LOG(ERROR) << "TAGAB args->GetList().size()=" << args->GetList().size();
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::OnWordsToBytesDone" <<
    brave::debug::ToPrintableString(*args);

  CHECK(sync_ui_);

  DCHECK(args->GetList()[0].GetString() == "bytes_to_words_done");
  DCHECK(args->GetList()[1].is_string());

  std::string words = args->GetList()[1].GetString();

  sync_ui_->OnHaveSyncWords(words);
}

void BraveSyncControllerImpl::OnSyncReady(const base::ListValue* args) {
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::OnSyncReady:";
  DCHECK(false == sync_initialized_);
  sync_initialized_ = true;


  if (sync_ui_) {
    LOG(ERROR) << "TAGAB BraveSyncControllerImpl::OnSyncReady: have sync ui, inform state changed";
    // inform WebUI page that data is ready
    // changed this device name and id
    sync_ui_->OnSyncStateChanged();
  } else {
    // it can be ui page is not opened yet
    LOG(ERROR) << "TAGAB BraveSyncControllerImpl::OnSyncReady: sync_ui_ is null";
  }

  // fetch the records
  RequestSyncData();
}

// Here we query sync lib for the records after initialization (or again later)
void BraveSyncControllerImpl::RequestSyncData() {
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::RequestSyncData:";

  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::RequestSyncData: sync_prefs_->GetSyncThisDevice()=" << sync_prefs_->GetSyncThisDevice();
  if (!sync_prefs_->GetSyncThisDevice()) {
    LOG(ERROR) << "TAGAB BraveSyncControllerImpl::RequestSyncData: sync is not enabled for this device";
    return;
  }

  const bool bookmarks = sync_prefs_->GetSyncBookmarksEnabled();
  const bool history = sync_prefs_->GetSyncHistoryEnabled();
  const bool preferences = sync_prefs_->GetSyncSiteSettingsEnabled();

  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::RequestSyncData: bookmarks="<<bookmarks;
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::RequestSyncData: history="<<history;
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::RequestSyncData: preferences="<<preferences;

  if (!bookmarks && !history && !preferences) {
    LOG(ERROR) << "TAGAB BraveSyncControllerImpl::RequestSyncData: none of option is enabled, abort";
    return;
  }

  //const int64_t start_at = sync_prefs_->GetTimeLastFetch();
  base::Time last_record_time = sync_prefs_->GetLatestRecordTime();
  const int64_t start_at = base::checked_cast<int64_t>(last_record_time.ToJsTime());
  const int max_records = 300;
  base::Time last_fetch_time = sync_prefs_->GetLastFetchTime();

  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::RequestSyncData: start_at="<<start_at;
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::RequestSyncData: last_fetch_time="<<last_fetch_time;
  //bool dbg_ignore_create_device = true; //the logic should rely upon sync_prefs_->GetTimeLastFetch() which is not saved yet
  if (last_fetch_time.is_null()
  /*0 == start_at*/ /*&& !dbg_ignore_create_device*/) {
    //SetUpdateDeleteDeviceName(CREATE_RECORD, mDeviceName, mDeviceId, "");
    SendCreateDevice();
    SendAllLocalBookmarks();
    SendAllLocalHistorySites();
  }

  FetchSyncRecords(bookmarks, history, preferences, start_at, max_records);
  sync_prefs_->SetLastFetchTime(base::Time::Now());
  // save last received record time in OnResolved
}

void BraveSyncControllerImpl::OnSaveInitData(const base::ListValue* args) {
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::OnSaveInitData:";
  DCHECK(false == sync_initialized_);

  LOG(ERROR) << "TAGAB: *args=" << brave::debug::ToPrintableString(*args);

  DCHECK(args->GetList()[1].is_string());
  DCHECK(args->GetList()[2].is_string());

  std::string seed = args->GetList()[1].GetString();
  std::string device_id = args->GetList()[2].GetString();

  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::OnSaveInitData: seed=<"<<seed<<">";
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::OnSaveInitData: device_id="<<device_id<<">";

  if (temp_storage_.seed_str_.empty() && !seed.empty()) {
    temp_storage_.seed_str_ = seed;
  }

  // Check existing values
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::OnSaveInitData: GetThisDeviceId()="<<sync_prefs_->GetThisDeviceId();
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::OnSaveInitData: GetSeed()="<<sync_prefs_->GetSeed();
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::OnSaveInitData: GetThisDeviceName()="<<sync_prefs_->GetThisDeviceName();

  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::OnSaveInitData: temp_storage_.seed_str_="<<temp_storage_.seed_str_;

  if (temp_storage_.device_name_.empty()) {
    temp_storage_.device_name_ = sync_prefs_->GetThisDeviceName();
  }

  //Save
  sync_prefs_->SetThisDeviceId(device_id);
  bookmarks_->SetThisDeviceId(device_id);
  // If we have already initialized sync earlier we don't receive seed again
  // and do not save it
  if (!temp_storage_.seed_str_.empty()) {
    sync_prefs_->SetSeed(temp_storage_.seed_str_);
  }
  sync_prefs_->SetDeviceName(temp_storage_.device_name_);//here I can have empty string, why ?
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::OnSaveInitData: saved device_id="<<device_id;
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::OnSaveInitData: saved seed="<<temp_storage_.seed_str_;
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::OnSaveInitData: saved temp_storage_.device_name_="<<temp_storage_.device_name_;

  sync_prefs_->SetSyncThisDevice(true);

  sync_prefs_->SetSyncBookmarksEnabled(true);
  sync_prefs_->SetSyncSiteSettingsEnabled(true);
  sync_prefs_->SetSyncHistoryEnabled(true);
}

void BraveSyncControllerImpl::OnGetExistingObjects(const base::ListValue* args) {
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::OnGetExistingObjects:";
  /**
   * webview -> browser
   * after sync gets records, it requests the browser's existing objects so sync
   * can perform conflict resolution.
   * isTruncated is true if maxRecords was used and the total number of
   * records exceeds the limit.
   */
//  GET_EXISTING_OBJECTS: _, /* @param {string} categoryName, @param {Array.<Object>} records, @param {lastRecordTimeStamp} number, @param {boolean} isTruncated */

  std::string category_name = args->GetList()[1].GetString();
  std::string records_json = args->GetList()[2].GetString();
  std::string last_record_timestamp = args->GetList()[3].GetString();
  bool is_truncated = args->GetList()[4].GetBool();

  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::OnGetExistingObjects: category_name="<<category_name;
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::OnGetExistingObjects: last_record_timestamp="<<last_record_timestamp;
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::OnGetExistingObjects: is_truncated="<<is_truncated;

  //JSON ==> Value
  int error_code_out = 0;
  std::string error_msg_out;
  int error_line_out = 0;
  int error_column_out = 0;
  std::unique_ptr<base::Value> records_v = base::JSONReader::ReadAndReturnError(
    records_json,
    base::JSONParserOptions::JSON_PARSE_RFC,
    &error_code_out,
    &error_msg_out,
    &error_line_out,
    &error_column_out);

  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::OnGetExistingObjects: records_v.get()="<<records_v.get();
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::OnGetExistingObjects: error_msg_out="<<error_msg_out;
  DCHECK(records_v);
  if (!records_v) {
    return;
  }

  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::OnGetExistingObjects records_v->type()="<< base::Value::GetTypeName(records_v->type());
  DCHECK(records_v->is_list());

  //LOG(ERROR) << "TAGAB BraveSyncControllerImpl::OnGetExistingObjects before ToPrintableString";
  //LOG(ERROR) << "TAGAB BraveSyncControllerImpl::OnGetExistingObjects" << std::endl << ToPrintableString(*records_v);

  //should:
  // resolve
  // then send data with RESOLVE_SYNC_RECORDS
  // then receive RESOLVED_SYNC_RECORDS

  std::unique_ptr<base::Value> resolvedResponse;

  resolvedResponse = PrepareResolvedResponse(category_name, records_v);
  SendResolveSyncRecords(category_name, resolvedResponse.get());
}

void BraveSyncControllerImpl::OnResolvedSyncRecords(const base::ListValue* args) {
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::OnResolvedSyncRecords:";
  /**
   * webview -> browser
   * browser must update its local values with the resolved sync records.
   */
   //RESOLVED_SYNC_RECORDS: _, /* @param {string} categoryName, @param {Array.<Object>} records */;

   std::string category_name = args->GetList()[1].GetString();
   std::string records_json = args->GetList()[2].GetString();

   LOG(ERROR) << "TAGAB BraveSyncControllerImpl::OnResolvedSyncRecords: category_name="<<category_name;
   LOG(ERROR) << "TAGAB BraveSyncControllerImpl::OnResolvedSyncRecords: records_json="<<records_json;

   // TODO, AB: Maybe direct HandleMessage without stringizing can avoid (data)=>JSON=>(value)
   // JSON ==> Value
   int error_code_out = 0;
   std::string error_msg_out;
   int error_line_out = 0;
   int error_column_out = 0;
   std::unique_ptr<base::Value> records_v = base::JSONReader::ReadAndReturnError(
     records_json,
     base::JSONParserOptions::JSON_PARSE_RFC,
     &error_code_out,
     &error_msg_out,
     &error_line_out,
     &error_column_out);

   LOG(ERROR) << "TAGAB BraveSyncControllerImpl::OnResolvedSyncRecords: records_v.get()="<<records_v.get();
   LOG(ERROR) << "TAGAB BraveSyncControllerImpl::OnResolvedSyncRecords: error_msg_out="<<error_msg_out;
   DCHECK(records_v);
   if (!records_v) {
     return;
   }

   LOG(ERROR) << "TAGAB BraveSyncControllerImpl::OnResolvedSyncRecords: ToPrintableString=" << std::endl << brave::debug::ToPrintableString(*records_v);

   // Get latest received record time
   base::Time latest_record_time;
   for (const auto &val : records_v->GetList() ) {
     const base::Value *p_sync_timestampt = val.FindKey("syncTimestamp");
     double d_sync_timestampt = p_sync_timestampt->GetDouble();
     base::Time timeJs = base::Time::FromJsTime(d_sync_timestampt);
     if (timeJs > latest_record_time) {
        latest_record_time = timeJs;
     }
   }
   sync_prefs_->SetLatestRecordTime(latest_record_time);

  if (category_name == brave_sync::jslib_const::kPreferences) {
    OnResolvedPreferences(category_name, std::move(records_v));
  } else if (category_name == brave_sync::jslib_const::kBookmarks) {
    OnResolvedBookmarks(std::move(records_v));
  } else if (category_name == brave_sync::jslib_const::kHistorySites) {
    OnResolvedHistorySites(category_name, std::move(records_v));
  }

} // endof BraveSyncControllerImpl::OnResolvedSyncRecords

void BraveSyncControllerImpl::OnSyncDebug(const base::ListValue* args) {
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::OnSyncDebug:";
  /**
   * webview -> browser
   * used for debugging in environments where the webview console output is not
   * easily accessible, such as in browser-laptop.
   */
  /*SYNC_DEBUG: _,*/ /* @param {string} message */
  std::string message = args->GetList()[1].GetString();
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::OnSyncDebug: message=<" << message << ">";
  if (sync_ui_ != nullptr) {
    sync_ui_->OnLogMessage(message);
  }
}

void BraveSyncControllerImpl::OnResolvedPreferences(const std::string &category_name,
  std::unique_ptr<base::Value> records_v) {
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::OnResolvedPreferences:";

  brave_sync_event_router_->BrowserToBackgroundPage("can see OnResolvedPreferences");

  SyncDevices existing_sync_devices;
  std::string json = sync_obj_map_->GetObjectIdByLocalId(jslib_const::DEVICES_NAMES);
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::OnResolvedPreferences: existing json=<" << json << ">";
  existing_sync_devices.FromJson(json);

  //SyncDevices sync_devices;

  //then merge to existing

  // std::unique_ptr<base::Value> root = std::make_unique<base::Value>(base::Value::Type::DICTIONARY);
  // std::unique_ptr<base::Value> arr_devices = std::make_unique<base::Value>(base::Value::Type::LIST);

  for (const auto &val : records_v->GetList() ) {
brave_sync::jslib::SyncRecord sr(&val);

    const base::Value *p_name = val.FindPath({"device","name"});
    LOG(ERROR) << "TAGAB OnResolvedPreferences p_name="<<p_name;
    LOG(ERROR) << "TAGAB OnResolvedPreferences p_name->GetString()="<<p_name->GetString();

    const base::Value *p_device_id = val.FindPath({"deviceId","0"});
    LOG(ERROR) << "TAGAB OnResolvedPreferences p_device_id="<<p_device_id;
    LOG(ERROR) << "TAGAB OnResolvedPreferences p_device_id->GetInt()="<<p_device_id->GetInt();

    const base::Value *p_object_data = val.FindKey("objectData");
    LOG(ERROR) << "TAGAB OnResolvedPreferences p_object_data="<<p_object_data;
    LOG(ERROR) << "TAGAB OnResolvedPreferences p_object_data->GetString()="<<p_object_data->GetString();

    //objectId
    // const base::Value *p_object_id = val.FindKey("objectId");
    // LOG(ERROR) << "TAGAB OnResolvedPreferences p_object_data="<<p_object_id;
    // //iterate
    // std::string object_id;
    // for (int i = 0; i < 16; ++i) {
    //     const base::Value *p_byte = p_object_id->FindKey({base::IntToString(i)});
    //     object_id += base::IntToString(p_byte->GetInt());
    //     if (i != 15) {object_id += ", ";}
    // }
    std::string object_id = ExtractObjectIdFromDict(&val);

    const base::Value *p_sync_timestampt = val.FindKey("syncTimestamp");
    LOG(ERROR) << "TAGAB OnResolvedPreferences p_sync_timestampt="<<p_sync_timestampt;
    LOG(ERROR) << "TAGAB OnResolvedPreferences p_sync_timestampt->GetDouble()="<<p_sync_timestampt->GetDouble();
    double d_sync_timestampt = p_sync_timestampt->GetDouble();
    uint64_t i64_sync_timestampt = base::checked_cast<uint64_t>(d_sync_timestampt);
    LOG(ERROR) << "TAGAB OnResolvedPreferences i64_sync_timestampt="<<i64_sync_timestampt;

    base::Time timeJs = base::Time::FromJsTime(d_sync_timestampt);
    LOG(ERROR) << "TAGAB OnResolvedPreferences timeJs="<<timeJs;

    LOG(ERROR) << "TAGAB OnResolvedPreferences object_id="<<object_id;
    // sync_devices.devices_.push_back(SyncDevice(p_name->GetString(),
    //     object_id, base::IntToString(p_device_id->GetInt()), i64_sync_timestampt));

    const base::Value *p_action = val.FindKey("action");
    LOG(ERROR) << "TAGAB OnResolvedPreferences p_action="<<p_action;
    LOG(ERROR) << "TAGAB OnResolvedPreferences p_action->GetInt()="<<p_action->GetInt();
    existing_sync_devices.Merge(SyncDevice(p_name->GetString(),
        object_id, base::IntToString(p_device_id->GetInt()), i64_sync_timestampt), p_action->GetInt());
  } // for each device

  // if (existing_sync_devices.devices_.empty()) {
  //   return;
  // }
  DCHECK(existing_sync_devices.devices_.size() > 0);

  std::string sync_devices_json = existing_sync_devices.ToJson();
  LOG(ERROR) << "TAGAB OnResolvedPreferences sync_devices_json="<<sync_devices_json;

  LOG(ERROR) << "TAGAB OnResolvedPreferences before SaveObjectId";
  sync_obj_map_->SaveObjectId(jslib_const::DEVICES_NAMES, sync_devices_json, "");
  LOG(ERROR) << "TAGAB OnResolvedPreferences SaveObjectId done";

  // Inform devices list chain has been changed
  LOG(ERROR) << "TAGAB OnResolvedPreferences OnSyncStateChanged()";
  if (sync_ui_) {
    sync_ui_->OnSyncStateChanged();
  } else {
    LOG(ERROR) << "TAGAB OnResolvedPreferences sync_ui_ is empty, don't call OnSyncStateChanged()";
  }
  LOG(ERROR) << "TAGAB OnResolvedPreferences OnSyncStateChanged() done";
}

void BraveSyncControllerImpl::OnResolvedBookmarks(std::unique_ptr<base::Value> sync_records_list) {
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::OnResolvedBookmarks: ";

  for (const auto &sync_record_value : sync_records_list->GetList()) {
    LOG(ERROR) << "TAGAB BraveSyncControllerImpl::OnResolvedBookmarks: sync_record_value="<<sync_record_value;
    brave_sync::jslib::SyncRecord sync_record(&sync_record_value);
    DCHECK(sync_record.has_bookmark());

    std::string action = GetAction(sync_record_value);
    LOG(ERROR) << "TAGAB BraveSyncControllerImpl::OnResolvedBookmarks: action=" << action;
    if (action.empty()) {
      continue;
    }

    std::string object_id = ExtractObjectIdFromDict(&sync_record_value);
    LOG(ERROR) << "TAGAB BraveSyncControllerImpl::OnResolvedBookmarks: object_id=" << object_id;
    std::string local_id = sync_obj_map_->GetLocalIdByObjectId(object_id);
    LOG(ERROR) << "TAGAB BraveSyncControllerImpl::OnResolvedBookmarks: local_id=" << local_id;

    DCHECK(sync_record.objectId == object_id);
    DCHECK( base::NumberToString(sync_record.action) == action);

    if (action == jslib_const::CREATE_RECORD && local_id.empty()) {
      std::string location = ExtractBookmarkLocation(&sync_record_value);
      std::string title = ExtractBookmarkTitle(&sync_record_value);
      LOG(ERROR) << "TAGAB BraveSyncControllerImpl::OnResolvedBookmarks: location=" << location;
      LOG(ERROR) << "TAGAB BraveSyncControllerImpl::OnResolvedBookmarks: title=" << title;
      LOG(ERROR) << "TAGAB BraveSyncControllerImpl::OnResolvedBookmarks: site.location=" << sync_record.GetBookmark().site.location;
      LOG(ERROR) << "TAGAB BraveSyncControllerImpl::OnResolvedBookmarks: site.title=" << sync_record.GetBookmark().site.title;
      DCHECK(location == sync_record.GetBookmark().site.location);
      DCHECK(title == sync_record.GetBookmark().site.title);
      bookmarks_->AddBookmark(sync_record);
    }
  }
}

void BraveSyncControllerImpl::OnResolvedHistorySites(const std::string &category_name,
  std::unique_ptr<base::Value> records_v) {
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::OnResolvedHistorySites:";
}

std::unique_ptr<base::Value> BraveSyncControllerImpl::PrepareResolvedResponse(
  const std::string &category_name,
  const std::unique_ptr<base::Value> &sync_records_list) {
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::PrepareResolvedResponse: category_name="<<category_name;

  std::unique_ptr<base::Value> resolvedResponse(new base::Value(base::Value::Type::LIST));

  for (const base::Value &val : sync_records_list->GetList() ) {
    LOG(ERROR) << "TAGAB val.type()="<< base::Value::GetTypeName(val.type());
    DCHECK(val.is_dict());
brave_sync::jslib::SyncRecord sr(&val);
    base::Value server_record(val.Clone());

    //base::Value local_record(base::Value::Type::NONE);
    std::unique_ptr<base::Value> p_local_record = std::make_unique<base::Value>(base::Value::Type::NONE);
    // local_record should be get by server_record->objectId => <local object id> => <local object>
    // if we cannot get local object id, then specify local record is <empty>
    std::string object_id = ExtractObjectIdFromDict(&val);
    LOG(ERROR) << "TAGAB BraveSyncControllerImpl::PrepareResolvedResponse object_id=" << object_id;

    if (category_name == jslib_const::kBookmarks) {
      //"BOOKMARKS"
      p_local_record = bookmarks_->GetResolvedBookmarkValue(object_id);
    } else if (category_name == jslib_const::kHistorySites) {
      //"HISTORY_SITES";
      p_local_record = std::make_unique<base::Value>(base::Value::Type::NONE);
      NOTIMPLEMENTED();
    } else if (category_name == jslib_const::kPreferences) {
      //"PREFERENCES"
      LOG(ERROR) << "TAGAB BraveSyncControllerImpl::PrepareResolvedResponse: resolving device";
      p_local_record = PrepareResolvedDevice(object_id);
      LOG(ERROR) << "TAGAB BraveSyncControllerImpl::PrepareResolvedResponse *p_local_record=" << std::endl << brave::debug::ToPrintableString(*p_local_record);
      LOG(ERROR) << "TAGAB BraveSyncControllerImpl::PrepareResolvedResponse: -----------------";
    }

// if (p_local_record->is_none()) {
//   LOG(ERROR) << "TAGAB BraveSyncControllerImpl::PrepareResolvedResponse" <<std::endl
//     <<"unexpected empty resolve for object_id="<<object_id;
//   NOTREACHED();
// }

    std::unique_ptr<base::Value> resolvedResponseRow(new base::Value(base::Value::Type::LIST));
    resolvedResponseRow->GetList().push_back(std::move(server_record));
    resolvedResponseRow->GetList().push_back(std::move(*p_local_record));
    resolvedResponse->GetList().push_back(std::move(*resolvedResponseRow));
  }

  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::PrepareResolvedResponse *resolvedResponse" << std::endl << brave::debug::ToPrintableString(*resolvedResponse);
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::PrepareResolvedResponse -----------------------------";
  return resolvedResponse;
}

std::unique_ptr<base::Value> BraveSyncControllerImpl::PrepareResolvedDevice(const std::string &object_id) {
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::PrepareResolvedResponse object_id=" << object_id;
  return std::make_unique<base::Value>(base::Value::Type::NONE);
  // std::string json = sync_obj_map_->GetObjectIdByLocalId(jslib_const::DEVICES_NAMES);
  // SyncDevices devices;
  // devices.FromJson(json);
  //
  // SyncDevice* device = devices.GetByObjectId(object_id);
  // LOG(ERROR) << "TAGAB BraveSyncControllerImpl::PrepareResolvedResponse device=" << device;
  // if (device) {
  //   LOG(ERROR) << "TAGAB BraveSyncControllerImpl::PrepareResolvedResponse will ret value";
  //   return device->ToValue();
  // } else {
  //   LOG(ERROR) << "TAGAB BraveSyncControllerImpl::PrepareResolvedResponse will ret none";
  //   return std::make_unique<base::Value>(base::Value::Type::NONE);
  // }
}

void BraveSyncControllerImpl::SendResolveSyncRecords(
  const std::string &category_name,
  const base::Value* response) {
    CHECK(response);
    base::Value command("resolve-sync-records");
    CallJsLibBV(command, base::Value(), base::Value(category_name), *response, base::Value());
}


//fetch-sync-records
/**
* webview -> browser
* sent when sync has finished initialization
*/
//can fetch records now
/**
 * browser -> webview
 * sent to fetch sync records after a given start time from the sync server.
 * we perform an S3 ListObjectsV2 request per category. for each category
 * with new records, do
 * GET_EXISTING_OBJECTS -> RESOLVE_SYNC_RECORDS -> RESOLVED_SYNC_RECORDS
 */
void BraveSyncControllerImpl::FetchSyncRecords(const bool &bookmarks,
  const bool &history, const bool &preferences, int64_t start_at, int max_records) {
    LOG(ERROR) << "TAGAB BraveSyncControllerImpl::FetchSyncRecords:";
    DCHECK(bookmarks || history || preferences);
    if (!(bookmarks || history || preferences)) {
      return;
    }

    std::string categoryNames = "[";
    using namespace brave_sync::jslib_const;
    if (history) {
      categoryNames += "\"";
      categoryNames += kHistorySites;//"HISTORY_SITES";
      categoryNames += "\",";
    }
    if (bookmarks) {
      categoryNames += "\"";
      categoryNames += kBookmarks;//"BOOKMARKS";
      categoryNames += "\",";
    }
    if (preferences) {
      categoryNames += "\"";
      categoryNames += kPreferences;//"PREFERENCES";
      categoryNames += "\",";
    }
    if (categoryNames.length() > 1) {
      categoryNames.resize(categoryNames.length() - 1);
    }
    categoryNames += "]";

    // I cannot use int64 -> Base::Value, I should use strings

    std::stringstream ss_start_at;
    ss_start_at << start_at;
    std::stringstream ss_max_records;
    ss_max_records << max_records;
    CallJsLibStr("fetch-sync-records", "", categoryNames, ss_start_at.str() , ss_max_records.str());

    base::Time this_fetch_time = base::Time::Now();
    sync_prefs_->SetLastFetchTime(this_fetch_time);
}

void BraveSyncControllerImpl::SendCreateDevice() {
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::SendCreateDevice";
  //SetUpdateDeleteDeviceName(CREATE_RECORD, mDeviceName, mDeviceId, "");

  std::string deviceName = sync_prefs_->GetThisDeviceName();
  std::string objectId = brave_sync::tools::GenerateObjectId();
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::SendCreateDevice deviceName=" << deviceName;
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::SendCreateDevice objectId="<<objectId;
  std::string deviceId = sync_prefs_->GetThisDeviceId();
  CHECK(!deviceId.empty());

  std::stringstream request;
  request << "[";
  //CreateDeviceCreationRecord(deviceName, objectId, action, deviceId);
  std::string action = jslib_const::CREATE_RECORD;
  std::string stmp = CreateDeviceCreationRecord(deviceName, objectId, action, deviceId);
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::SendCreateDevice objectId="<<stmp;
  request << stmp;
  request << "]";

  std::vector<std::string> ids;
  //SendSyncRecords(SyncRecordType.PREFERENCES, request, action, ids);
  SendSyncRecords(jslib_const::SyncRecordType_PREFERENCES, request.str(), action, ids);
}

void BraveSyncControllerImpl::SendSyncRecords(const std::string &recordType,
  const std::string &recordsJSON,
  const std::string &action,
  const std::vector<std::string> &ids) {

  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::SendSyncRecords";
  LOG(ERROR) << "TAGAB recordType=" << recordType;
  LOG(ERROR) << "TAGAB recordsJSON=" << recordsJSON;
  LOG(ERROR) << "TAGAB action=" << action;

  if (!sync_initialized_) {
    LOG(ERROR) << "TAGAB BraveSyncControllerImpl::SendSyncRecords sync is not initialized";
    DCHECK(false);
    return;
  }

  SaveGetDeleteNotSyncedRecords(recordType, action, ids, NotSyncedRecordsOperation::AddItems);
  CallJsLibStr("send-sync-records", std::string(), recordType, recordsJSON, std::string());
}

std::vector<std::string> BraveSyncControllerImpl::SaveGetDeleteNotSyncedRecords(
  const std::string &recordType, const std::string &action,
  const std::vector<std::string> &ids,
  NotSyncedRecordsOperation operation) {
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::SaveGetDeleteNotSyncedRecords, early quit";
  return std::vector<std::string>();
  // java SaveGetDeleteNotSyncedRecords
}

void BraveSyncControllerImpl::SendAllLocalBookmarks() {
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::SendAllLocalBookmarks";
  static const int SEND_RECORDS_COUNT_LIMIT = 1000;
  std::vector<const bookmarks::BookmarkNode*> localBookmarks;
  bookmarks_->GetAllBookmarks(localBookmarks);

  for(size_t i = 0; i < localBookmarks.size(); i += SEND_RECORDS_COUNT_LIMIT) {
    size_t sub_list_last = std::min(localBookmarks.size(), i + SEND_RECORDS_COUNT_LIMIT);
    std::vector<const bookmarks::BookmarkNode*> sub_list(localBookmarks.begin()+i, localBookmarks.begin()+sub_list_last);
    CreateUpdateDeleteBookmarks(jslib_const::kActionCreate, sub_list, true, true);
  }
}

void BraveSyncControllerImpl::CreateUpdateDeleteBookmarks(
  const int &action,
  const std::vector<const bookmarks::BookmarkNode*> &list,
  const bool &addIdsToNotSynced,
  const bool &isInitialSync) {
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::CreateUpdateDeleteBookmarks";

  DCHECK(sync_initialized_);
  if (list.empty() || !sync_initialized_ || !sync_prefs_->GetSyncBookmarksEnabled() ) {
    return;
  }

  // Should finally call 'send-sync-records'
  /**
   * browser -> webview
   * browser sends this to the webview with the data that needs to be synced
   * to the sync server.
   */
  //SEND_SYNC_RECORDS: _, /* @param {string=} categoryName, @param {Array.<Object>} records */

  std::unique_ptr<base::Value> lv_bookmarks = bookmarks_->NativeBookmarksToSyncLV(list, action);

  CallJsLibBV(base::Value("send-sync-records"),
    base::Value(),
    base::Value(jslib_const::SyncRecordType_BOOKMARKS) /*"BOOKMARKS"*/,
    std::move(*lv_bookmarks),
    base::Value()
  );
}

void BraveSyncControllerImpl::SendAllLocalHistorySites() {
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::SendAllLocalHistorySites";

}

std::string BraveSyncControllerImpl::CreateDeviceCreationRecord(
  const std::string &deviceName,
  const std::string &objectId,
  const std::string &action,
  const std::string &deviceId) {
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::CreateDeviceCreationRecord";
  DCHECK(!deviceName.empty());
  if (deviceName.empty()) {
    return std::string();
  }

  std::stringstream ss;
  ss << "{ \"action\": " << action << ", ";
  ss << "\"deviceId\": [" << deviceId << "], ";
  ss << "\"objectId\": [" << objectId << "], ";
  ss << "\"" << jslib_const::SyncObjectData_DEVICE << "\"" << ": { \"name\": \"" << brave_sync::tools::replaceUnsupportedCharacters(deviceName) << "\"}}";

  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::CreateDeviceCreationRecord ss.str()=" << ss.str();

  return ss.str();
}

void BraveSyncControllerImpl::SetUpdateDeleteDeviceName(
  const std::string &action,
  const std::string &deviceName,
  const std::string &deviceId,
  const std::string &objectId) {
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::SetUpdateDeleteDeviceName";
  LOG(ERROR) << "TAGAB action=" << action;
  LOG(ERROR) << "TAGAB deviceName=" << deviceName;
  LOG(ERROR) << "TAGAB deviceId=" << deviceId;
  LOG(ERROR) << "TAGAB objectId=" << objectId;

  std::string objectIdCopy;
  if (action == jslib_const::CREATE_RECORD) {
    objectIdCopy = this->GenerateObjectIdWithMapCheck("deviceName");
  } else {
    objectIdCopy = objectId;
  }

  std::stringstream request;
  request << "[";
  DCHECK(!objectIdCopy.empty());

  std::string stmp = CreateDeviceCreationRecord(deviceName, objectIdCopy, action, deviceId);
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::SetUpdateDeleteDeviceName stmp="<<stmp;
  request << stmp;
  request << "]";

  std::vector<std::string> ids;
  SendSyncRecords(jslib_const::SyncRecordType_PREFERENCES, request.str(), action, ids);
}

std::string BraveSyncControllerImpl::GenerateObjectIdWithMapCheck(const std::string &local_id) {
  std::string res = sync_obj_map_->GetObjectIdByLocalId(local_id);
  if (!res.empty()) {
    return res;
  }

  return brave_sync::tools::GenerateObjectId();
}

static const int64_t kCheckUpdatesIntervalSec = 30;

void BraveSyncControllerImpl::StartLoop() {
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::StartLoop " << GetThreadInfoString();
  // Repeated task runner
  //https://chromium.googlesource.com/chromium/src/+/lkgr/docs/threading_and_tasks.md#posting-a-repeating-task-with-a-delay
  timer_->Start(FROM_HERE, base::TimeDelta::FromSeconds(kCheckUpdatesIntervalSec),
                 this, &BraveSyncControllerImpl::LoopProc);
  //in UI THREAD
}

void BraveSyncControllerImpl::StopLoop() {
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::StopLoop " << GetThreadInfoString();
  timer_->Stop();
  //in UI THREAD
}

void BraveSyncControllerImpl::LoopProc() {
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::LoopProc " << GetThreadInfoString();

  // task_runner_->PostTask(FROM_HERE,
  // base::Bind(&BraveSyncControllerImpl::LoopProcThreadAligned,
  //     base::Unretained(this)));
  //in UI THREAD

  LoopProcThreadAligned();
  // For now cannot run LoopProcThreadAligned in a task runner because it uses
  // sync_prefs_ which should be accessed in UI thread
}

void BraveSyncControllerImpl::LoopProcThreadAligned() {
  LOG(ERROR) << "TAGAB BraveSyncControllerImpl::LoopProcThreadAligned " << GetThreadInfoString();

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



} // namespace brave_sync
