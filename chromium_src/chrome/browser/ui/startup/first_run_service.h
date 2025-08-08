/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_STARTUP_FIRST_RUN_SERVICE_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_STARTUP_FIRST_RUN_SERVICE_H_

// Make FirstRunServiceTest a friend class, to give it access to
// TryMarkFirstRunAlreadyFinished private method.
#define resume_task_callback_ \
  resume_task_callback_;      \
  friend class FirstRunServiceTest_FinishProfileSetUpShouldNotChangeName_Test

#include <chrome/browser/ui/startup/first_run_service.h>  // IWYU pragma: export
#undef resume_task_callback_

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_STARTUP_FIRST_RUN_SERVICE_H_
