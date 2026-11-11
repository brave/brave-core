/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/v2/agent/test/fake_agent_identity.h"

#include <memory>
#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/location.h"
#include "base/process/process.h"
#include "base/process/process_handle.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/components/brave_vpn/browser/v2/agent/agent_identity.h"
#include "build/build_config.h"
#include "mojo/public/cpp/platform/platform_handle.h"

namespace brave_vpn::v2 {

FakeAgentIdentity::FakeAgentIdentity(AgentIdentity::VerificationResult result)
#if BUILDFLAG(IS_WIN)
    : AgentIdentity(base::Process{}),
#elif BUILDFLAG(IS_LINUX)
    : AgentIdentity(base::kNullProcessId, base::Time{}),
#elif BUILDFLAG(IS_MAC)
    : AgentIdentity(mojo::PlatformHandle{}, mojo::PlatformHandle{}),
#endif
      result_(result) {
}

FakeAgentIdentity::~FakeAgentIdentity() = default;

void FakeAgentIdentity::Verify(VerificationResponseCallback callback) {
  CHECK(callback);
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), result_));
}

}  // namespace brave_vpn::v2
