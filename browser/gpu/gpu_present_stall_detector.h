/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_GPU_GPU_PRESENT_STALL_DETECTOR_H_
#define BRAVE_BROWSER_GPU_GPU_PRESENT_STALL_DETECTOR_H_

#include "base/time/time.h"

namespace brave {

// Inputs for deciding whether a compositor present stall should restart the
// GPU process. `unacked_since` is the first commit that has not been followed
// by a present; it is null when presents are keeping up.
struct GpuPresentStallInput {
  bool compositor_visible = false;
  base::TimeTicks now;
  base::TimeTicks unacked_since;
  base::TimeTicks last_restart;
};

// Returns true when a visible compositor has been waiting for a present longer
// than `stall_timeout`, and a GPU restart is not still in cooldown.
bool ShouldRestartGpuProcessForPresentStall(const GpuPresentStallInput& input,
                                            base::TimeDelta stall_timeout,
                                            base::TimeDelta cooldown);

}  // namespace brave

#endif  // BRAVE_BROWSER_GPU_GPU_PRESENT_STALL_DETECTOR_H_
