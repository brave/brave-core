/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_IMPORTER_IMPORTER_LOCK_DIALOG_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_IMPORTER_IMPORTER_LOCK_DIALOG_H_

#define ShowImportLockDialog(...)    \
  ShowImportLockDialog(__VA_ARGS__); \
  void ShowImportLockDialog(gfx::NativeView parent_view, __VA_ARGS__)

#include "src/chrome/browser/importer/importer_lock_dialog.h"  // IWYU pragma: export

#undef ShowImportLockDialog

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_IMPORTER_IMPORTER_LOCK_DIALOG_H_
