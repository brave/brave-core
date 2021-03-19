/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ipfs/ipfs_service_factory.h"

#include "base/path_service.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/components/ipfs/ipfs_service.h"
#include "brave/components/ipfs/ipfs_utils.h"
#include "chrome/common/channel_info.h"
#include "chrome/common/chrome_paths.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "extensions/browser/extension_registry_factory.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/extension_system_provider.h"
#include "extensions/browser/extensions_browser_client.h"

namespace ipfs {

// static
IpfsServiceFactory* IpfsServiceFactory::GetInstance() {
  return base::Singleton<IpfsServiceFactory>::get();
}

// static
IpfsService* IpfsServiceFactory::GetForContext(
    content::BrowserContext* context) {
  if (!brave::IsRegularProfile(context) || !IsIpfsEnabled(context))
    return nullptr;

  return static_cast<IpfsService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

IpfsServiceFactory::IpfsServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "IpfsService",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(extensions::ExtensionRegistryFactory::GetInstance());
  DependsOn(
      extensions::ExtensionsBrowserClient::Get()->GetExtensionSystemFactory());
}

IpfsServiceFactory::~IpfsServiceFactory() {}

KeyedService* IpfsServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  base::FilePath user_data_dir;
  base::PathService::Get(chrome::DIR_USER_DATA, &user_data_dir);
  return new IpfsService(context,
                         g_brave_browser_process
                             ? g_brave_browser_process->ipfs_client_updater()
                             : nullptr,
                         user_data_dir, chrome::GetChannel());
}

}  // namespace ipfs
