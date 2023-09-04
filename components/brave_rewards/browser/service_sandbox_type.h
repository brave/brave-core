/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_SERVICE_SANDBOX_TYPE_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_SERVICE_SANDBOX_TYPE_H_

#include "build/build_config.h"
#include "content/public/browser/service_process_host.h"

namespace brave_rewards::mojom {
class RewardsEngineFactory;
}  // namespace brave_rewards::mojom

template <>
inline sandbox::mojom::Sandbox
content::GetServiceSandboxType<brave_rewards::mojom::RewardsEngineFactory>() {
#if !BUILDFLAG(IS_ANDROID)
  return sandbox::mojom::Sandbox::kNoSandbox;
#else
  return sandbox::mojom::Sandbox::kUtility;
#endif  // !BUILDFLAG(IS_ANDROID)
}

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_SERVICE_SANDBOX_TYPE_H_
