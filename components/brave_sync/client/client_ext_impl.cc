/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/brave_sync/brave_sync_event_router.h"
#include "brave/components/brave_sync/client/client_ext_impl.h"
#include "base/logging.h"

namespace brave_sync {

ClientExtImpl::ClientExtImpl() : handler_(nullptr) {
  ;
}

ClientExtImpl::~ClientExtImpl() {
  ;
}

void ClientExtImpl::SetProfile(Profile *profile) {
  DCHECK(profile);
  DCHECK(!brave_sync_event_router_);

  if (!brave_sync_event_router_) {
    brave_sync_event_router_ = std::make_unique<extensions::BraveSyncEventRouter>(profile);
  }
}

void ClientExtImpl::SetSyncToBrowserHandler(SyncLibToBrowserHandler *handler) {
  DCHECK(handler);
  DCHECK(!handler_);
  handler_ = handler;
}

void ClientExtImpl::LoadClient() {
  ;
}

void ClientExtImpl::SendBrowserToSync(
  const std::string &message,
  const base::Value &arg1,
  const base::Value &arg2,
  const base::Value &arg3,
  const base::Value &arg4) {
  DCHECK(!brave_sync_event_router_);

  brave_sync_event_router_->BrowserToBackgroundPageRaw(message, arg1, arg2, arg3, arg4);
}

void ClientExtImpl::SendGotInitDataStr(const std::string &seed, const std::string &device_id, const std::string & config) {
  ;
}

void ClientExtImpl::SendFetchSyncRecords(
  const std::vector<std::string> &category_names, const base::Time &startAt,
  const int &max_records) {
  ;
}

void ClientExtImpl::SendFetchSyncDevices() {
  ;
}

void ClientExtImpl::SendResolveSyncRecords(const std::string &category_name,
  const SyncRecordAndExistingList &records_and_existing_objects) {
  ;
}

void ClientExtImpl::SendSyncRecords(const std::string &category_name,
  const RecordsList &records) {
  ;
}

void ClientExtImpl::SendDeleteSyncUser()  {
  ;
}

void ClientExtImpl::SendDeleteSyncCategory(const std::string &category_name) {
  ;
}

void ClientExtImpl::SendGetBookmarksBaseOrder(const std::string &device_id, const std::string &platform) {
  ;
}

void ClientExtImpl::SendGetBookmarkOrder(const std::string &prevOrder, const std::string &nextOrder) {
  ;
}

} // namespace brave_sync
