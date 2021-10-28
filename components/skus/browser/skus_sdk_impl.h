// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_SKUS_BROWSER_SKUS_SDK_IMPL_H_
#define BRAVE_COMPONENTS_SKUS_BROWSER_SKUS_SDK_IMPL_H_

#include <string>

#include "brave/components/skus/common/skus_sdk.mojom.h"

class PrefService;

namespace user_prefs {
class PrefRegistrySyncable;
}  // namespace user_prefs

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

namespace user_prefs {
class PrefRegistrySyncable;
}  // namespace user_prefs

namespace brave_rewards {

class SkusSdkFetcher;

class SkusSdkImpl final : public skus::mojom::SkusSdk {
 public:
  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

  SkusSdkImpl(const SkusSdkImpl&) = delete;
  SkusSdkImpl& operator=(const SkusSdkImpl&) = delete;

  SkusSdkImpl(
      PrefService* prefs,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~SkusSdkImpl() override;

  std::unique_ptr<SkusSdkFetcher> CreateFetcher() const;

  std::string GetValueFromStore(std::string key) const;
  void PurgeStore() const;
  void UpdateStoreValue(std::string key, std::string value) const;

  void RefreshOrder(const std::string& order_id,
                    RefreshOrderCallback callback) override;
  void FetchOrderCredentials(const std::string& order_id,
                             FetchOrderCredentialsCallback callback) override;
  void PrepareCredentialsPresentation(
      const std::string& domain,
      const std::string& path,
      PrepareCredentialsPresentationCallback callback) override;

 private:
  // used for making requests to SKU server
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;

  // used to store the credential
  PrefService* prefs_;

  // TODO(bsclifton): REMOVE ME (THIS IS A HACK)
  std::unique_ptr<SkusSdkImpl> unique_instance_;
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_SKUS_BROWSER_SKUS_SDK_IMPL_H_
