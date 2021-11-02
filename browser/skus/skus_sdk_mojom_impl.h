// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_SKUS_SKUS_SDK_MOJOM_IMPL_H_
#define BRAVE_BROWSER_SKUS_SKUS_SDK_MOJOM_IMPL_H_

#include <string>

#include "brave/components/skus/common/skus_sdk.mojom.h"

class SkusSdkService;

namespace content {
class BrowserContext;
}  // namespace content

namespace brave_rewards {

class SkusSdkMojomImpl final : public skus::mojom::SkusSdk {
 public:
  SkusSdkMojomImpl(const SkusSdkMojomImpl&) = delete;
  SkusSdkMojomImpl& operator=(const SkusSdkMojomImpl&) = delete;

  SkusSdkMojomImpl(content::BrowserContext* context);
  ~SkusSdkMojomImpl() override;

  void RefreshOrder(const std::string& order_id,
                    RefreshOrderCallback callback) override;
  void FetchOrderCredentials(const std::string& order_id,
                             FetchOrderCredentialsCallback callback) override;
  void PrepareCredentialsPresentation(
      const std::string& domain,
      const std::string& path,
      PrepareCredentialsPresentationCallback callback) override;
  void CredentialSummary(const std::string& domain,
                         CredentialSummaryCallback callback) override;

 private:
  SkusSdkService* service_;
};

}  // namespace brave_rewards

#endif  // BRAVE_BROWSER_SKUS_SKUS_SDK_MOJOM_IMPL_H_
