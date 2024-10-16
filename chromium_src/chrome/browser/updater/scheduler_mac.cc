/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/functional/callback.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/browser/mac_features.h"

#define DoPeriodicTasks DoPeriodicTasks_ChromiumImpl

#include "src/chrome/browser/updater/scheduler_mac.cc"

#undef DoPeriodicTasks

namespace updater {

void DoPeriodicTasks(base::OnceClosure callback) {
  if (brave::ShouldUseOmaha4()) {
    DoPeriodicTasks_ChromiumImpl(std::move(callback));
  }
}

}  // namespace updater
