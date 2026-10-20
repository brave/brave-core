// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/brave_transfer/brave_transfer_service_factory.h"

#include <utility>

#include "base/functional/bind.h"
#include "brave/browser/brave_transfer/brave_transfer_service_delegate_impl.h"
#include "brave/browser/brave_transfer/mdns_discovery_provider.h"
#include "brave/browser/brave_transfer/sync_trust_source.h"
#include "brave/browser/brave_transfer/tcp_connector.h"
#include "brave/components/brave_transfer/browser/brave_transfer_service.h"
#include "brave/components/brave_transfer/browser/inbox_store.h"
#include "brave/components/brave_transfer/core/features.h"
#include "brave/components/sync/service/brave_sync_service_impl.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/notifications/notification_display_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sync/device_info_sync_service_factory.h"
#include "chrome/browser/sync/sync_service_factory.h"
#include "content/public/browser/storage_partition.h"
#include "services/network/public/mojom/network_context.mojom.h"

namespace brave_transfer {

namespace {

network::mojom::NetworkContext* GetNetworkContext(
    base::WeakPtr<Profile> profile) {
  if (!profile) {
    return nullptr;
  }
  return profile->GetDefaultStoragePartition()->GetNetworkContext();
}

}  // namespace

// static
BraveTransferService* BraveTransferServiceFactory::GetForProfile(
    Profile* profile) {
  return static_cast<BraveTransferService*>(
      GetInstance()->GetServiceForBrowserContext(profile, /*create=*/true));
}

// static
BraveTransferServiceFactory* BraveTransferServiceFactory::GetInstance() {
  static base::NoDestructor<BraveTransferServiceFactory> instance;
  return instance.get();
}

BraveTransferServiceFactory::BraveTransferServiceFactory()
    : ProfileKeyedServiceFactory("BraveTransferService") {
  DependsOn(SyncServiceFactory::GetInstance());
  DependsOn(DeviceInfoSyncServiceFactory::GetInstance());
  DependsOn(NotificationDisplayServiceFactory::GetInstance());
}

BraveTransferServiceFactory::~BraveTransferServiceFactory() = default;

std::unique_ptr<KeyedService>
BraveTransferServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  if (!features::IsBraveTransferEnabled()) {
    return nullptr;
  }
  Profile* profile = Profile::FromBrowserContext(context);

  syncer::BraveSyncServiceImpl* sync_service =
      SyncServiceFactory::IsSyncAllowed(profile)
          ? static_cast<syncer::BraveSyncServiceImpl*>(
                SyncServiceFactory::GetForProfile(profile))
          : nullptr;

  return std::make_unique<BraveTransferService>(
      profile->GetPrefs(),
      profile->GetPath()
          .AppendASCII(InboxStore::kDirectoryName)
          .AppendASCII(InboxStore::kFileName),
      g_browser_process->os_crypt_async(),
      std::make_unique<SyncTrustSource>(
          sync_service, DeviceInfoSyncServiceFactory::GetForProfile(profile)),
      std::make_unique<MdnsDiscoveryProvider>(),
      std::make_unique<TcpConnector>(
          base::BindRepeating(&GetNetworkContext, profile->GetWeakPtr())),
      CreateDownloadsFileSink(profile),
      std::make_unique<BraveTransferServiceDelegateImpl>(profile));
}

}  // namespace brave_transfer
