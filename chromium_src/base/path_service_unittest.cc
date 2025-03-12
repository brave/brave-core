/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// One of the paths this test attempts to verify on Windows is DIR_ONE_DRIVE.
// This path doesn't exist on our CIs. To avoid filtering out the entire test
// let's just skip this path.
#define BRAVE_PATH_SERVICE_TEST_GET \
  if (key == DIR_ONE_DRIVE) {       \
    continue;                       \
  }

#include "src/base/path_service_unittest.cc"
#undef BRAVE_PATH_SERVICE_TEST_GET
