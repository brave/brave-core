/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/gpu/gpu_present_stall_detector.h"

namespace brave {

bool ShouldRestartGpuProcessForPresentStall(const GpuPresentStallInput& input,
                                            base::TimeDelta stall_timeout,
                                            base::TimeDelta cooldown) {
  if (!input.compositor_visible) {
    return false;
  }
  if (input.unacked_since.is_null()) {
    return false;
  }
  if (input.now - input.unacked_since < stall_timeout) {
    return false;
  }
  if (!input.last_restart.is_null() &&
      input.now - input.last_restart < cooldown) {
    return false;
  }
  return true;
}

}  // namespace brave
