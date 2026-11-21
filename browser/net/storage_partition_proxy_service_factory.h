// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_NET_STORAGE_PARTITION_PROXY_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_NET_STORAGE_PARTITION_PROXY_SERVICE_FACTORY_H_

#include <memory>

#include "chrome/browser/profiles/profile_keyed_service_factory.h"

class Profile;

namespace base {
template <typename T>
class NoDestructor;
}

namespace brave {

class StoragePartitionProxyService;

class StoragePartitionProxyServiceFactory : public ProfileKeyedServiceFactory {
 public:
  static StoragePartitionProxyServiceFactory* GetInstance();
  // Returns nullptr for Tor, guest, and system profiles.
  static StoragePartitionProxyService* GetForProfile(Profile* profile);

 private:
  friend base::NoDestructor<StoragePartitionProxyServiceFactory>;

  StoragePartitionProxyServiceFactory();
  ~StoragePartitionProxyServiceFactory() override;

  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

}  // namespace brave

#endif  // BRAVE_BROWSER_NET_STORAGE_PARTITION_PROXY_SERVICE_FACTORY_H_
