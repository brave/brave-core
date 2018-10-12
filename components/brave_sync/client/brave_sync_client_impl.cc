/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/client/brave_sync_client_impl.h"

#include "base/logging.h"
#include "brave/components/brave_sync/api/brave_sync_event_router.h"
#include "brave/components/brave_sync/client/client_ext_impl_data.h"
#include "brave/components/brave_sync/grit/brave_sync_resources.h"
#include "brave/components/brave_sync/pref_names.h"
#include "brave/components/brave_sync/profile_prefs.h"
#include "brave/common/extensions/api/brave_sync.h"
#include "brave/common/extensions/extension_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/extensions/component_loader.h"
#include "chrome/browser/extensions/extension_service.h"
#include "content/public/browser/browser_thread.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/common/one_shot_event.h"

namespace brave_sync {

BraveSyncClientImpl::BraveSyncClientImpl(Profile* profile) :
    extension_loaded_(false),
    handler_(nullptr),
    brave_sync_event_router_(new extensions::BraveSyncEventRouter(profile)),
    profile_(profile),
    extension_registry_observer_(this) {
  DLOG(INFO) << "[Brave Sync] " << __func__;
  DCHECK(profile_);

  sync_prefs_ = std::make_unique<brave_sync::prefs::Prefs>(profile);

  // Handle when the extension system is ready
  extensions::ExtensionSystem::Get(profile)->ready().Post(
      FROM_HERE, base::Bind(&BraveSyncClientImpl::OnExtensionSystemReady,
          base::Unretained(this)));
  sync_this_device_enabled_.Init(
      prefs::kSyncThisDeviceEnabled,
      profile->GetPrefs(),
      base::Bind(&BraveSyncClientImpl::OnPreferenceChanged,
                 base::Unretained(this)));
}

BraveSyncClientImpl::~BraveSyncClientImpl() {
  LOG(ERROR) << "TAGAB BraveSyncClientImpl::~BraveSyncClientImpl";
}

void BraveSyncClientImpl::SetSyncToBrowserHandler(SyncLibToBrowserHandler *handler) {
  DCHECK(handler);
  DCHECK(!handler_);
  handler_ = handler;
}

SyncLibToBrowserHandler *BraveSyncClientImpl::GetSyncToBrowserHandler() {
  DCHECK(handler_);
  return handler_;
}

void BraveSyncClientImpl::SendGotInitData(const Uint8Array& seed,
                                          const Uint8Array& device_id,
                                          const client_data::Config& config,
                                          const std::string& sync_words) {
  extensions::api::brave_sync::Config config_extension;
  ConvertConfig(config, config_extension);
  brave_sync_event_router_->GotInitData(seed, device_id, config_extension,
                                        sync_words);
}

void BraveSyncClientImpl::SendFetchSyncRecords(
  const std::vector<std::string> &category_names, const base::Time &startAt,
  const int &max_records) {
  brave_sync_event_router_->FetchSyncRecords(category_names, startAt, max_records);
}

void BraveSyncClientImpl::SendFetchSyncDevices() {
  NOTIMPLEMENTED();
}

void BraveSyncClientImpl::SendResolveSyncRecords(
    const std::string &category_name,
    std::unique_ptr<SyncRecordAndExistingList> records_and_existing_objects) {

  std::vector<extensions::api::brave_sync::RecordAndExistingObject> records_and_existing_objects_ext;

  ConvertResolvedPairs(*records_and_existing_objects, records_and_existing_objects_ext);

  brave_sync_event_router_->ResolveSyncRecords(category_name,
    records_and_existing_objects_ext);
}

void BraveSyncClientImpl::SendSyncRecords(const std::string &category_name,
  const RecordsList &records) {

  std::vector<extensions::api::brave_sync::SyncRecord> records_ext;
  ConvertSyncRecordsFromLibToExt(records, records_ext);

  brave_sync_event_router_->SendSyncRecords(category_name, records_ext);
}

void BraveSyncClientImpl::SendDeleteSyncUser()  {
  NOTIMPLEMENTED();
}

void BraveSyncClientImpl::SendDeleteSyncCategory(const std::string &category_name) {
  NOTIMPLEMENTED();
}

void BraveSyncClientImpl::SendGetBookmarksBaseOrder(const std::string &device_id, const std::string &platform) {
  LOG(ERROR) << "TAGAB BraveSyncClientImpl::SendGetBookmarksBaseOrder: device_id="<<device_id<<" platform="<<platform;
  brave_sync_event_router_->SendGetBookmarksBaseOrder(device_id, platform);
}

void BraveSyncClientImpl::SendGetBookmarkOrder(const std::string& prev_order,
                                               const std::string& next_order,
                                               const std::string& parent_order) {
  LOG(ERROR) << "TAGAB BraveSyncClientImpl::SendGetBookmarkOrder: prev_order="<<prev_order<<" next_order="<<next_order;
  brave_sync_event_router_->SendGetBookmarkOrder(
      prev_order, next_order, parent_order);
}

void BraveSyncClientImpl::NeedSyncWords(const std::string &seed) {
  brave_sync_event_router_->NeedSyncWords(seed);
}

void BraveSyncClientImpl::OnExtensionInitialized() {
  DLOG(INFO) << "[Brave Sync] " << __func__;
  DCHECK(extension_loaded_);
  if (extension_loaded_)
    brave_sync_event_router_->LoadClient();
}

void BraveSyncClientImpl::Shutdown() {
  DLOG(INFO) << "[Brave Sync] " << __func__;
  LoadOrUnloadExtension(false);
}

void BraveSyncClientImpl::OnExtensionLoaded(
    content::BrowserContext* browser_context,
    const extensions::Extension* extension) {
  if (extension->id() == brave_sync_extension_id) {
    DLOG(INFO) << "[Brave Sync] " << __func__;
    extension_loaded_ = true;
  }
}

void BraveSyncClientImpl::OnExtensionUnloaded(
    content::BrowserContext* browser_context,
    const extensions::Extension* extension,
    extensions::UnloadedExtensionReason reason) {
  if (extension->id() == brave_sync_extension_id) {
    DLOG(INFO) << "[Brave Sync] " << __func__;
    extension_loaded_ = false;
  }
}

void BraveSyncClientImpl::LoadOrUnloadExtension(bool load) {
  DLOG(INFO) << "[Brave Sync] " << __func__ << ":" << load;
  base::FilePath brave_sync_extension_path(FILE_PATH_LITERAL(""));
  brave_sync_extension_path =
      brave_sync_extension_path.Append(FILE_PATH_LITERAL("brave_sync"));
  extensions::ExtensionSystem* system =
    extensions::ExtensionSystem::Get(profile_);
  extensions::ComponentLoader* component_loader =
    system->extension_service()->component_loader();
  if (load) {
    component_loader->Add(IDR_BRAVE_SYNC_EXTENSION, brave_sync_extension_path);
  } else {
    // Remove by root path doesn't have effect, using extension id instead
    // component_loader->Remove(brave_sync_extension_path);
    component_loader->Remove(brave_sync_extension_id);
  }
}

void BraveSyncClientImpl::OnExtensionSystemReady() {
  DLOG(INFO) << "[Brave Sync] " << __func__;
  // observe changes in extension system
  extension_registry_observer_.Add(ExtensionRegistry::Get(profile_));
  DCHECK(!extension_loaded_);
  if (sync_prefs_->GetSyncThisDevice()) {
    LoadOrUnloadExtension(true);
  }
};

void BraveSyncClientImpl::OnPreferenceChanged(
    const std::string& pref_name) {
  DCHECK(pref_name == prefs::kSyncThisDeviceEnabled);
  if (sync_prefs_->GetSyncThisDevice()) {
    DLOG(INFO) << "[Brave Sync] " << __func__
      << " kSyncThisDeviceEnabled <= true";
    LoadOrUnloadExtension(true);
  } else {
    DLOG(INFO) << "[Brave Sync] " << __func__
      << " kSyncThisDeviceEnabled <= false";
    LoadOrUnloadExtension(false);
  }
}

} // namespace brave_sync
