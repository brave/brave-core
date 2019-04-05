/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/client/brave_sync_client_impl.h"

#include <memory>
#include <string>
#include <vector>

#include "base/logging.h"
#include "brave/browser/extensions/api/brave_sync_event_router.h"
#include "brave/components/brave_sync/client/client_ext_impl_data.h"
#include "brave/components/brave_sync/grit/brave_sync_resources.h"
#include "brave/components/brave_sync/brave_sync_prefs.h"
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

BraveSyncClient* brave_sync_client_for_testing_;

// static
void BraveSyncClientImpl::set_for_testing(BraveSyncClient* sync_client) {
  brave_sync_client_for_testing_ = sync_client;
}

// static
BraveSyncClient* BraveSyncClient::Create(
    SyncMessageHandler* handler,
    Profile* profile) {
  if (brave_sync_client_for_testing_)
    return brave_sync_client_for_testing_;

  return new BraveSyncClientImpl(handler, profile);
}

BraveSyncClientImpl::BraveSyncClientImpl(SyncMessageHandler* handler,
                                         Profile* profile) :
    handler_(handler),
    profile_(profile),
    sync_prefs_(new brave_sync::prefs::Prefs(profile->GetPrefs())),
    extension_loaded_(false),
    brave_sync_event_router_(new extensions::BraveSyncEventRouter(profile)),
    extension_registry_observer_(this) {
  // Handle when the extension system is ready
  extensions::ExtensionSystem::Get(profile)->ready().Post(
      FROM_HERE, base::Bind(&BraveSyncClientImpl::OnExtensionSystemReady,
          base::Unretained(this)));
}

BraveSyncClientImpl::~BraveSyncClientImpl() {}

SyncMessageHandler* BraveSyncClientImpl::sync_message_handler() {
  return handler_;
}

void BraveSyncClientImpl::SendGotInitData(const Uint8Array& seed,
                                          const Uint8Array& device_id,
                                          const client_data::Config& config,
                                          const std::string& sync_words) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  extensions::api::brave_sync::Config config_extension;
  ConvertConfig(config, config_extension);
  brave_sync_event_router_->GotInitData(seed, device_id, config_extension,
                                        sync_words);
}

void BraveSyncClientImpl::SendFetchSyncRecords(
    const std::vector<std::string> &category_names,
    const base::Time &startAt,
    const int max_records) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  brave_sync_event_router_->FetchSyncRecords(category_names, startAt,
                                             max_records);
}

void BraveSyncClientImpl::SendFetchSyncDevices() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  brave_sync_event_router_->FetchSyncDevices();
}

void BraveSyncClientImpl::SendResolveSyncRecords(
    const std::string &category_name,
    std::unique_ptr<SyncRecordAndExistingList> records_and_existing_objects) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  std::vector<extensions::api::brave_sync::RecordAndExistingObject>
      records_and_existing_objects_ext;

  ConvertResolvedPairs(*records_and_existing_objects,
                       records_and_existing_objects_ext);

  brave_sync_event_router_->ResolveSyncRecords(category_name,
    records_and_existing_objects_ext);
}

void BraveSyncClientImpl::SendSyncRecords(const std::string &category_name,
                                          const RecordsList &records) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  std::vector<extensions::api::brave_sync::SyncRecord> records_ext;
  ConvertSyncRecordsFromLibToExt(records, records_ext);

  brave_sync_event_router_->SendSyncRecords(category_name, records_ext);
}

void BraveSyncClientImpl::SendDeleteSyncUser()  {
  NOTIMPLEMENTED();
}

void BraveSyncClientImpl::SendDeleteSyncCategory(
    const std::string& category_name) {
  NOTIMPLEMENTED();
}

void BraveSyncClientImpl::SendGetBookmarksBaseOrder(
    const std::string& device_id,
    const std::string& platform) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  brave_sync_event_router_->SendGetBookmarksBaseOrder(device_id, platform);
}

void BraveSyncClientImpl::NeedSyncWords(const std::string &seed) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  brave_sync_event_router_->NeedSyncWords(seed);
}

void BraveSyncClientImpl::OnExtensionInitialized() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(extension_loaded_);
  if (extension_loaded_)
    brave_sync_event_router_->LoadClient();
}

void BraveSyncClientImpl::OnSyncEnabledChanged() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (sync_prefs_->GetSyncEnabled()) {
    LoadOrUnloadExtension(true);
  } else {
    LoadOrUnloadExtension(false);
  }
}

void BraveSyncClientImpl::OnExtensionReady(
    content::BrowserContext* browser_context,
    const extensions::Extension* extension) {
  if (extension->id() == brave_sync_extension_id)
    handler_->BackgroundSyncStarted(true);
}

void BraveSyncClientImpl::OnExtensionLoaded(
    content::BrowserContext* browser_context,
    const extensions::Extension* extension) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (extension->id() == brave_sync_extension_id) {
    extension_loaded_ = true;
  }
}

void BraveSyncClientImpl::OnExtensionUnloaded(
    content::BrowserContext* browser_context,
    const extensions::Extension* extension,
    extensions::UnloadedExtensionReason reason) {
  if (extension->id() == brave_sync_extension_id) {
    extension_loaded_ = false;
    handler_->BackgroundSyncStopped(true);
  }
}

void BraveSyncClientImpl::LoadOrUnloadExtension(bool load) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
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
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  // observe changes in extension system
  extension_registry_observer_.Add(ExtensionRegistry::Get(profile_));
  DCHECK(!extension_loaded_);
  if (sync_prefs_->GetSyncEnabled()) {
    LoadOrUnloadExtension(true);
  }
}

void BraveSyncClientImpl::ClearOrderMap() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  brave_sync_event_router_->ClearOrderMap();
}

}  // namespace brave_sync
