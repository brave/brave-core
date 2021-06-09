/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_INSTALLER_UTIL_BRAVE_SHELL_UTIL_H_
#define BRAVE_INSTALLER_UTIL_BRAVE_SHELL_UTIL_H_

#include <string>

namespace installer {

std::wstring GetProgIdForFileType();
bool ShouldUseFileTypeProgId(const std::wstring& ext);

}  // namespace installer

#endif  // BRAVE_INSTALLER_UTIL_BRAVE_SHELL_UTIL_H_
