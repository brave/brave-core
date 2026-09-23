/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/app/v2/agent/single_instance.h"

#include <windows.h>

#include <memory>
#include <utility>

#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/win/scoped_handle.h"

namespace brave_vpn::v2 {
namespace {

// Name of the single-instance mutex.
constexpr wchar_t kAgentMutexName[] = L"Local\\BraveVpnAgentSingleInstance";

class SingleInstanceWin : public SingleInstance {
 public:
  explicit SingleInstanceWin(base::win::ScopedHandle handle)
      : handle_(std::move(handle)) {}

 private:
  base::win::ScopedHandle handle_;
};

}  // namespace

// static
std::unique_ptr<SingleInstance> SingleInstance::TryAcquire(
    const base::FilePath& /*user_data_dir*/) {
  // |user_data_dir| is unused on Windows: single-instance is enforced with a
  // session-local named mutex, not a lock file.
  base::win::ScopedHandle handle(::CreateMutexW(
      /*lpMutexAttributes=*/nullptr, /*bInitialOwner=*/FALSE, kAgentMutexName));
  const DWORD error = ::GetLastError();

  if (!handle.is_valid()) {
    // Could not create the object at all. This is a fatal error, because we
    // cannot guarantee single-instance.
    LOG(ERROR) << "Failed to create single-instance mutex, error=" << error;
    return nullptr;
  }

  if (error == ERROR_ALREADY_EXISTS) {
    // Another agent in this session already owns the name.
    LOG(WARNING) << "Another agent is already running in this session";
    return nullptr;
  }

  return std::make_unique<SingleInstanceWin>(std::move(handle));
}

}  // namespace brave_vpn::v2
