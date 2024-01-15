/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_FILE_SYSTEM_ACCESS_FILE_SYSTEM_ACCESS_HANDLE_BASE_H_
#define BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_FILE_SYSTEM_ACCESS_FILE_SYSTEM_ACCESS_HANDLE_BASE_H_

#define DoMove(...)                 \
  DoMove_ChromiumImpl(__VA_ARGS__); \
  void DoMove(__VA_ARGS__)

#define DoRename(...)                 \
  DoRename_ChromiumImpl(__VA_ARGS__); \
  void DoRename(__VA_ARGS__)

#include "src/content/browser/file_system_access/file_system_access_handle_base.h"  // IWYU pragma: export
#undef DoRename
#undef DoMove

#endif  // BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_FILE_SYSTEM_ACCESS_FILE_SYSTEM_ACCESS_HANDLE_BASE_H_
