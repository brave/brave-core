/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/client/brave_sync_client_ext_impl.h"
#include "base/logging.h"

namespace brave_sync {

BraveSyncClientExtImpl::BraveSyncClientExtImpl() : handler_(nullptr) {
  ;
}

void BraveSyncClientExtImpl::SetSyncToBrowserHandler(SyncLibToBrowserHandler *handler) {
  DCHECK(handler);
  DCHECK(!handler_);
  handler_ = handler;
}

void BraveSyncClientExtImpl::LoadClient() {
  ;
}

void BraveSyncClientExtImpl::SendBrowserToSync(const std::string &command, const std::string &arg1) {
  ;
}

void BraveSyncClientExtImpl::SendGotInitDataStr(const std::string &seed, const std::string &device_id, const std::string & config) {
  ;
}

void BraveSyncClientExtImpl::SendFetchSyncRecords(
  const std::vector<std::string> &category_names, const base::Time &startAt,
  const int &max_records) {
  ;
}

void BraveSyncClientExtImpl::SendFetchSyncDevices() {
  ;
}

void BraveSyncClientExtImpl::SendResolveSyncRecords(const std::string &category_name,
  const SyncRecordAndExistingList &records_and_existing_objects) {
  ;
}

void BraveSyncClientExtImpl::SendSyncRecords(const std::string &category_name,
  const RecordsList &records) {
  ;
}

void BraveSyncClientExtImpl::SendDeleteSyncUser()  {
  ;
}

void BraveSyncClientExtImpl::SendDeleteSyncCategory(const std::string &category_name) {
  ;
}

void BraveSyncClientExtImpl::SendGetBookmarksBaseOrder(const std::string &device_id, const std::string &platform) {
  ;
}

void BraveSyncClientExtImpl::SendGetBookmarkOrder(const std::string &prevOrder, const std::string &nextOrder) {
  ;
}

} // namespace brave_sync
