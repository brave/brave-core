/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "brave/components/brave_sync/client/client_web_ui_impl.h"
#include "base/logging.h"
#include "brave/browser/ui/webui/sync/sync_js_layer.h"

namespace brave_sync {

BraveSyncClientWebUiImpl::BraveSyncClientWebUiImpl() : handler_(nullptr), sync_js_layer_(nullptr) {
  LOG(ERROR) << "TAGAB BraveSyncClientWebUiImpl::BraveSyncClientWebUiImpl CTOR";
}

void BraveSyncClientWebUiImpl::SetSyncToBrowserHandler(SyncLibToBrowserHandler *handler) {
  DCHECK(handler);
  DCHECK(!handler_);
  handler_ = handler;
}

void BraveSyncClientWebUiImpl::LoadClient() {
  LOG(ERROR) << "TAGAB BraveSyncClientWebUiImpl::LoadClient";
  DCHECK(sync_js_layer_);
  sync_js_layer_->LoadJsLibScript();
}

void BraveSyncClientWebUiImpl::SendBrowserToSync(
    const std::string &message,
    const base::Value &arg1,
    const base::Value &arg2,
    const base::Value &arg3,
    const base::Value &arg4) {
  ;
}

void BraveSyncClientWebUiImpl::SendGotInitDataStr(const std::string &seed, const std::string &device_id, const std::string & config) {
  ;
}

void BraveSyncClientWebUiImpl::SendFetchSyncRecords(
  const std::vector<std::string> &category_names, const base::Time &startAt,
  const int &max_records) {
  ;
}

void BraveSyncClientWebUiImpl::SendFetchSyncDevices() {
  ;
}

void BraveSyncClientWebUiImpl::SendResolveSyncRecords(const std::string &category_name,
  const SyncRecordAndExistingList &records_and_existing_objects) {
  ;
}

void BraveSyncClientWebUiImpl::SendSyncRecords(const std::string &category_name,
  const RecordsList &records) {
  ;
}

void BraveSyncClientWebUiImpl::SendDeleteSyncUser() {
  ;
}

void BraveSyncClientWebUiImpl::SendDeleteSyncCategory(const std::string &category_name) {
  ;
}

void BraveSyncClientWebUiImpl::SendGetBookmarksBaseOrder(const std::string &device_id, const std::string &platform) {
  ;
}

void BraveSyncClientWebUiImpl::SendGetBookmarkOrder(const std::string &prevOrder, const std::string &nextOrder) {
  ;
}

// Temporary from SyncJsLayer
void BraveSyncClientWebUiImpl::SetupJsLayer(SyncJsLayer *sync_js_layer) {
  LOG(ERROR) << "TAGAB BraveSyncClientWebUiImpl::SetupJsLayer sync_js_layer=" << sync_js_layer;
  LOG(ERROR) << "TAGAB BraveSyncClientWebUiImpl::SetupJsLayer this->sync_js_layer_=" << this->sync_js_layer_;
  DCHECK(sync_js_layer);
  DCHECK(sync_js_layer_ == nullptr);

  sync_js_layer_ = sync_js_layer;
}

void BraveSyncClientWebUiImpl::RunCommandBV(const std::vector<const base::Value*> &args) {
  sync_js_layer_->RunCommandBV(args);
}

void BraveSyncClientWebUiImpl::RunCommandStr(const std::string &command,
  const std::string &arg1, const std::string &arg2, const std::string &arg3,
  const std::string &arg4) {

  DCHECK(nullptr != sync_js_layer_);
  if (!sync_js_layer_) {
    return;
  }

  sync_js_layer_->RunCommandStr(command, arg1, arg2, arg3, arg4);
}

} // namespace brave_sync
