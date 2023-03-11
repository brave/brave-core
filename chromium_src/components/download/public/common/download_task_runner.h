/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_DOWNLOAD_PUBLIC_COMMON_DOWNLOAD_TASK_RUNNER_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_DOWNLOAD_PUBLIC_COMMON_DOWNLOAD_TASK_RUNNER_H_

#define SetIOTaskRunner          \
  ClearIOTaskRunnerForTesting(); \
  COMPONENTS_DOWNLOAD_EXPORT void SetIOTaskRunner

#include "src/components/download/public/common/download_task_runner.h"  // IWYU pragma: export

#undef SetIOTaskRunner

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_DOWNLOAD_PUBLIC_COMMON_DOWNLOAD_TASK_RUNNER_H_
