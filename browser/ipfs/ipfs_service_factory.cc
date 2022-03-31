/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ipfs/ipfs_service_factory.h"

#include <memory>
#include <utility>

#include "base/path_service.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/ipfs/ipfs_blob_context_getter_factory.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/components/ipfs/ipfs_service.h"
#include "brave/components/ipfs/ipfs_utils.h"
#include "chrome/common/channel_info.h"
#include "chrome/common/chrome_paths.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/storage_partition.h"
#include "extensions/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "base/metrics/histogram_macros.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_registry_factory.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/extension_system_provider.h"
#include "extensions/browser/extensions_browser_client.h"
#endif

namespace {
#if BUILDFLAG(ENABLE_EXTENSIONS)
// IPFS companion installed?
// i) No, ii) Yes
void RecordIPFSCompanionInstalled(extensions::ExtensionRegistry* registry) {
  const char ipfs_companion_extension_id[] = "nibjojkomfdiaoajekhjakgkdhaomnch";
  DCHECK(registry);
  bool installed =
      registry->enabled_extensions().Contains(ipfs_companion_extension_id);
  UMA_HISTOGRAM_BOOLEAN("Brave.IPFS.IPFSCompanionInstalled", installed);
}
#endif
}  // namespace

namespace ipfs {

// static
IpfsServiceFactory* IpfsServiceFactory::GetInstance() {
  return base::Singleton<IpfsServiceFactory>::get();
}

// static
IpfsService* IpfsServiceFactory::GetForContext(
    content::BrowserContext* context) {
  if (!IpfsServiceFactory::IsIpfsEnabled(context))
    return nullptr;

  return static_cast<IpfsService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

IpfsServiceFactory::IpfsServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "IpfsService",
          BrowserContextDependencyManager::GetInstance()) {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  DependsOn(extensions::ExtensionRegistryFactory::GetInstance());
  DependsOn(
      extensions::ExtensionsBrowserClient::Get()->GetExtensionSystemFactory());
#endif
}

IpfsServiceFactory::~IpfsServiceFactory() {}

KeyedService* IpfsServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  base::FilePath user_data_dir;
  base::PathService::Get(chrome::DIR_USER_DATA, &user_data_dir);
  auto url_loader = context->GetDefaultStoragePartition()
                        ->GetURLLoaderFactoryForBrowserProcess();
  auto context_getter = std::make_unique<IpfsBlobContextGetterFactory>(context);
  auto* ipfs_updater = g_brave_browser_process
                           ? g_brave_browser_process->ipfs_client_updater()
                           : nullptr;
#if BUILDFLAG(ENABLE_EXTENSIONS)
  RecordIPFSCompanionInstalled(extensions::ExtensionRegistry::Get(context));
#endif
  return new IpfsService(user_prefs::UserPrefs::Get(context),
                         std::move(url_loader), std::move(context_getter),
                         ipfs_updater, user_data_dir, chrome::GetChannel());
}

// static
bool IpfsServiceFactory::IsIpfsEnabled(content::BrowserContext* context) {
  auto* prefs = user_prefs::UserPrefs::Get(context);
  return (brave::IsRegularProfile(context) &&
          !IsIpfsDisabledByFeatureOrPolicy(prefs));
}

}  // namespace ipfs
