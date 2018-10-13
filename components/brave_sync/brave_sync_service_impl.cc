/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/brave_sync_service_impl.h"

#include <sstream>

#include "base/strings/utf_string_conversions.h"
#include "base/task_runner.h"
#include "base/task/post_task.h"
#include "brave/browser/ui/brave_pages.h"
#include "brave/browser/ui/webui/sync/sync_ui.h"
#include "brave/common/extensions/extension_constants.h"
#include "brave/components/brave_sync/bookmark_order_util.h"
#include "brave/components/brave_sync/brave_sync_service_observer.h"
#include "brave/components/brave_sync/client/brave_sync_client.h"
#include "brave/components/brave_sync/client/brave_sync_client_factory.h"
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
#include "brave/vendor/bip39wally-core-native/include/wally_bip39.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser.h"
#include "content/public/browser/browser_thread.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_storage.h"
#include "components/bookmarks/browser/bookmark_utils.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/common/one_shot_event.h"
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

void SendDeviceSyncRecord(
    base::WeakPtr<BraveSyncClient> sync_client,
    const int &action,
    const std::string &device_name,
    const std::string &device_id,
    const std::string &object_id) {
  if (!sync_client.get())
    return;

  RecordsListPtr records = CreateDeviceCreationRecordExtension(device_name, object_id,
    static_cast<jslib::SyncRecord::Action>(action),
    device_id);
  sync_client->SendSyncRecords(jslib_const::SyncRecordType_PREFERENCES, *records);
}

void OnDeleteDeviceFileWork(base::WeakPtr<BraveSyncClient> sync_client,
                            storage::ObjectMap* sync_object_map,
                            const std::string &device_id) {
  VLOG(1) << "TAGAB  OnDeleteDeviceFileWork";
  std::string json = sync_object_map->GetSpecialJSONByLocalId(jslib_const::DEVICES_NAMES);
  SyncDevices syncDevices;
  syncDevices.FromJson(json);
  VLOG(1) << "TAGAB brave_sync::BraveSyncServiceImpl::OnDeleteDeviceFileWork json="<<json;

  const SyncDevice *device = syncDevices.GetByDeviceId(device_id);
  VLOG(1) << "TAGAB brave_sync::BraveSyncServiceImpl::OnDeleteDeviceFileWork device="<<device;
  //DCHECK(device); // once I saw it nullptr
  if (device) {
    const std::string device_name = device->name_;
    const std::string object_id = device->object_id_;
    VLOG(1) << "TAGAB brave_sync::BraveSyncServiceImpl::OnDeleteDeviceFileWork device_name="<<device_name;
    VLOG(1) << "TAGAB brave_sync::BraveSyncServiceImpl::OnDeleteDeviceFileWork object_id="<<object_id;

    content::BrowserThread::PostTask(content::BrowserThread::UI, FROM_HERE,
        base::Bind(&SendDeviceSyncRecord,
            sync_client,
            jslib::SyncRecord::Action::DELETE,
            device_name,
            device_id,
            object_id));
  }
}

void OnResetSyncFileWork(
    base::WeakPtr<BraveSyncClient> sync_client,
    storage::ObjectMap* sync_object_map,
    const std::string &device_id) {
  VLOG(1) << "TAGAB brave_sync::BraveSyncServiceImpl::OnResetSyncFileWork";
  OnDeleteDeviceFileWork(sync_client, sync_object_map, device_id);
  sync_object_map->DestroyDB();
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

  std::unique_ptr<jslib::Device> device_record = std::make_unique<jslib::Device>();
  device_record->name = device->name_;
  record->SetDevice(std::move(device_record));

  return record;
}

std::unique_ptr<SyncRecordAndExistingList>
PrepareResolvedPreferences(
    storage::ObjectMap* sync_object_map,
    std::unique_ptr<RecordsList> records) {
  SyncDevices devices;
  devices.FromJson(
      sync_object_map->GetSpecialJSONByLocalId(jslib_const::DEVICES_NAMES));

  auto records_and_existing_objects =
        std::make_unique<SyncRecordAndExistingList>();

  for (const SyncRecordPtr& record : *records) {
    auto resolved_record = std::make_unique<SyncRecordAndExisting>();
    resolved_record->first = jslib::SyncRecord::Clone(*record);
    auto* device = devices.GetByObjectId(record->objectId);
    if (device)
      resolved_record->second = PrepareResolvedDevice(device, record->action);
    records_and_existing_objects->emplace_back(std::move(resolved_record));
  }

  return records_and_existing_objects;
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
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  CHECK(bookmark_model_);
  DLOG(INFO) << "[Brave Sync] " << __func__;

  // Handle when the extension system is ready
  extensions::ExtensionSystem::Get(profile)->ready().Post(
      FROM_HERE, base::Bind(&BraveSyncServiceImpl::OnExtensionSystemReady,
                            weak_ptr_factory_.GetWeakPtr()));

  DETACH_FROM_SEQUENCE(sequence_checker_);

  sync_prefs_ = std::make_unique<brave_sync::prefs::Prefs>(profile);

  DLOG(INFO) << "[Brave Sync] " << __func__ << " sync_prefs_->GetSeed()="
    << sync_prefs_->GetSeed();
  DLOG(INFO) << "[Brave Sync] " << __func__ <<
  " sync_prefs_->GetThisDeviceName()=" << sync_prefs_->GetThisDeviceName();

  task_runner_ = base::CreateSequencedTaskRunnerWithTraits(
      {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
      base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN} );

  sync_obj_map_ = std::make_unique<storage::ObjectMap>(profile->GetPath());

  history_ = std::make_unique<brave_sync::History>(/*this, */profile, this);
  if (!sync_prefs_->GetThisDeviceId().empty()) {
    history_->SetThisDeviceId(sync_prefs_->GetThisDeviceId());
  }
  history_->SetObjectMap(sync_obj_map_.get());


  if (!sync_prefs_->GetSeed().empty() &&
      !sync_prefs_->GetThisDeviceName().empty()) {
    DLOG(INFO) << "[Brave Sync] " << __func__ << " sync is configured";
    sync_configured_ = true;
  } else {
    DLOG(INFO) << "[Brave Sync] " << __func__ << " sync is NOT configured";
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
  LOG(ERROR) << "TAGAB brave_sync::BraveSyncServiceImpl::IsSyncConfigured will return sync_configured_="<<sync_configured_;
  return sync_configured_;
}

bool BraveSyncServiceImpl::IsSyncInitialized() {
  LOG(ERROR) << "TAGAB brave_sync::BraveSyncServiceImpl::IsSyncConfigured will return sync_initialized_="<<sync_initialized_;
  return sync_initialized_;
}

void BraveSyncServiceImpl::Shutdown() {
  LOG(ERROR) << "TAGAB BraveSyncServiceImpl::Shutdown";

  history_.reset();

  task_runner_->DeleteSoon(FROM_HERE, sync_obj_map_.release());
}

void BraveSyncServiceImpl::OnSetupSyncHaveCode(const std::string &sync_words,
  const std::string &device_name) {
  DLOG(INFO) << "[Brave Sync] " << __func__ << " sync_words=" << sync_words <<
    " device_name=" << device_name;
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (sync_words.empty() || device_name.empty()) {
    OnSyncSetupError("missing sync words or device name");
    return;
  }

  if (initializing_) {
    TriggerOnLogMessage("currently initializing");
    return;
  }

  if (IsSyncConfigured()) {
    TriggerOnLogMessage("already configured");
    return;
  }

  sync_prefs_->SetThisDeviceName(device_name);
  initializing_ = true;

  sync_prefs_->SetSyncThisDevice(true);
  sync_words_ = sync_words;
}

void BraveSyncServiceImpl::OnSetupSyncNewToSync(const std::string &device_name) {
  LOG(ERROR) << "TAGAB brave_sync::BraveSyncServiceImpl::OnSetupSyncNewToSync";
  LOG(ERROR) << "TAGAB device_name="<<device_name;
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (device_name.empty()) {
    OnSyncSetupError("missing device name");
    return;
  }

  if (initializing_) {
    TriggerOnLogMessage("currently initializing");
    return;
  }

  if (IsSyncConfigured()) {
    TriggerOnLogMessage("already configured");
    return;
  }

  sync_prefs_->SetThisDeviceName(device_name);
  initializing_ = true;

  sync_prefs_->SetSyncThisDevice(true);
}

void BraveSyncServiceImpl::OnDeleteDevice(const std::string &device_id) {
  VLOG(1) << "TAGAB brave_sync::BraveSyncServiceImpl::OnDeleteDevice";
  VLOG(1) << "TAGAB device_id="<<device_id;
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  CHECK(sync_client_ != nullptr);
  CHECK(sync_initialized_);

  task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&OnDeleteDeviceFileWork,
          sync_client_->AsWeakPtr(),
          sync_obj_map_.get(),
          device_id)
  );
}

void BraveSyncServiceImpl::OnResetSync() {
  LOG(ERROR) << "TAGAB  BraveSyncServiceImpl::OnResetSync";
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  DCHECK(sync_client_ != nullptr);
  //DCHECK(sync_initialized_);

  const std::string device_id = sync_prefs_->GetThisDeviceId();
  LOG(ERROR) << "TAGAB brave_sync::BraveSyncServiceImpl::OnResetSync device_id="<<device_id;

  OnResetBookmarks();

  task_runner_->PostTaskAndReply(
      FROM_HERE,
      base::BindOnce(&OnResetSyncFileWork,
          sync_client_->AsWeakPtr(),
          sync_obj_map_.get(),
          device_id),
      base::BindOnce(&BraveSyncServiceImpl::TriggerOnSyncStateChanged,
          weak_ptr_factory_.GetWeakPtr()));

  sync_prefs_->Clear();

  sync_configured_ = false;
  sync_initialized_ = false;

  sync_prefs_->SetSyncThisDevice(false);
}

void BraveSyncServiceImpl::GetSettingsAndDevices(const GetSettingsAndDevicesCallback &callback) {
  LOG(ERROR) << "TAGAB brave_sync::BraveSyncServiceImpl::GetSettingsAndDevices " << GetThreadInfoString();
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  // pref service should be queried on UI thread in anyway
  std::unique_ptr<brave_sync::Settings> settings = sync_prefs_->GetBraveSyncSettings();

  // Jump to task runner thread to perform FILE operation and then back to UI
  task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&BraveSyncServiceImpl::GetSettingsAndDevicesImpl,
                      weak_ptr_factory_.GetWeakPtr(),
                      std::move(settings), std::move(callback))
  );
}

void BraveSyncServiceImpl::GetSettingsAndDevicesImpl(
    std::unique_ptr<brave_sync::Settings> settings,
    const GetSettingsAndDevicesCallback &callback) {
  LOG(ERROR) << "TAGAB brave_sync::BraveSyncServiceImpl::GetSettingsAndDevicesImpl " << GetThreadInfoString();
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  std::unique_ptr<brave_sync::SyncDevices> devices = std::make_unique<brave_sync::SyncDevices>();
  std::string json = sync_obj_map_->GetSpecialJSONByLocalId(jslib_const::DEVICES_NAMES);
  devices->FromJson(json);
  LOG(ERROR) << "TAGAB brave_sync::BraveSyncServiceImpl::GetSettingsAndDevicesImpl json="<<json;

  content::BrowserThread::PostTask(content::BrowserThread::UI, FROM_HERE,
      base::Bind(std::move(callback),
          base::Passed(std::move(settings)),
          base::Passed(std::move(devices)))
  );
}

void BraveSyncServiceImpl::GetSyncWords() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  LOG(ERROR) << "TAGAB brave_sync::BraveSyncServiceImpl::GetSyncWords";

  // Ask sync client
  DCHECK(sync_client_);
  std::string seed = sync_prefs_->GetSeed();
  sync_client_->NeedSyncWords(seed);
}

std::string BraveSyncServiceImpl::GetSeed() {
  LOG(ERROR) << "TAGAB brave_sync::BraveSyncServiceImpl::GetSeed";
  std::string seed = sync_prefs_->GetSeed();
  return seed;
}

void BraveSyncServiceImpl::OnSetSyncThisDevice(const bool &sync_this_device) {
  LOG(ERROR) << "TAGAB brave_sync::BraveSyncServiceImpl::OnSetSyncThisDevice " << sync_this_device;
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  sync_prefs_->SetSyncThisDevice(sync_this_device);
}

void BraveSyncServiceImpl::OnSetSyncBookmarks(const bool &sync_bookmarks) {
  LOG(ERROR) << "TAGAB brave_sync::BraveSyncServiceImpl::OnSetSyncBookmarks " << sync_bookmarks;
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  sync_prefs_->SetSyncBookmarksEnabled(sync_bookmarks);
}

void BraveSyncServiceImpl::OnSetSyncBrowsingHistory(const bool &sync_browsing_history) {
  LOG(ERROR) << "TAGAB brave_sync::BraveSyncServiceImpl::OnSetSyncBrowsingHistory " << sync_browsing_history;
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  sync_prefs_->SetSyncHistoryEnabled(sync_browsing_history);
}

void BraveSyncServiceImpl::OnSetSyncSavedSiteSettings(const bool &sync_saved_site_settings) {
  LOG(ERROR) << "TAGAB brave_sync::BraveSyncServiceImpl::OnSyncSavedSiteSettings " << sync_saved_site_settings;
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  sync_prefs_->SetSyncSiteSettingsEnabled(sync_saved_site_settings);
}

void BraveSyncServiceImpl::OnMessageFromSyncReceived() {
  ;
}

// SyncLibToBrowserHandler overrides
void BraveSyncServiceImpl::OnSyncDebug(const std::string &message) {
  LOG(ERROR) << "TAGAB brave_sync::BraveSyncServiceImpl::OnSyncDebug: message=<" << message << ">";
  TriggerOnLogMessage(message);
}

void BraveSyncServiceImpl::OnSyncSetupError(const std::string &error) {
  LOG(ERROR) << "[Brave Sync] " << __func__ << "error: " << error;
  initializing_ = false;
  if (!sync_initialized_) {
    sync_prefs_->Clear();
  }
  OnSyncDebug(error);
}

void BraveSyncServiceImpl::OnGetInitData(const std::string &sync_version) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DLOG(INFO) << "[Brave Sync] " << __func__ << " sync_version=" << sync_version;

  Uint8Array seed;
  if (!sync_words_.empty()) {
    DLOG(INFO) << "[Brave Sync] " << __func__ << " add device to existing" <<
      "chain, seed from sync words will be sent back in OnSaveInitData()";
  } else if (!sync_prefs_->GetSeed().empty()) {
    seed = Uint8ArrayFromString(sync_prefs_->GetSeed());
    DLOG(INFO) << "[Brave Sync] " << __func__ << " take seed from prefs. Seed="
      << sync_prefs_->GetSeed();
  } else {
    // We are starting a new chain, so we don't know neither seed nor device id
    DLOG(INFO) << "[Brave Sync] " << __func__ << "starting new chaing, " <<
      "new seed will be sent back in OnSaveInitData()";
  }

  Uint8Array device_id;
  if (!sync_prefs_->GetThisDeviceId().empty()) {
    device_id = Uint8ArrayFromString(sync_prefs_->GetThisDeviceId());
    DLOG(INFO) << "[Brave Sync] " << __func__ << " use device id from prefs," <<
      "device_id="<< StrFromUint8Array(device_id);
  } else {
    DLOG(INFO) << "[Brave Sync] " << __func__ << " use empty device id";
  }

  DCHECK(!sync_version.empty());
  sync_version_ = sync_version;
  sync_obj_map_->SetApiVersion("0");

  brave_sync::client_data::Config config;
  config.api_version = "0";
#if defined(OFFICIAL_BUILD)
  config.server_url = "https://sync.brave.com";
#else
  config.server_url = "https://sync-staging.brave.com";
#endif
  config.debug = true;
  sync_client_->SendGotInitData(seed, device_id, config, sync_words_);
}

void BraveSyncServiceImpl::OnSaveInitData(const Uint8Array &seed, const Uint8Array &device_id) {
  DLOG(INFO) << "[Brave Sync] " << __func__;
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  DCHECK(!sync_initialized_);
  DCHECK(initializing_);

  std::string seed_str = StrFromUint8Array(seed);
  std::string device_id_str = StrFromUint8Array(device_id);

  DLOG(INFO) << "[Brave Sync] " << __func__ << " seed=" << seed_str <<
    " device_id=" << device_id_str;

  sync_words_.clear();
  DCHECK(!seed_str.empty());
  sync_prefs_->SetSeed(seed_str);
  sync_prefs_->SetThisDeviceId(device_id_str);

  sync_configured_ = true;

  sync_prefs_->SetSyncBookmarksEnabled(true);
  sync_prefs_->SetSyncSiteSettingsEnabled(true);
  sync_prefs_->SetSyncHistoryEnabled(true);

  initializing_ = false;
}

void BraveSyncServiceImpl::OnSyncReady() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DLOG(INFO) << "[Brave Sync] " << __func__;
  const std::string bookmarks_base_order = sync_prefs_->GetBookmarksBaseOrder();
  DLOG(INFO) << "[Brave Sync] " << __func__ << " bookmarks_base_order=" <<
    bookmarks_base_order;
  if (bookmarks_base_order.empty()) {
    std::string platform = tools::GetPlatformName();
    DLOG(INFO) << "[Brave Sync] " << __func__ << " platform=" << platform;
    DLOG(INFO) << "[Brave Sync] " << __func__ <<
      " sync_prefs_->GetThisDeviceId()=" << sync_prefs_->GetThisDeviceId();
    sync_client_->SendGetBookmarksBaseOrder(sync_prefs_->GetThisDeviceId(),
                                            platform);
    // OnSyncReady will be called by OnSaveBookmarksBaseOrder
    return;
  }

  DCHECK(false == sync_initialized_);
  sync_initialized_ = true;

  TriggerOnSyncStateChanged();

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

void BraveSyncServiceImpl::OnGetExistingObjects(const std::string &category_name,
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
    base::PostTaskAndReplyWithResult(task_runner_.get(),
        FROM_HERE,
        base::BindOnce(
            &PrepareResolvedPreferences,
                // the sync object map is destroyed by the task_runner
                // so it's always safe to pass it without WeakPtr
                sync_obj_map_.get(),
                std::move(records)),
        base::BindOnce(
            &BraveSyncClient::SendResolveSyncRecords,
                sync_client_->AsWeakPtr(),
                category_name));
  }
}

void BraveSyncServiceImpl::OnResolvedSyncRecords(const std::string &category_name,
  std::unique_ptr<RecordsList> records) {
  LOG(ERROR) << "TAGAB OnResolvedSyncRecords records->size()=" << records->size();
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (category_name == brave_sync::jslib_const::kPreferences) {
    std::string this_device_id = sync_prefs_->GetThisDeviceId();
    task_runner_->PostTaskAndReply(
        FROM_HERE,
        base::BindOnce(&BraveSyncServiceImpl::OnResolvedPreferences,
            weak_ptr_factory_.GetWeakPtr(),
            std::move(records),
            this_device_id),
        base::BindOnce(&BraveSyncServiceImpl::TriggerOnSyncStateChanged,
            weak_ptr_factory_.GetWeakPtr()));
  } else if (category_name == brave_sync::jslib_const::kBookmarks) {
    OnResolvedBookmarks(*records.get());
  } else if (category_name == brave_sync::jslib_const::kHistorySites) {
    NOTIMPLEMENTED();
  }

  SendUnsyncedBookmarks();
}

void BraveSyncServiceImpl::OnResolvedPreferences(
    std::unique_ptr<RecordsList> records,
    const std::string& this_device_id) {
  DLOG(INFO) << "[Brave Sync] " << __func__ << ":";
  DLOG(INFO) << "[Brave Sync] this_device_id=" << this_device_id;
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  SyncDevices existing_sync_devices;
  std::string json = sync_obj_map_->GetSpecialJSONByLocalId(jslib_const::DEVICES_NAMES);
  LOG(ERROR) << "TAGAB brave_sync::BraveSyncServiceImpl::OnResolvedPreferences: existing json=<" << json << ">";
  existing_sync_devices.FromJson(json);

  bool this_device_deleted = false;

  for (const auto &record : *records) {
    DCHECK(record->has_device() || record->has_sitesetting());
    if (record->has_device()) {
      DLOG(INFO) << "[Brave Sync] record->GetDevice().name=" <<
                    record->GetDevice().name;
      DLOG(INFO) << "[Brave Sync] record->syncTimestamp=<" <<
                    record->syncTimestamp << ">";
      DLOG(INFO) << "[Brave Sync] record->deviceId=" << record->deviceId;
      DLOG(INFO) << "[Brave Sync] record->objectId=" << record->objectId;
      DLOG(INFO) << "[Brave Sync] record->action=" << record->action;

      bool actually_merged = false;
      existing_sync_devices.Merge(SyncDevice(record->GetDevice().name,
          record->objectId, record->deviceId, record->syncTimestamp.ToJsTime()),
          record->action, actually_merged);
      this_device_deleted = this_device_deleted ||
        (record->deviceId == this_device_id && record->action == jslib::SyncRecord::Action::DELETE && actually_merged);
    }
  } // for each device

  DCHECK(existing_sync_devices.devices_.size() > 0)
    << "existing_sync_devices.devices_.size() =="
    << existing_sync_devices.devices_.size();

  std::string sync_devices_json = existing_sync_devices.ToJson();
  DLOG(INFO) << "[Brave Sync] OnResolvedPreferences sync_devices_json="<<sync_devices_json;

  DLOG(INFO) << "[Brave Sync] OnResolvedPreferences before SaveObjectId";
  sync_obj_map_->SaveSpecialJson(jslib_const::DEVICES_NAMES, sync_devices_json);
  DLOG(INFO) << "[Brave Sync] OnResolvedPreferences SaveObjectId done";

  DLOG(INFO) << "[Brave Sync] " << __func__ << " this_device_deleted=" << this_device_deleted;
  // We received request to delete the current device
  if (this_device_deleted) {
    OnResetSyncFileWork(
        sync_client_->AsWeakPtr(), sync_obj_map_.get(), this_device_id);
  }

  DLOG(INFO) << "[Brave Sync] OnResolvedPreferences OnSyncStateChanged() done";
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
    } else if (node && sync_record->action == jslib::SyncRecord::Action::DELETE) {
      if (node->parent() == GetDeletedNodeRoot()) {
        // this is a deleted node so remove without firing events
        int index = GetDeletedNodeRoot()->GetIndexOf(node);
        GetDeletedNodeRoot()->Remove(index);
      } else {
        // normal remove
        bookmark_model_->Remove(node);
      }
    } else if (!node) {
      // TODO(bridiver) (make sure there isn't an existing record with the objectId)

      const bookmarks::BookmarkNode* parent_node =
          FindByObjectId(bookmark_model_, bookmark_record.parentFolderObjectId);

      if (!parent_node) {
        if (!bookmark_record.order.empty() &&
            bookmark_record.order.at(0) == '2') {
          // mobile generated bookmarks go in the mobile folder so they don't get
          // so we don't get m.xxx.xxx domains in the normal bookmarks
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

void BraveSyncServiceImpl::OnResolvedHistorySites(const RecordsList &records) {
  NOTIMPLEMENTED();
}

void BraveSyncServiceImpl::OnDeletedSyncUser() {
  NOTIMPLEMENTED();
}

void BraveSyncServiceImpl::OnDeleteSyncSiteSettings()  {
  NOTIMPLEMENTED();
}

void BraveSyncServiceImpl::OnSaveBookmarksBaseOrder(const std::string &order)  {
  DLOG(INFO) << "[Brave Sync ] " << __func__ << "order=" << order;
  DCHECK(!order.empty());
  sync_prefs_->SetBookmarksBaseOrder(order);
  DLOG(INFO) << "[Brave Sync ] " << __func__ << " forced call of OnSyncReady";
  OnSyncReady();
}

void BraveSyncServiceImpl::OnSaveBookmarkOrder(const std::string &order,
                                               const std::string &prev_order,
                                               const std::string &next_order,
                                               const std::string &parent_order) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(!order.empty());

  int64_t between_order_rr_context_node_id = -1;
  int action = -1;

  PopRRContext(prev_order, next_order, parent_order,
      between_order_rr_context_node_id, action);

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

void BraveSyncServiceImpl::PushRRContext(const std::string &prev_order,
                                         const std::string &next_order,
                                         const std::string &parent_order,
                                         const int64_t &node_id,
                                         const int &action) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  std::string key(prev_order + "-" + next_order + "-" + parent_order);
  DCHECK(rr_map_.find(key) == rr_map_.end());
  rr_map_[key] = std::make_tuple(node_id, action);
}

void BraveSyncServiceImpl::PopRRContext(const std::string &prev_order,
                                        const std::string &next_order,
                                        const std::string &parent_order,
                                        int64_t &node_id,
                                        int &action) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  std::string key(prev_order + "-" + next_order + "-" + parent_order);
  auto it = rr_map_.find(key);
  DCHECK(it != rr_map_.end());
  node_id = std::get<0>(it->second);
  action = std::get<1>(it->second);
  rr_map_.erase(it);
}

void BraveSyncServiceImpl::OnSyncWordsPrepared(const std::string &words) {
  TriggerOnHaveSyncWords(words);
}

// Here we query sync lib for the records after initialization (or again later)
void BraveSyncServiceImpl::RequestSyncData() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  LOG(ERROR) << "TAGAB brave_sync::BraveSyncServiceImpl::RequestSyncData: sync_prefs_->GetSyncThisDevice()=" << sync_prefs_->GetSyncThisDevice();
  if (!sync_prefs_->GetSyncThisDevice()) {
    LOG(ERROR) << "TAGAB brave_sync::BraveSyncServiceImpl::RequestSyncData: sync is not enabled for this device";
    return;
  }

  // still sending sync records
  if (initial_sync_records_remaining_ > 0)
    return;

  const bool bookmarks = sync_prefs_->GetSyncBookmarksEnabled();
  const bool history = sync_prefs_->GetSyncHistoryEnabled();
  const bool preferences = sync_prefs_->GetSyncSiteSettingsEnabled();

  LOG(ERROR) << "TAGAB brave_sync::BraveSyncServiceImpl::RequestSyncData: bookmarks="<<bookmarks;
  LOG(ERROR) << "TAGAB brave_sync::BraveSyncServiceImpl::RequestSyncData: history="<<history;
  LOG(ERROR) << "TAGAB brave_sync::BraveSyncServiceImpl::RequestSyncData: preferences="<<preferences;

  if (!bookmarks && !history && !preferences) {
    LOG(ERROR) << "TAGAB brave_sync::BraveSyncServiceImpl::RequestSyncData: none of option is enabled, abort";
    return;
  }

  base::Time last_fetch_time = sync_prefs_->GetLastFetchTime();

  LOG(ERROR) << "TAGAB brave_sync::BraveSyncServiceImpl::RequestSyncData: last_fetch_time="<<last_fetch_time;

  //bool dbg_ignore_create_device = true; //the logic should rely upon sync_prefs_->GetTimeLastFetch() which is not saved yet
  if (tools::IsTimeEmpty(last_fetch_time)) {
    //SetUpdateDeleteDeviceName(CREATE_RECORD, mDeviceName, mDeviceId, "");
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
    //SendAllLocalHistorySites();

    if (initial_sync_records_remaining_ > 0)
      return;
  }

  FetchSyncRecords(bookmarks, history, preferences, 1000);
}

void BraveSyncServiceImpl::FetchSyncRecords(const bool bookmarks,
  const bool history, const bool preferences, int max_records) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  LOG(ERROR) << "TAGAB brave_sync::BraveSyncServiceImpl::FetchSyncRecords:";
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
  LOG(ERROR) << "TAGAB brave_sync::BraveSyncServiceImpl::SendCreateDevice";

  std::string device_name = sync_prefs_->GetThisDeviceName();
  std::string object_id = brave_sync::tools::GenerateObjectId();
  LOG(ERROR) << "TAGAB brave_sync::BraveSyncServiceImpl::SendCreateDevice deviceName=" << device_name;
  LOG(ERROR) << "TAGAB brave_sync::BraveSyncServiceImpl::SendCreateDevice objectId=" << object_id;
  std::string device_id = sync_prefs_->GetThisDeviceId();
  LOG(ERROR) << "TAGAB brave_sync::BraveSyncServiceImpl::SendCreateDevice deviceId=" << device_id;
  CHECK(!device_id.empty());

  SendDeviceSyncRecord(
      sync_client_->AsWeakPtr(),
      jslib::SyncRecord::Action::CREATE,
      device_name,
      device_id,
      object_id);
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
          base::Time::Now() - base::Time::FromJsTime(std::stod(last_send_time)) <
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

void BraveSyncServiceImpl::SendAllLocalHistorySites() {
  LOG(ERROR) << "TAGAB brave_sync::BraveSyncServiceImpl::SendAllLocalHistorySites";
  ///static const int SEND_RECORDS_COUNT_LIMIT = 1000;
  // history_->GetAllHistory();

  // for(size_t i = 0; i < localBookmarks.size(); i += SEND_RECORDS_COUNT_LIMIT) {
  //   size_t sub_list_last = std::min(localBookmarks.size(), i + SEND_RECORDS_COUNT_LIMIT);
  //   std::vector<const bookmarks::BookmarkNode*> sub_list(localBookmarks.begin()+i, localBookmarks.begin()+sub_list_last);
  //   CreateUpdateDeleteBookmarks(jslib_const::kActionCreate, sub_list, true, true);
  // }
}

void BraveSyncServiceImpl::HaveInitialHistory(history::QueryResults* results) {
  LOG(ERROR) << "TAGAB brave_sync::BraveSyncServiceImpl::HaveInitialHistory";
  static const int SEND_RECORDS_COUNT_LIMIT = 1000;

  if (!results || results->empty() || !sync_initialized_ || !sync_prefs_->GetSyncHistoryEnabled() ) {
    return;
  }

  if (results && !results->empty()) {
    LOG(ERROR) << "TAGAB brave_sync::BraveSyncServiceImpl::HaveInitialHistory results->size()=" << results->size();
    for (const auto& item : *results){
      //history_item_vec.push_back(GetHistoryItem(item));
      //item.visit_time();
      item.visit_time();
      LOG(ERROR) << "brave_sync::BraveSyncServiceImpl::HaveInitialHistory item=" << item.url().spec();
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


void BraveSyncServiceImpl::CreateUpdateDeleteHistorySites(
    const int &action,
    //const history::QueryResults::URLResultVector &list,
    const std::vector<history::URLResult> &list,
    const bool &addIdsToNotSynced,
    const bool &isInitialSync) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  LOG(ERROR) << "TAGAB brave_sync::BraveSyncServiceImpl::CreateUpdateDeleteHistorySites";

  if (list.empty() || !sync_initialized_ || !sync_prefs_->GetSyncHistoryEnabled() ) {
    return;
  }

  DCHECK(sync_client_);
  std::unique_ptr<RecordsList> records = history_->NativeHistoryToSyncRecords(list, action);
  sync_client_->SendSyncRecords(jslib_const::SyncRecordType_HISTORY, *records);
}

static const int64_t kCheckUpdatesIntervalSec = 60;

void BraveSyncServiceImpl::StartLoop() {
  DLOG(INFO) << "[Brave Sync] " << __func__;
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  bookmark_model_->AddObserver(this);
  timer_->Start(FROM_HERE, base::TimeDelta::FromSeconds(kCheckUpdatesIntervalSec),
                 this, &BraveSyncServiceImpl::LoopProc);
}

void BraveSyncServiceImpl::StopLoop() {
  DLOG(INFO) << "[Brave Sync] " << __func__;
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  timer_->Stop();
  bookmark_model_->RemoveObserver(this);
}

void BraveSyncServiceImpl::LoopProc() {
  content::BrowserThread::GetTaskRunnerForThread(
      content::BrowserThread::UI)->PostTask(
          FROM_HERE,
          base::Bind(&BraveSyncServiceImpl::LoopProcThreadAligned,
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

void BraveSyncServiceImpl::TriggerOnLogMessage(const std::string &message) {
  for (auto& observer : observers_)
    observer.OnLogMessage(this, message);
}

void BraveSyncServiceImpl::TriggerOnSyncStateChanged() {
  for (auto& observer : observers_)
    observer.OnSyncStateChanged(this);
}

void BraveSyncServiceImpl::TriggerOnHaveSyncWords(const std::string &sync_words) {
  for (auto& observer : observers_)
    observer.OnHaveSyncWords(this, sync_words);
}

void BraveSyncServiceImpl::OnExtensionSystemReady() {
  DLOG(INFO) << "[Brave Sync] " << __func__;
  // observe changes in extension system
  extension_registry_observer_.Add(ExtensionRegistry::Get(profile_));
};

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
