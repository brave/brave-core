/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/functional/callback.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/browser/updater/buildflags.h"

#if BUILDFLAG(ENABLE_OMAHA4)
#include "brave/browser/updater/features.h"
#endif

#define DoPeriodicTasks DoPeriodicTasks_ChromiumImpl

#include "src/chrome/browser/updater/scheduler_mac.cc"

#undef DoPeriodicTasks

namespace updater {

void DoPeriodicTasks(base::OnceClosure callback) {
  bool need_to_run_callback = true;
#if BUILDFLAG(ENABLE_OMAHA4)
  if (brave_updater::ShouldUseOmaha4()) {
    DoPeriodicTasks_ChromiumImpl(std::move(callback));
    need_to_run_callback = false;
  }
#endif  // BUILDFLAG(ENABLE_OMAHA4)
  if (need_to_run_callback) {
    std::move(callback).Run();
  }
}

}  // namespace updater
