// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_SKUS_BROWSER_SKUS_SDK_CONTEXT_H_
#define BRAVE_COMPONENTS_SKUS_BROWSER_SKUS_SDK_CONTEXT_H_

#include <memory>
#include <string>

#include "base/memory/scoped_refptr.h"

class PrefService;

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace user_prefs {
class PrefRegistrySyncable;
}  // namespace user_prefs

namespace brave_rewards {
class SkusSdkFetcher;
}  // namespace brave_rewards

namespace brave_rewards {

class SkusSdkContext {
 public:
  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

  SkusSdkContext(const SkusSdkContext&) = delete;
  SkusSdkContext& operator=(const SkusSdkContext&) = delete;

  explicit SkusSdkContext(
      PrefService* prefs,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~SkusSdkContext();

  std::unique_ptr<brave_rewards::SkusSdkFetcher> CreateFetcher() const;
  std::string GetValueFromStore(std::string key) const;
  void PurgeStore() const;
  void UpdateStoreValue(std::string key, std::string value) const;

 private:
  // used to store the credential
  PrefService* prefs_;

  // used for making requests to SKU server
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_SKUS_BROWSER_SKUS_SDK_CONTEXT_H_
