/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SKUS_SERVICE_SANDBOX_TYPE_H_
#define BRAVE_COMPONENTS_SKUS_SERVICE_SANDBOX_TYPE_H_

#include "content/public/browser/service_process_host.h"

// namespace skus_sdk_caller {
// namespace mojom {
// class SkusSdkCaller;
// }  // namespace mojom
// }  // namespace skus_sdk_caller

// template <>
// inline sandbox::policy::SandboxType
// content::GetServiceSandboxType<skus_sdk_caller::mojom::SkusSdkCaller>() {
//   return sandbox::policy::SandboxType::kNoSandbox;
// }


namespace brave_rewards {
class SkusSdkCallerImpl;
}  // namespace brave_rewards

template <>
inline sandbox::policy::SandboxType
content::GetServiceSandboxType<brave_rewards::SkusSdkCallerImpl>() {
  return sandbox::policy::SandboxType::kNoSandbox;
}

#endif  // BRAVE_COMPONENTS_SKUS_SERVICE_SANDBOX_TYPE_H_
