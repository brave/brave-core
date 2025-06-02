/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_IMPORTER_EXTERNAL_PROCESS_IMPORTER_HOST_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_IMPORTER_EXTERNAL_PROCESS_IMPORTER_HOST_H_

#define NotifyImportEnded(...)                        \
  virtual NotifyImportEnded(__VA_ARGS__);             \
  friend class BraveExternalProcessImporterHost;      \
  void set_parent_view(gfx::NativeView parent_view) { \
    parent_view_ = parent_view;                       \
  }                                                   \
  gfx::NativeView parent_view_ = gfx::NativeView()

#include "src/chrome/browser/importer/external_process_importer_host.h"  // IWYU pragma: export

#undef NotifyImportEnded

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_IMPORTER_EXTERNAL_PROCESS_IMPORTER_HOST_H_
