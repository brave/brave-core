// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_SKUS_BROWSER_SKUS_SDK_CONTEXT_IMPL_H_
#define BRAVE_COMPONENTS_SKUS_BROWSER_SKUS_SDK_CONTEXT_IMPL_H_

#include <memory>
#include <string>

#include "base/memory/scoped_refptr.h"
#include "brave/components/skus/browser/rs/cxx/src/shim.h"

class PrefService;

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace user_prefs {
class PrefRegistrySyncable;
}  // namespace user_prefs

namespace skus {
class SkusSdkFetcher;
}  // namespace skus

namespace skus {

// Context object used with the SKU SDK to provide 1) key/value pair storage
// and 2) the fetcher used for contacting the SKU SDK endpoint via HTTPS.
//
// In the .cc, there are implementations for global methods originally defined
// in `rs/cxx/src/shim.h`. These implementations are called from
// Rust and will pass this context object along, so that the results can be
// persisted.
class SkusSdkContextImpl : public SkusSdkContext {
 public:
  // TODO(bridiver) - this is not the right place for this
  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

  SkusSdkContextImpl(const SkusSdkContextImpl&) = delete;
  SkusSdkContextImpl& operator=(const SkusSdkContextImpl&) = delete;

  explicit SkusSdkContextImpl(
      PrefService* prefs,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~SkusSdkContextImpl() override;

  std::unique_ptr<skus::SkusSdkFetcher> CreateFetcher() const override;
  std::string GetValueFromStore(std::string key) const override;
  void PurgeStore() const override;
  void UpdateStoreValue(std::string key, std::string value) const override;

 private:
  // used to store the credential
  PrefService* prefs_;

  // used for making requests to SKU server
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
};

}  // namespace skus

#endif  // BRAVE_COMPONENTS_SKUS_BROWSER_SKUS_SDK_CONTEXT_IMPL_H_
