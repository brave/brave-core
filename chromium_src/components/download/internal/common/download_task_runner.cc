/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/download/public/common/download_task_runner.h"

#include "base/check_is_test.h"

#include "src/components/download/internal/common/download_task_runner.cc"

namespace download {

// IOTaskRunner is set on global variable. Once a task runner is set, we can't
// change global IOTaskRunner. But this makes unit tests flaky because tasks
// are posted to wrong runner - typically, tasks could be posted to the runner
// created by previous test, not to what we want to post the tasks.
void ClearIOTaskRunnerForTesting() {
  CHECK_IS_TEST();
  g_io_task_runner.Get() = nullptr;
}

}  // namespace download
