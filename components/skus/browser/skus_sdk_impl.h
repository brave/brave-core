// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_SKUS_SKUS_
#define BRAVE_COMPONENTS_SKUS_SKUS_

#include "brave/components/skus/common/skus_sdk.mojom.h"

class PrefService;

namespace brave_rewards {

class SkusSdkImpl final : public skus::mojom::SkusSdk {
 public:
  SkusSdkImpl(const SkusSdkImpl&) = delete;
  SkusSdkImpl& operator=(const SkusSdkImpl&) = delete;

  SkusSdkImpl(PrefService* prefs);

  void RefreshOrder(uint32_t order_id, RefreshOrderCallback callback) override;
  void FetchOrderCredentials(uint32_t order_id) override;
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_SKUS_SKUS_
