/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/app/v2/agent/browser_identity.h"

#include <utility>

#include "base/auto_reset.h"
#include "base/check.h"
#include "base/check_is_test.h"
#include "base/functional/callback.h"
#include "base/location.h"
#include "base/memory/scoped_refptr.h"
#include "base/no_destructor.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"

namespace brave_vpn::v2 {
namespace {
BrowserIdentityCaptureCallback& GetBrowserIdentityCaptureCallbackInstance() {
  static base::NoDestructor<BrowserIdentityCaptureCallback> instance;
  return *instance;
}
}  // namespace

BrowserIdentity::BrowserIdentity(base::ProcessId pid) : pid_(pid) {}

BrowserIdentity::~BrowserIdentity() = default;

void BrowserIdentity::Verify(VerificationResponseCallback callback) const {
  CHECK(callback);
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
       base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
      BindVerificationRequest(), std::move(callback));
}

// static
scoped_refptr<BrowserIdentity> BrowserIdentity::Create(
    const named_mojo_ipc_server::ConnectionInfo& info) {
  // In tests, the identity capture callback is used to capture the identity.
  if (!GetBrowserIdentityCaptureCallbackInstance().is_null()) {
    CHECK_IS_TEST();
    return GetBrowserIdentityCaptureCallbackInstance().Run(info);
  }
  return Capture(info);
}

base::AutoReset<BrowserIdentityCaptureCallback>
SetBrowserIdentityCaptureCallbackForTesting(  // IN-TEST
    BrowserIdentityCaptureCallback callback) {
  CHECK_IS_TEST();
  return base::AutoReset<BrowserIdentityCaptureCallback>(
      &GetBrowserIdentityCaptureCallbackInstance(), std::move(callback));
}

}  // namespace brave_vpn::v2
