/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/v2/agent/agent_identity.h"

#include <utility>

#include "base/check.h"
#include "base/location.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"

namespace brave_vpn::v2 {

AgentIdentity::~AgentIdentity() = default;

mojo::PlatformHandle AgentIdentity::TakeSendHandle() {
  return std::move(send_handle_);
}

void AgentIdentity::Verify(VerificationResponseCallback callback) {
  CHECK(callback);
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
       base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
      BindVerificationRequest(), std::move(callback));
}

}  // namespace brave_vpn::v2
