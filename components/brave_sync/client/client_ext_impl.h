/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SYNC_CLIENT_BRAVE_SYNC_CLIENT_EXT_IMPL_H
#define BRAVE_COMPONENTS_BRAVE_SYNC_CLIENT_BRAVE_SYNC_CLIENT_EXT_IMPL_H

#include "brave/components/brave_sync/client/client.h"
#include "base/macros.h"
#include "extensions/browser/extension_registry_observer.h"

class Profile;
namespace extensions {
class BraveSyncEventRouter;
}

namespace brave_sync {

class ClientExtImpl : public BraveSyncClient,
                      public extensions::ExtensionRegistryObserver {
public:
  ClientExtImpl();
  ~ClientExtImpl() override;

  void SetProfile(Profile *profile) override;

  // extensions::ExtensionRegistryObserver implementations.
  void OnExtensionInstalled(content::BrowserContext* browser_context,
                            const extensions::Extension* extension,
                            bool is_update) override;
  void OnExtensionUnloaded(content::BrowserContext* browser_context,
                           const extensions::Extension* extension,
                           extensions::UnloadedExtensionReason reason) override;
  void OnExtensionUninstalled(content::BrowserContext* browser_context,
                              const extensions::Extension* extension,
                              extensions::UninstallReason reason) override;
  void OnExtensionLoaded(content::BrowserContext* browser_context,
                         const extensions::Extension* extension) override;

  // BraveSyncClient overrides

  // BraveSync to Browser messages
  void SetSyncToBrowserHandler(SyncLibToBrowserHandler *handler) override;
  SyncLibToBrowserHandler *GetSyncToBrowserHandler() override;

  // After this call the library gets loaded and
  // sends SyncLibToBrowserHandler::OnGetInitData and so on
  void LoadClient() override;

  // Browser to BraveSync messages
  void SendGotInitData(const Uint8Array &seed, const Uint8Array &device_id,
    const client_data::Config &config) override;
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
  void NeedSyncWords(const std::string &seed) override;
  void NeedBytesFromSyncWords(const std::string &words) override;

private:
  DISALLOW_COPY_AND_ASSIGN(ClientExtImpl);
  SyncLibToBrowserHandler *handler_;
  std::unique_ptr<extensions::BraveSyncEventRouter> brave_sync_event_router_;
  Profile *profile_;
  ///bool loaded_;
};

} // namespace brave_sync

#endif // BRAVE_COMPONENTS_BRAVE_SYNC_CLIENT_BRAVE_SYNC_CLIENT_EXT_IMPL_H
