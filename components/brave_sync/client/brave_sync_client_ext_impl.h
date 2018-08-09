/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SYNC_CLIENT_BRAVE_SYNC_CLIENT_EXT_IMPL_H
#define BRAVE_COMPONENTS_BRAVE_SYNC_CLIENT_BRAVE_SYNC_CLIENT_EXT_IMPL_H

#include "brave/components/brave_sync/client/brave_sync_client.h"
#include "base/macros.h"

namespace brave_sync {

class BraveSyncClientExtImpl : public BraveSyncClient {
public:
  BraveSyncClientExtImpl();
  ~BraveSyncClientExtImpl() override = default;

  // BraveSyncClient overrides

  // BraveSync to Browser messages
  void SetSyncToBrowserHandler(SyncLibToBrowserHandler *handler) override;

  // After this call the library gets loaded and
  // sends SyncLibToBrowserHandler::OnGetInitData and so on
  void LoadClient() override;

  // Browser to BraveSync messages
  void SendBrowserToSync(const std::string &command, const std::string &arg1) override;

  void SendGotInitDataStr(const std::string &seed, const std::string &device_id, const std::string & config) override;
  void SendFetchSyncRecords(
    const std::vector<std::string> &category_names, const base::Time &startAt,
    const int &max_records) override;
  void SendFetchSyncDevices() override ;
  void SendResolveSyncRecords(const std::string &category_name,
    const SyncRecordAndExistingList &records_and_existing_objects) override;
  void SendSyncRecords(const std::string &category_name,
    const RecordsList &records) override;
  void SendDeleteSyncUser() override;
  void SendDeleteSyncCategory(const std::string &category_name) override;
  void SendGetBookmarksBaseOrder(const std::string &device_id, const std::string &platform) override;
  void SendGetBookmarkOrder(const std::string &prevOrder, const std::string &nextOrder) override;

private:
  DISALLOW_COPY_AND_ASSIGN(BraveSyncClientExtImpl);
  SyncLibToBrowserHandler *handler_;
};

} // namespace brave_sync

#endif // BRAVE_COMPONENTS_BRAVE_SYNC_CLIENT_BRAVE_SYNC_CLIENT_EXT_IMPL_H
