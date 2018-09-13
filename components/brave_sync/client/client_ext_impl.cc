/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/client/client_ext_impl.h"

#include "base/logging.h"
#include "brave/components/brave_sync/api/brave_sync_event_router.h"
#include "brave/components/brave_sync/client/client_ext_impl_data.h"
#include "brave/common/extensions/api/brave_sync.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_thread.h"
#include "extensions/browser/extension_registry.h"

namespace brave_sync {

ClientExtImpl::ClientExtImpl(Profile *profile) : handler_(nullptr),
  profile_(nullptr), startup_complete_(false), set_load_pending_(false) {
  SetProfile(profile);
}

ClientExtImpl::~ClientExtImpl() {
  LOG(ERROR) << "TAGAB ClientExtImpl::~ClientExtImpl";
}

void ClientExtImpl::Shutdown() {
  LOG(ERROR) << "TAGAB ClientExtImpl::Shutdown";
}

void ClientExtImpl::SetProfile(Profile *profile) {
  LOG(ERROR) << "TAGAB ClientExtImpl::SetProfile profile=" << profile;
  DCHECK(profile);
  if (profile_ == profile) {
    LOG(WARNING) << "TAGAB ClientExtImpl::SetProfile profile already set";
    DCHECK(false);
    return;
  }

  DCHECK(!brave_sync_event_router_);
  DCHECK(!profile_);

  if (!brave_sync_event_router_) {
    brave_sync_event_router_ = std::make_unique<extensions::BraveSyncEventRouter>(profile);
  }

  if (!profile_) {
    profile_ = profile;
  }
}

void ClientExtImpl::ExtensionStartupComplete() {
  LOG(WARNING) << "TAGAB ClientExtImpl::ExtensionStartupComplete";
  DCHECK(!startup_complete_);
  startup_complete_ = true;
  if (set_load_pending_) {
    LOG(WARNING) << "TAGAB ClientExtImpl::ExtensionStartupComplete loading pending";
    brave_sync_event_router_->LoadClient();
  }
}

void ClientExtImpl::SetSyncToBrowserHandler(SyncLibToBrowserHandler *handler) {
  DCHECK(handler);
  DCHECK(!handler_);
  handler_ = handler;
}

SyncLibToBrowserHandler *ClientExtImpl::GetSyncToBrowserHandler() {
  DCHECK(handler_);
  return handler_;
}

void ClientExtImpl::LoadClient() {
  LOG(ERROR) << "TAGAB ClientExtImpl::LoadClient";
  LOG(ERROR) << "TAGAB ClientExtImpl::LoadClient WILL DO LOAD";
  if (startup_complete_) {
    LOG(ERROR) << "TAGAB ClientExtImpl::LoadClient WILL DO LOAD RIGHT HERE";
    brave_sync_event_router_->LoadClient();
  } else {
    LOG(ERROR) << "TAGAB ClientExtImpl::LoadClient WILL DO LOAD PENDING";
    DCHECK(!set_load_pending_);
    set_load_pending_ = true;
    // Not happy with this, but extensions::ExtensionRegistryObserver approach does not work
  }
}

void ClientExtImpl::SendGotInitData(const Uint8Array &seed, const Uint8Array &device_id,
  const client_data::Config &config) {
  extensions::api::brave_sync::Config config_extension;
  ConvertConfig(config, config_extension);
  brave_sync_event_router_->GotInitData(seed, device_id, config_extension);
}

void ClientExtImpl::SendFetchSyncRecords(
  const std::vector<std::string> &category_names, const base::Time &startAt,
  const int &max_records) {
  brave_sync_event_router_->FetchSyncRecords(category_names, startAt, max_records);
}

void ClientExtImpl::SendFetchSyncDevices() {
  NOTIMPLEMENTED();
}

void ClientExtImpl::SendResolveSyncRecords(const std::string &category_name,
  const SyncRecordAndExistingList &records_and_existing_objects) {

  std::vector<extensions::api::brave_sync::RecordAndExistingObject> records_and_existing_objects_ext;

  ConvertResolvedPairs(records_and_existing_objects, records_and_existing_objects_ext);

  brave_sync_event_router_->ResolveSyncRecords(category_name,
    records_and_existing_objects_ext);
}

void ClientExtImpl::SendSyncRecords(const std::string &category_name,
  const RecordsList &records) {

  std::vector<extensions::api::brave_sync::SyncRecord2> records_ext;
  ConvertSyncRecordsFromLibToExt(records, records_ext);

  brave_sync_event_router_->SendSyncRecords(category_name, records_ext);
}

void ClientExtImpl::SendDeleteSyncUser()  {
  NOTIMPLEMENTED();
}

void ClientExtImpl::SendDeleteSyncCategory(const std::string &category_name) {
  NOTIMPLEMENTED();
}

void ClientExtImpl::SendGetBookmarksBaseOrder(const std::string &device_id, const std::string &platform) {
  NOTIMPLEMENTED();
}

void ClientExtImpl::SendGetBookmarkOrder(const std::string &prevOrder, const std::string &nextOrder) {
  NOTIMPLEMENTED();
}

void ClientExtImpl::NeedSyncWords(const std::string &seed) {
  brave_sync_event_router_->NeedSyncWords(seed);
}

void ClientExtImpl::NeedBytesFromSyncWords(const std::string &words) {
  brave_sync_event_router_->NeedBytesFromSyncWords(words);
}

} // namespace brave_sync
