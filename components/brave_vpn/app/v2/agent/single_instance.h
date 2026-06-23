/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_SINGLE_INSTANCE_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_SINGLE_INSTANCE_H_

#include <memory>

#include "base/files/file_path.h"

namespace brave_vpn::v2 {

// Enforces that at most one VPN agent runs per OS session.
//
// The agent calls TryAcquire() once at startup and holds the returned owner
// object for its entire lifetime. When the object is destroyed -- or when the
// process exits, is killed, or crashes -- the underlying OS primitive is
// released by the kernel, so there is never any stale state to clean up.
class SingleInstance {
 public:
  SingleInstance() = default;
  SingleInstance(const SingleInstance&) = delete;
  SingleInstance& operator=(const SingleInstance&) = delete;
  virtual ~SingleInstance() = default;

  // Attempts to claim the single-instance slot for the current OS session.
  // |user_data_dir| is the directory in which the lock file is created on
  // platforms that back single-instance with a file (macOS: "agent.lock" is
  // placed directly inside it; the directory is expected to already exist).
  // Platforms that use a kernel-named object (Windows) ignore it.
  //
  // Returns a non-null owner if this process is now the sole instance.
  // The caller must keep it alive for as long as the agent runs.
  // Returns nullptr if another instance already holds the slot, or if the
  // OS primitive could not be created. In both cases the caller must not
  // proceed as the agent. The concrete reason is logged internally.
  static std::unique_ptr<SingleInstance> TryAcquire(
      const base::FilePath& user_data_dir);
};

}  // namespace brave_vpn::v2

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_SINGLE_INSTANCE_H_
