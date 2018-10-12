/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SYNC_BRAVE_SYNC_CLIENT_IMPL_H
#define BRAVE_COMPONENTS_BRAVE_SYNC_BRAVE_SYNC_CLIENT_IMPL_H

#include "brave/components/brave_sync/client/brave_sync_client.h"
#include "base/macros.h"
#include "base/scoped_observer.h"
#include "base/memory/weak_ptr.h"
#include "components/prefs/pref_member.h"
#include "extensions/browser/extension_registry_observer.h"

class Profile;
namespace extensions {
class BraveSyncEventRouter;
class ExtensionRegistry;
}

using extensions::ExtensionRegistryObserver;
using extensions::ExtensionRegistry;

namespace brave_sync {

namespace prefs {
  class Prefs;
}

class BraveSyncClientImpl : public BraveSyncClient,
                            public ExtensionRegistryObserver {
 public:
  BraveSyncClientImpl(Profile *profile);
  ~BraveSyncClientImpl() override;

  void Shutdown() override;

  // BraveSyncClient overrides

  // BraveSync to Browser messages
  void SetSyncToBrowserHandler(SyncLibToBrowserHandler *handler) override;
  SyncLibToBrowserHandler *GetSyncToBrowserHandler() override;

  // Browser to BraveSync messages
  void SendGotInitData(const Uint8Array& seed, const Uint8Array& device_id,
    const client_data::Config& config, const std::string& sync_words) override;
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
  void SendGetBookmarkOrder(const std::string& prev_order,
                            const std::string& next_order,
                            const std::string& parent_order) override;
  void NeedSyncWords(const std::string &seed) override;

 private:
  void OnExtensionInitialized() override;

  // ExtensionRegistryObserver:
  void OnExtensionLoaded(content::BrowserContext* browser_context,
                         const extensions::Extension* extension) override;
  void OnExtensionUnloaded(content::BrowserContext* browser_context,
                           const extensions::Extension* extension,
                           extensions::UnloadedExtensionReason reason) override;
  void LoadOrUnloadExtension(bool load);
  void OnExtensionSystemReady();
  void OnPreferenceChanged(const std::string& pref_name);

  bool extension_loaded_;

  SyncLibToBrowserHandler *handler_;
  std::unique_ptr<extensions::BraveSyncEventRouter> brave_sync_event_router_;
  Profile *profile_;

  std::unique_ptr<brave_sync::prefs::Prefs> sync_prefs_;

  BooleanPrefMember sync_this_device_enabled_;

  ScopedObserver<ExtensionRegistry, ExtensionRegistryObserver>
    extension_registry_observer_;

  base::WeakPtrFactory<BraveSyncClientImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(BraveSyncClientImpl);
};

} // namespace brave_sync

#endif // BRAVE_COMPONENTS_BRAVE_SYNC_BRAVE_SYNC_CLIENT_IMPL_H
