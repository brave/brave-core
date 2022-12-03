/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/download/public/common/download_task_runner.h"

#include "base/check_is_test.h"

#include "src/components/download/internal/common/download_task_runner.cc"

namespace download {

void ClearIOTaskRunnerForTesting() {
  CHECK_IS_TEST();
  g_io_task_runner.Get() = nullptr;
}

}  // namespace download
