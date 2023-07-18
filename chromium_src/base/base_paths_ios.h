/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_BASE_BASE_PATHS_IOS_H_
#define BRAVE_CHROMIUM_SRC_BASE_BASE_PATHS_IOS_H_

#include "src/base/base_paths_ios.h"

namespace base {

class FilePath;
bool GetModuleDir(base::FilePath* path);

}  // namespace base

#endif  // BRAVE_CHROMIUM_SRC_BASE_BASE_PATHS_IOS_H_
