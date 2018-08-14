/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/controller_impl.h"

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
#include "brave/components/brave_sync/bookmarks.h"
#include "brave/components/brave_sync/devices.h"
#include "brave/components/brave_sync/profile_prefs.h"
#include "brave/components/brave_sync/settings.h"
#include "brave/components/brave_sync/jslib_const.h"
#include "brave/components/brave_sync/jslib_messages.h"
#include "brave/components/brave_sync/object_map.h"
#include "brave/components/brave_sync/tools.h"
#include "brave/components/brave_sync/client/client_web_ui_impl.h"
#include "brave/components/brave_sync/debug.h"
#include "brave/components/brave_sync/values_conv.h"
#include "brave/components/brave_sync/value_debug.h"

#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser.h"
#include "content/public/browser/browser_thread.h"

namespace brave_sync {

ControllerImpl::TempStorage::TempStorage() {
}

ControllerImpl::TempStorage::~TempStorage() {
}

ControllerImpl::ControllerImpl() :
  sync_ui_(nullptr),
  //sync_js_layer_(nullptr),
  sync_client_(nullptr),
  sync_initialized_(false),
  timer_(std::make_unique<base::RepeatingTimer>()) {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::ControllerImpl CTOR";

  DETACH_FROM_SEQUENCE(sequence_checker_);

  sync_prefs_.reset(new brave_sync::prefs::Prefs(nullptr)); //this is wrong. TODO, AB: pass the pointer

  std::unique_ptr<brave_sync::Settings> settings_ = std::make_unique<brave_sync::Settings>();

  std::unique_ptr<brave_sync::Settings> settingsTest = sync_prefs_->GetBraveSyncSettings();
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::ControllerImpl settingsTest";
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::ControllerImpl settingsTest->this_device_name_=" << settingsTest->this_device_name_;
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::ControllerImpl settingsTest->sync_this_device_=" << settingsTest->sync_this_device_;

  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::ControllerImpl sync_prefs_->GetSeed()=<" << sync_prefs_->GetSeed() <<">";
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::ControllerImpl sync_prefs_->GetThisDeviceName()=<" << sync_prefs_->GetThisDeviceName() <<">";

  // volatile bool b_DBG_clear_on_start = true;
  // if (b_DBG_clear_on_start) {
  //   LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::ControllerImpl clearing prefs";
  //   sync_prefs_->Clear();
  // }

  // volatile bool b_DBG_setname_on_start = true;
  // if (b_DBG_setname_on_start) {
  //   LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::ControllerImpl set name to cf56";
  //   sync_prefs_->SetDeviceName("cf56");
  // LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::ControllerImpl sync_prefs_->GetThisDeviceName()=" << sync_prefs_->GetThisDeviceName();
  //   DCHECK(false);
  // }

  task_runner_ = base::CreateSequencedTaskRunnerWithTraits(
      {base::MayBlock(), base::TaskPriority::BACKGROUND,
       base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN} );

  sync_obj_map_ = std::make_unique<storage::ObjectMap>();

  bookmarks_ = std::make_unique<brave_sync::Bookmarks>(this);

  if (!sync_prefs_->GetThisDeviceId().empty()) {
    bookmarks_->SetThisDeviceId(sync_prefs_->GetThisDeviceId());
  }
  bookmarks_->SetObjectMap(sync_obj_map_.get());

  BrowserList::GetInstance()->AddObserver(this);

  if (!sync_prefs_->GetSeed().empty() && !sync_prefs_->GetThisDeviceName().empty()) {
    LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::ControllerImpl sync is configured";

    task_runner_->PostTask(FROM_HERE,
    base::Bind(&ControllerImpl::InitJsLib,
        base::Unretained(this), false));

  } else {
    LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::ControllerImpl sync is NOT configured";
  }

//  sync_client_ = new BraveSyncClientWebUiImpl(); for web ui impl assign it in InitJsLib

  StartLoop();
}

ControllerImpl::~ControllerImpl() {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::~ControllerImpl DTOR";
  BrowserList::GetInstance()->RemoveObserver(this);

  StopLoop();

  if (sync_client_) {
    delete sync_client_;
    sync_client_ = nullptr;
  }
}

ControllerImpl* ControllerImpl::GetInstance() {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::GetInstance";
  //LOG(ERROR) << base::debug::StackTrace().ToString();

  return base::Singleton<ControllerImpl>::get();
}

void ControllerImpl::OnSetupSyncHaveCode(const std::string &sync_words,
  const std::string &device_name) {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnSetupSyncHaveCode";
  LOG(ERROR) << "TAGAB sync_words=" << sync_words;
  LOG(ERROR) << "TAGAB device_name=" << device_name;

  temp_storage_.device_name_ = device_name; // Fill here, but save in OnSaveInitData

  std::string arg1;
  arg1= "\"" + sync_words + "\"";
  CallJsLibStr("words_to_bytes", arg1, "", "", "");
}

void ControllerImpl::OnSetupSyncNewToSync(const std::string &device_name) {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnSetupSyncNewToSync";
  LOG(ERROR) << "TAGAB device_name="<<device_name;

  temp_storage_.device_name_ = device_name; // Fill here, but save in OnSaveInitData

  InitJsLib(true); // Init will cause load of the Script
  // Then we will got GOT_INIT_DATA and SAVE_INIT_DATA, where we will save the seed and device id
  // Then when we will receive sync_ready, we should display web page with sync settings
}

void ControllerImpl::OnDeleteDevice(const std::string &device_id) {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnDeleteDevice";
  LOG(ERROR) << "TAGAB device_id="<<device_id;

  //CHECK(sync_js_layer_ != nullptr);
  CHECK(sync_client_ != nullptr);
  CHECK(sync_initialized_);

  std::string json = sync_obj_map_->GetObjectIdByLocalId(jslib_const::DEVICES_NAMES);
  SyncDevices syncDevices;
  syncDevices.FromJson(json);
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnDeleteDevice json="<<json;

  const SyncDevice *device = syncDevices.GetByDeviceId(device_id);
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnDeleteDevice device="<<device;
  //DCHECK(device); // once I saw it nullptr
  if (device) {
    const std::string device_name = device->name_;
    const std::string object_id = device->object_id_;
    LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnDeleteDevice device_name="<<device_name;
    LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnDeleteDevice object_id="<<object_id;

    SetUpdateDeleteDeviceName(
      jslib_const::DELETE_RECORD,
      device_name,
      device_id,
      object_id);
  }
}

void ControllerImpl::OnResetSync() {
  LOG(ERROR) << "TAGAB  ControllerImpl::OnResetSync";
  //CHECK(sync_js_layer_ != nullptr);
  CHECK(sync_client_ != nullptr);
  CHECK(sync_initialized_);

  const std::string device_id = sync_prefs_->GetThisDeviceId();
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnResetSync device_id="<<device_id;
  OnDeleteDevice(device_id);

  sync_prefs_->Clear();

  sync_obj_map_->DestroyDB();

  if (sync_ui_) {
    sync_ui_->OnSyncStateChanged();
  }

  // Close js lib pseudo-tab
}

void ControllerImpl::GetSettings(brave_sync::Settings &settings) {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::GetSettings";

  std::unique_ptr<brave_sync::Settings> bss = sync_prefs_->GetBraveSyncSettings();
  settings = *std::move(bss);

  LOG(ERROR) << "TAGAB settings.this_device_name_=<" << settings.this_device_name_ << ">";
  LOG(ERROR) << "TAGAB settings.sync_this_device_=<" << settings.sync_this_device_ << ">";
  LOG(ERROR) << "TAGAB sync_prefs_->GetSeed()=<" << sync_prefs_->GetSeed() << ">";
  LOG(ERROR) << "TAGAB sync_prefs_->GetThisDeviceName()=<" << sync_prefs_->GetThisDeviceName() << ">";

  settings.sync_configured_ = !sync_prefs_->GetSeed().empty() &&
    !sync_prefs_->GetThisDeviceName().empty();

  LOG(ERROR) << "TAGAB settings.sync_configured_=<" << settings.sync_configured_ << ">";
}

void ControllerImpl::GetDevices(SyncDevices &devices) {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::GetDevices";

//   // fetch devices
//   FetchSyncRecords(false, false, true, 0, 300);
// LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::GetDevices, request FetchSyncRecords is done";
// ^ causes recursion

  std::string json = sync_obj_map_->GetObjectIdByLocalId(jslib_const::DEVICES_NAMES);
  SyncDevices syncDevices;
  syncDevices.FromJson(json);
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::GetDevices json="<<json;
  devices = syncDevices;
}

void ControllerImpl::GetSyncWords() {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::GetSyncWords";

  // Ask JS library
  std::string seed = sync_prefs_->GetSeed();
  std::string arg1= "\"" + seed + "\"";
  CallJsLibStr("bytes_to_words", arg1, "", "", "");
}

std::string ControllerImpl::GetSeed() {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::GetSeed";
  std::string seed = sync_prefs_->GetSeed();
  return seed;
}

void ControllerImpl::LoadJsLibPseudoTab() {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::LoadJsLibPseudoTab";

  // TODO, AB: this is not good.
  // Possible situation:
  // 1) open browser A
  // 2) create tab with js lib in tab A
  // 3) create browser B
  // 4) close browser B
  // either to move js lib into V8 or subscribe on BrowserListObserver events
  // so during BrowserListObserver::OnBrowserRemoved do re-init of sync lib
  Browser* browser = BrowserList::GetInstance()->GetLastActive();

  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::LoadJsLibPseudoTab browser=" << browser;

  if (browser) {
    brave::LoadBraveSyncJsLib(browser);
  } else {
    // Well, wait for the browser to be loaded, do work in ControllerImpl::OnBrowserAdded
    LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::LoadJsLibPseudoTab browser=(NIL)!";
  }
}

void ControllerImpl::OnBrowserAdded(Browser* browser) {
  LOG(ERROR) << "TAGAB  ControllerImpl::OnBrowserAdded browser="<<browser;
}

void ControllerImpl::OnBrowserSetLastActive(Browser* browser) {
  LOG(ERROR) << "TAGAB  ControllerImpl::OnBrowserSetLastActive browser="<<browser;
  browser_ = browser;
  bookmarks_->SetBrowser(browser);

  //TODO, AB: need several profiles, ControllerImpl per profile
  if (!brave_sync_event_router_) {
    brave_sync_event_router_ = std::make_unique<extensions::BraveSyncEventRouter>(browser_->profile());
  }

  // LOG(ERROR) << "TAGAB  ControllerImpl::OnBrowserSetLastActive sync_js_layer_="<<sync_js_layer_;
  // if (sync_js_layer_) {
  //   return;
  // }
  LOG(ERROR) << "TAGAB  ControllerImpl::OnBrowserSetLastActive sync_client_="<<sync_client_;
  if (sync_client_) {
    return;
  }

  LOG(ERROR) << "TAGAB  ControllerImpl::OnBrowserSetLastActive sync_client_ null, post in UI ControllerImpl::InitJsLib";
  content::BrowserThread::GetTaskRunnerForThread(content::BrowserThread::UI)->PostTask(
    FROM_HERE, base::Bind(&ControllerImpl::InitJsLib,
         base::Unretained(this), false));
}

void ControllerImpl::InitJsLib(const bool &setup_new_sync) {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::InitJsLib " << GetThreadInfoString();

  // if (!sync_js_layer_) {
  //   LoadJsLibPseudoTab();
  //   return;
  // }

  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::InitJsLib sync_client_=" << sync_client_;
  if (!sync_client_) {
    LoadJsLibPseudoTab();
    return;
  }


  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::InitJsLib (2) " << GetThreadInfoString();
  if ( (!sync_prefs_->GetSeed().empty() && !sync_prefs_->GetThisDeviceName().empty()) ||
      setup_new_sync) {
    LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::InitJsLib sync is active or setup_new_sync";
    //sync_js_layer_->LoadJsLibScript();//
    sync_client_->LoadClient();
  } else {
    LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::InitJsLib sync is NOT active";
  }

}

void ControllerImpl::CallJsLibBV(const base::Value &command,
  const base::Value &arg1, const base::Value &arg2, const base::Value &arg3,
  const base::Value &arg4) {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::CallJsLibBV";
  // DCHECK(nullptr != sync_js_layer_);
  // if (!sync_js_layer_) {
  //   return;
  // }
  DCHECK(nullptr != sync_client_);
  if (!sync_client_) {
    return;
  }

  const std::vector<const base::Value*> args = {&command, &arg1, &arg2, &arg3, &arg4};
  //sync_js_layer_->RunCommandBV(args);//
  sync_client_->RunCommandBV(args);
}

void ControllerImpl::CallJsLibStr(const std::string &command,
  const std::string &arg1, const std::string &arg2, const std::string &arg3,
  const std::string &arg4) {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::CallJsLibStr";
  // DCHECK(nullptr != sync_js_layer_);
  // if (!sync_js_layer_) {
  //   return;
  // }
  //
  // sync_js_layer_->RunCommandStr(command, arg1, arg2, arg3, arg4);

  DCHECK(nullptr != sync_client_);
  if (!sync_client_) {
    return;
  }

  sync_client_->RunCommandStr(command, arg1, arg2, arg3, arg4);
}

// Temporary while moving to extension
void ControllerImpl::SetupJsLayer(SyncJsLayer *sync_js_layer) {
  // LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::SetupJsLayer sync_js_layer=" << sync_js_layer;
  // LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::SetupJsLayer this->sync_js_layer_=" << this->sync_js_layer_;
  // DCHECK(sync_js_layer);
  // DCHECK(sync_js_layer_ == nullptr);
  //
  // sync_js_layer_ = sync_js_layer;


  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::SetupJsLayer sync_js_layer=" << sync_js_layer;
  DCHECK(sync_js_layer);
  DCHECK(!sync_client_);
  sync_client_ = new BraveSyncClientWebUiImpl();
  sync_client_->SetupJsLayer(sync_js_layer);
}

void ControllerImpl::SetupUi(SyncUI *sync_ui) {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::SetupUi sync_ui=" << sync_ui;

  DCHECK(sync_ui);
  DCHECK(sync_ui_ == nullptr);

  sync_ui_ = sync_ui;
}

void ControllerImpl::OnMessageFromSyncReceived() {
  ;
}
// SyncLibToBrowserHandler overrides
void ControllerImpl::OnSyncDebug(const std::string &message) {
  ;
}

void ControllerImpl::OnSyncSetupError(const std::string &error) {
  ;
}

void ControllerImpl::OnGetInitData(const std::string &sync_version) {
  ;
}

void ControllerImpl::OnSaveInitData(const Uint8Array &seed, const Uint8Array &device_id) {
  ;
}

void ControllerImpl::OnSyncReady() {
  ;
}

void ControllerImpl::OnGetExistingObjects(const std::string &category_name,
  const RecordsList &records, const base::Time &last_record_time_stamp) {
  ;
}

void ControllerImpl::OnResolvedSyncRecords(const std::string &category_name,
  const RecordsList &records) {
  ;
}

void ControllerImpl::OnDeletedSyncUser() {
  ;
}

void ControllerImpl::OnDeleteSyncSiteSettings()  {
  ;
}

void ControllerImpl::OnSaveBookmarksBaseOrder(const std::string &order)  {
  ;
}

void ControllerImpl::OnSaveBookmarkOrder(const std::string &order,
  const std::string &prev_order, const std::string &next_order)  {
  ;
}

void ControllerImpl::OnJsLibMessage(const std::string &message, const base::ListValue* args) {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnJsLibMessage, message=" << message;
  if (message == "words_to_bytes_done") {
    OnWordsToBytesDone_(args);
  } else if (message == "bytes_to_words_done") {
    OnBytesToWordsDone_(args);
  }
  else if (message == "get-init-data") {
    ;
  } else if (message == "got-init-data") {
    OnGotInitData_(args);
  } else if (message == "save-init-data") {
    OnSaveInitData_(args);
  } else if (message == "sync-ready") {
    OnSyncReady_(args);
  } else if (message == "get-existing-objects") {
    OnGetExistingObjects_(args);
  } else if (message == "resolved-sync-records") {
    OnResolvedSyncRecords_(args);
  } else if (message == "sync-debug") {
    OnSyncDebug_(args);
  }
}

void ControllerImpl::OnGotInitData_(const base::ListValue* args) {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnGotInitData";

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
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnGotInitData: lv_seed=" << brave::debug::ToPrintableString(*lv_seed);
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnGotInitData: lv_deviceId=" << brave::debug::ToPrintableString(*lv_deviceId);

  CallJsLibBV(command, base::Value(), *lv_seed, *lv_deviceId, config);
}

void ControllerImpl::OnWordsToBytesDone_(const base::ListValue* args) {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnWordsToBytesDone";
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

  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnWordsToBytesDone: call InitJsLib";
  InitJsLib(true);//Init will cause load of the Script;
}

void ControllerImpl::OnBytesToWordsDone_(const base::ListValue* args) {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnWordsToBytesDone";
  LOG(ERROR) << "TAGAB args->GetList().size()=" << args->GetList().size();
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnWordsToBytesDone" <<
    brave::debug::ToPrintableString(*args);

  CHECK(sync_ui_);

  DCHECK(args->GetList()[0].GetString() == "bytes_to_words_done");
  DCHECK(args->GetList()[1].is_string());

  std::string words = args->GetList()[1].GetString();

  sync_ui_->OnHaveSyncWords(words);
}

void ControllerImpl::OnSyncReady_(const base::ListValue* args) {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnSyncReady:";
  DCHECK(false == sync_initialized_);
  sync_initialized_ = true;


  if (sync_ui_) {
    LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnSyncReady: have sync ui, inform state changed";
    // inform WebUI page that data is ready
    // changed this device name and id
    sync_ui_->OnSyncStateChanged();
  } else {
    // it can be ui page is not opened yet
    LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnSyncReady: sync_ui_ is null";
  }

  // fetch the records
  RequestSyncData();
}

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

  //const int64_t start_at = sync_prefs_->GetTimeLastFetch();
  base::Time last_record_time = sync_prefs_->GetLatestRecordTime();
  const int64_t start_at = base::checked_cast<int64_t>(last_record_time.ToJsTime());
  const int max_records = 300;
  base::Time last_fetch_time = sync_prefs_->GetLastFetchTime();

  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::RequestSyncData: start_at="<<start_at;
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::RequestSyncData: last_fetch_time="<<last_fetch_time;
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

void ControllerImpl::OnSaveInitData_(const base::ListValue* args) {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnSaveInitData:";
  DCHECK(false == sync_initialized_);

  LOG(ERROR) << "TAGAB: *args=" << brave::debug::ToPrintableString(*args);

  DCHECK(args->GetList()[1].is_string());
  DCHECK(args->GetList()[2].is_string());

  std::string seed = args->GetList()[1].GetString();
  std::string device_id = args->GetList()[2].GetString();

  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnSaveInitData: seed=<"<<seed<<">";
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnSaveInitData: device_id="<<device_id<<">";

  if (temp_storage_.seed_str_.empty() && !seed.empty()) {
    temp_storage_.seed_str_ = seed;
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
  sync_prefs_->SetThisDeviceId(device_id);
  bookmarks_->SetThisDeviceId(device_id);
  // If we have already initialized sync earlier we don't receive seed again
  // and do not save it
  if (!temp_storage_.seed_str_.empty()) {
    sync_prefs_->SetSeed(temp_storage_.seed_str_);
  }
  sync_prefs_->SetDeviceName(temp_storage_.device_name_);//here I can have empty string, why ?
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnSaveInitData: saved device_id="<<device_id;
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnSaveInitData: saved seed="<<temp_storage_.seed_str_;
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnSaveInitData: saved temp_storage_.device_name_="<<temp_storage_.device_name_;

  sync_prefs_->SetSyncThisDevice(true);

  sync_prefs_->SetSyncBookmarksEnabled(true);
  sync_prefs_->SetSyncSiteSettingsEnabled(true);
  sync_prefs_->SetSyncHistoryEnabled(true);
}

void ControllerImpl::OnGetExistingObjects_(const base::ListValue* args) {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnGetExistingObjects:";
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

  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnGetExistingObjects: category_name="<<category_name;
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnGetExistingObjects: last_record_timestamp="<<last_record_timestamp;
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnGetExistingObjects: is_truncated="<<is_truncated;

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

  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnGetExistingObjects: records_v.get()="<<records_v.get();
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnGetExistingObjects: error_msg_out="<<error_msg_out;
  DCHECK(records_v);
  if (!records_v) {
    return;
  }

  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnGetExistingObjects records_v->type()="<< base::Value::GetTypeName(records_v->type());
  DCHECK(records_v->is_list());

  //LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnGetExistingObjects before ToPrintableString";
  //LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnGetExistingObjects" << std::endl << ToPrintableString(*records_v);

  //should:
  // resolve
  // then send data with RESOLVE_SYNC_RECORDS
  // then receive RESOLVED_SYNC_RECORDS

  std::unique_ptr<base::Value> resolvedResponse;

  resolvedResponse = PrepareResolvedResponse(category_name, records_v);
  SendResolveSyncRecords(category_name, resolvedResponse.get());
}

void ControllerImpl::OnResolvedSyncRecords_(const base::ListValue* args) {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnResolvedSyncRecords:";
  /**
   * webview -> browser
   * browser must update its local values with the resolved sync records.
   */
   //RESOLVED_SYNC_RECORDS: _, /* @param {string} categoryName, @param {Array.<Object>} records */;

   std::string category_name = args->GetList()[1].GetString();
   std::string records_json = args->GetList()[2].GetString();

   LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnResolvedSyncRecords: category_name="<<category_name;
   LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnResolvedSyncRecords: records_json="<<records_json;

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

   LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnResolvedSyncRecords: records_v.get()="<<records_v.get();
   LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnResolvedSyncRecords: error_msg_out="<<error_msg_out;
   DCHECK(records_v);
   if (!records_v) {
     return;
   }

   LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnResolvedSyncRecords: ToPrintableString=" << std::endl << brave::debug::ToPrintableString(*records_v);

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

} // endof ControllerImpl::OnResolvedSyncRecords

void ControllerImpl::OnSyncDebug_(const base::ListValue* args) {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnSyncDebug:";
  /**
   * webview -> browser
   * used for debugging in environments where the webview console output is not
   * easily accessible, such as in browser-laptop.
   */
  /*SYNC_DEBUG: _,*/ /* @param {string} message */
  std::string message = args->GetList()[1].GetString();
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnSyncDebug: message=<" << message << ">";
  if (sync_ui_ != nullptr) {
    sync_ui_->OnLogMessage(message);
  }
}

void ControllerImpl::OnResolvedPreferences(const std::string &category_name,
  std::unique_ptr<base::Value> records_v) {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnResolvedPreferences:";

  brave_sync_event_router_->BrowserToBackgroundPage("can see OnResolvedPreferences");

  SyncDevices existing_sync_devices;
  std::string json = sync_obj_map_->GetObjectIdByLocalId(jslib_const::DEVICES_NAMES);
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnResolvedPreferences: existing json=<" << json << ">";
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

void ControllerImpl::OnResolvedBookmarks(std::unique_ptr<base::Value> sync_records_list) {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnResolvedBookmarks: ";

  for (const auto &sync_record_value : sync_records_list->GetList()) {
    LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnResolvedBookmarks: sync_record_value="<<sync_record_value;
    brave_sync::jslib::SyncRecord sync_record(&sync_record_value);
    DCHECK(sync_record.has_bookmark());

    std::string action = GetAction(sync_record_value);
    LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnResolvedBookmarks: action=" << action;
    if (action.empty()) {
      continue;
    }

    std::string object_id = ExtractObjectIdFromDict(&sync_record_value);
    LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnResolvedBookmarks: object_id=" << object_id;
    std::string local_id = sync_obj_map_->GetLocalIdByObjectId(object_id);
    LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnResolvedBookmarks: local_id=" << local_id;

    DCHECK(sync_record.objectId == object_id);
    DCHECK( base::NumberToString(sync_record.action) == action);

    if (action == jslib_const::CREATE_RECORD && local_id.empty()) {
      std::string location = ExtractBookmarkLocation(&sync_record_value);
      std::string title = ExtractBookmarkTitle(&sync_record_value);
      LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnResolvedBookmarks: location=" << location;
      LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnResolvedBookmarks: title=" << title;
      LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnResolvedBookmarks: site.location=" << sync_record.GetBookmark().site.location;
      LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnResolvedBookmarks: site.title=" << sync_record.GetBookmark().site.title;
      DCHECK(location == sync_record.GetBookmark().site.location);
      DCHECK(title == sync_record.GetBookmark().site.title);
      bookmarks_->AddBookmark(sync_record);
    }
  }
}

void ControllerImpl::OnResolvedHistorySites(const std::string &category_name,
  std::unique_ptr<base::Value> records_v) {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::OnResolvedHistorySites:";
}

std::unique_ptr<base::Value> ControllerImpl::PrepareResolvedResponse(
  const std::string &category_name,
  const std::unique_ptr<base::Value> &sync_records_list) {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::PrepareResolvedResponse: category_name="<<category_name;

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
    LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::PrepareResolvedResponse object_id=" << object_id;

    if (category_name == jslib_const::kBookmarks) {
      //"BOOKMARKS"
      p_local_record = bookmarks_->GetResolvedBookmarkValue(object_id);
    } else if (category_name == jslib_const::kHistorySites) {
      //"HISTORY_SITES";
      p_local_record = std::make_unique<base::Value>(base::Value::Type::NONE);
      NOTIMPLEMENTED();
    } else if (category_name == jslib_const::kPreferences) {
      //"PREFERENCES"
      LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::PrepareResolvedResponse: resolving device";
      p_local_record = PrepareResolvedDevice(object_id);
      LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::PrepareResolvedResponse *p_local_record=" << std::endl << brave::debug::ToPrintableString(*p_local_record);
      LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::PrepareResolvedResponse: -----------------";
    }

// if (p_local_record->is_none()) {
//   LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::PrepareResolvedResponse" <<std::endl
//     <<"unexpected empty resolve for object_id="<<object_id;
//   NOTREACHED();
// }

    std::unique_ptr<base::Value> resolvedResponseRow(new base::Value(base::Value::Type::LIST));
    resolvedResponseRow->GetList().push_back(std::move(server_record));
    resolvedResponseRow->GetList().push_back(std::move(*p_local_record));
    resolvedResponse->GetList().push_back(std::move(*resolvedResponseRow));
  }

  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::PrepareResolvedResponse *resolvedResponse" << std::endl << brave::debug::ToPrintableString(*resolvedResponse);
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::PrepareResolvedResponse -----------------------------";
  return resolvedResponse;
}

std::unique_ptr<base::Value> ControllerImpl::PrepareResolvedDevice(const std::string &object_id) {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::PrepareResolvedResponse object_id=" << object_id;
  return std::make_unique<base::Value>(base::Value::Type::NONE);
  // std::string json = sync_obj_map_->GetObjectIdByLocalId(jslib_const::DEVICES_NAMES);
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
}

void ControllerImpl::SendResolveSyncRecords(
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
void ControllerImpl::FetchSyncRecords(const bool &bookmarks,
  const bool &history, const bool &preferences, int64_t start_at, int max_records) {
    LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::FetchSyncRecords:";
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

void ControllerImpl::SendCreateDevice() {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::SendCreateDevice";
  //SetUpdateDeleteDeviceName(CREATE_RECORD, mDeviceName, mDeviceId, "");

  std::string deviceName = sync_prefs_->GetThisDeviceName();
  std::string objectId = brave_sync::tools::GenerateObjectId();
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::SendCreateDevice deviceName=" << deviceName;
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::SendCreateDevice objectId="<<objectId;
  std::string deviceId = sync_prefs_->GetThisDeviceId();
  CHECK(!deviceId.empty());

  std::stringstream request;
  request << "[";
  //CreateDeviceCreationRecord(deviceName, objectId, action, deviceId);
  std::string action = jslib_const::CREATE_RECORD;
  std::string stmp = CreateDeviceCreationRecord(deviceName, objectId, action, deviceId);
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::SendCreateDevice objectId="<<stmp;
  request << stmp;
  request << "]";

  std::vector<std::string> ids;
  //SendSyncRecords(SyncRecordType.PREFERENCES, request, action, ids);
  SendSyncRecords(jslib_const::SyncRecordType_PREFERENCES, request.str(), action, ids);
}

void ControllerImpl::SendSyncRecords(const std::string &recordType,
  const std::string &recordsJSON,
  const std::string &action,
  const std::vector<std::string> &ids) {

  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::SendSyncRecords";
  LOG(ERROR) << "TAGAB recordType=" << recordType;
  LOG(ERROR) << "TAGAB recordsJSON=" << recordsJSON;
  LOG(ERROR) << "TAGAB action=" << action;

  if (!sync_initialized_) {
    LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::SendSyncRecords sync is not initialized";
    DCHECK(false);
    return;
  }

  SaveGetDeleteNotSyncedRecords(recordType, action, ids, NotSyncedRecordsOperation::AddItems);
  CallJsLibStr("send-sync-records", std::string(), recordType, recordsJSON, std::string());
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
  std::vector<const bookmarks::BookmarkNode*> localBookmarks;
  bookmarks_->GetAllBookmarks(localBookmarks);

  for(size_t i = 0; i < localBookmarks.size(); i += SEND_RECORDS_COUNT_LIMIT) {
    size_t sub_list_last = std::min(localBookmarks.size(), i + SEND_RECORDS_COUNT_LIMIT);
    std::vector<const bookmarks::BookmarkNode*> sub_list(localBookmarks.begin()+i, localBookmarks.begin()+sub_list_last);
    CreateUpdateDeleteBookmarks(jslib_const::kActionCreate, sub_list, true, true);
  }
}

void ControllerImpl::CreateUpdateDeleteBookmarks(
  const int &action,
  const std::vector<const bookmarks::BookmarkNode*> &list,
  const bool &addIdsToNotSynced,
  const bool &isInitialSync) {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::CreateUpdateDeleteBookmarks";

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

void ControllerImpl::SendAllLocalHistorySites() {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::SendAllLocalHistorySites";

}

std::string ControllerImpl::CreateDeviceCreationRecord(
  const std::string &deviceName,
  const std::string &objectId,
  const std::string &action,
  const std::string &deviceId) {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::CreateDeviceCreationRecord";
  DCHECK(!deviceName.empty());
  if (deviceName.empty()) {
    return std::string();
  }

  std::stringstream ss;
  ss << "{ \"action\": " << action << ", ";
  ss << "\"deviceId\": [" << deviceId << "], ";
  ss << "\"objectId\": [" << objectId << "], ";
  ss << "\"" << jslib_const::SyncObjectData_DEVICE << "\"" << ": { \"name\": \"" << brave_sync::tools::replaceUnsupportedCharacters(deviceName) << "\"}}";

  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::CreateDeviceCreationRecord ss.str()=" << ss.str();

  return ss.str();
}

void ControllerImpl::SetUpdateDeleteDeviceName(
  const std::string &action,
  const std::string &deviceName,
  const std::string &deviceId,
  const std::string &objectId) {
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::SetUpdateDeleteDeviceName";
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
  LOG(ERROR) << "TAGAB brave_sync::ControllerImpl::SetUpdateDeleteDeviceName stmp="<<stmp;
  request << stmp;
  request << "]";

  std::vector<std::string> ids;
  SendSyncRecords(jslib_const::SyncRecordType_PREFERENCES, request.str(), action, ids);
}

std::string ControllerImpl::GenerateObjectIdWithMapCheck(const std::string &local_id) {
  std::string res = sync_obj_map_->GetObjectIdByLocalId(local_id);
  if (!res.empty()) {
    return res;
  }

  return brave_sync::tools::GenerateObjectId();
}

static const int64_t kCheckUpdatesIntervalSec = 30;

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



} // namespace brave_sync
