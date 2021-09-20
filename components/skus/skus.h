// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_SKUS_SKUS_
#define BRAVE_COMPONENTS_SKUS_SKUS_

#include "brave/components/skus/service_sandbox_type.h"
#include "brave/components/skus/skus_sdk_caller.mojom.h"

namespace brave_rewards {

class SkusSdkCallerImpl : public skus_sdk_caller::mojom::SkusSdkCaller {
  void StartRefreshOrder(uint32_t order_id) override;
  void StartFetchOrderCredentials(uint32_t order_id) override;


};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_SKUS_SKUS_
