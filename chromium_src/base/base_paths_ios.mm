/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/base/base_paths_ios.mm"

#include "base/base_paths_apple.h"

namespace base {

bool GetModuleDir(base::FilePath* path) {
  if (!apple::internal::GetModulePathForAddress(
          path, reinterpret_cast<const void*>(&base::PathProviderIOS))) {
    return false;
  }
  *path = path->DirName();
  return true;
}

}  // namespace base
