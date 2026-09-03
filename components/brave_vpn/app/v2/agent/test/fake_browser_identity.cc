/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/app/v2/agent/test/fake_browser_identity.h"

#include <string>
#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/location.h"
#include "base/process/process_handle.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/components/brave_vpn/app/v2/agent/browser_identity.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

namespace brave_vpn::v2 {

FakeBrowserIdentity::FakeBrowserIdentity(
    base::ProcessId pid,
    FakeVerificationResultProvider result_provider,
    bool is_same_process)
    : BrowserIdentity(pid),
      verification_result_(std::move(result_provider)),
      is_same_process_(is_same_process) {}

FakeBrowserIdentity::~FakeBrowserIdentity() = default;

std::string FakeBrowserIdentity::GetDescription() const {
  return absl::StrFormat("fake pid=%d", static_cast<int>(pid()));
}

bool FakeBrowserIdentity::IsSameProcess(
    const BrowserIdentity& /*other*/) const {
  return is_same_process_;
}

void FakeBrowserIdentity::Verify(VerificationResponseCallback callback) const {
  CHECK(callback);
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(std::move(callback), verification_result_.Run()));
}

}  // namespace brave_vpn::v2
