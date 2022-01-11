
/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_TOOLS_REDIRECT_CC_OS_UTILS_H_
#define BRAVE_TOOLS_REDIRECT_CC_OS_UTILS_H_

#include <vector>

#include "brave/tools/redirect_cc/types.h"

namespace os_utils {

int LaunchProcessAndWaitForExitCode(const std::vector<FilePathString>& argv);
bool PathExists(FilePathStringView path);
bool GetEnvVar(FilePathStringView variable_name, FilePathString* result);

}  // namespace os_utils

#endif  // BRAVE_TOOLS_REDIRECT_CC_OS_UTILS_H_
