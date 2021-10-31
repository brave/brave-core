// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_SKUS_BROWSER_SKUS_SDK_SERVICE_H_
#define BRAVE_COMPONENTS_SKUS_BROWSER_SKUS_SDK_SERVICE_H_

#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "brave/components/skus/browser/brave-rewards-cxx/src/wrapper.h"
#include "brave/components/skus/common/skus_sdk.mojom.h"
#include "components/keyed_service/core/keyed_service.h"

class PrefService;

// namespace brave_rewards {
// class SkusSdkContext;
// }  // namespace brave_rewards

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

// TODO(bsclifton): description here
class SkusSdkService : public KeyedService {
 public:
  explicit SkusSdkService(
      PrefService* prefs,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~SkusSdkService() override;

  SkusSdkService(const SkusSdkService&) = delete;
  SkusSdkService& operator=(SkusSdkService&) = delete;

  void RefreshOrder(const std::string& order_id,
                    skus::mojom::SkusSdk::RefreshOrderCallback callback);
  void FetchOrderCredentials(
      const std::string& order_id,
      skus::mojom::SkusSdk::FetchOrderCredentialsCallback callback);
  void PrepareCredentialsPresentation(
      const std::string& domain,
      const std::string& path,
      skus::mojom::SkusSdk::PrepareCredentialsPresentationCallback callback);

 private:
  std::unique_ptr<brave_rewards::SkusSdkContext> context_;
  ::rust::Box<brave_rewards::CppSDK> sdk_;
  base::WeakPtrFactory<SkusSdkService> weak_factory_;
};

#endif  // BRAVE_COMPONENTS_SKUS_BROWSER_SKUS_SDK_SERVICE_H_
