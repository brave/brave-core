/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_SERVICE_SANDBOX_TYPE_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_SERVICE_SANDBOX_TYPE_H_

#include "content/public/browser/service_process_host.h"

namespace bat_ledger {
namespace mojom {
class BatLedgerService;
}  // namespace mojom
}  // namespace bat_ledger

template <>
inline sandbox::policy::SandboxType
content::GetServiceSandboxType<bat_ledger::mojom::BatLedgerService>() {
#if !defined(OS_ANDROID)
  return sandbox::policy::SandboxType::kNoSandbox;
#else
  return sandbox::policy::SandboxType::kUtility;
#endif  // !defined(OS_ANDROID)
}

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_SERVICE_SANDBOX_TYPE_H_
