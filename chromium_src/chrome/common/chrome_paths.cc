/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#define RegisterPathProvider RegisterPathProvider_ChromiumImpl
#include "src/chrome/common/chrome_paths.cc"
#undef RegisterPathProvider

#include "brave/components/constants/brave_paths.h"

namespace brave {

bool BravePathProvider(int key, base::FilePath* result) {
  base::FilePath cur;

  switch (key) {
    case DIR_TEST_DATA:
      if (!base::PathService::Get(base::DIR_SRC_TEST_DATA_ROOT, &cur)) {
        return false;
      }
      cur = cur.Append(FILE_PATH_LITERAL("brave"));
      cur = cur.Append(FILE_PATH_LITERAL("test"));
      cur = cur.Append(FILE_PATH_LITERAL("data"));
      if (!base::PathExists(cur)) {  // We don't want to create this.
        return false;
      }
      break;

    default:
      return false;
  }

  *result = cur;
  return true;
}

void RegisterBravePathProvider() {
  base::PathService::RegisterProvider(BravePathProvider, PATH_START, PATH_END);
}

}  // namespace brave

namespace chrome {

void RegisterPathProvider() {
  RegisterPathProvider_ChromiumImpl();
  brave::RegisterBravePathProvider();
}

}  // namespace chrome
