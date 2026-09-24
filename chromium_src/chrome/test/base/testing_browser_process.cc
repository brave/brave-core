/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/test/base/testing_browser_process.h"

#include "brave/test/base/testing_brave_browser_process.h"

// These overrides are used to make sure a TestingBraveBrowserProcess instance
// has a its life time matching the one for the global instance of
// `TestingBraveBrowserProcess`. Therefore, we override all the methods involved
// with creating and destroying the upstream instance.
#define BRAVE_TESTING_BROWSER_PROCESS_CREATE_INSTANCE \
  TestingBraveBrowserProcess::CreateInstance();
#define BRAVE_TESTING_BROWSER_PROCESS_DELETE_INSTANCE \
  TestingBraveBrowserProcess::DeleteInstance();
#define BRAVE_TESTING_BROWSER_PROCESS_TEAR_DOWN_AND_DELETE_INSTANCE \
  TestingBraveBrowserProcess::TearDownAndDeleteInstance();

#include <chrome/test/base/testing_browser_process.cc>

#undef BRAVE_TESTING_BROWSER_PROCESS_CREATE_INSTANCE
#undef BRAVE_TESTING_BROWSER_PROCESS_DELETE_INSTANCE
#undef BRAVE_TESTING_BROWSER_PROCESS_TEAR_DOWN_AND_DELETE_INSTANCE
