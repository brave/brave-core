/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_DOWNLOAD_CHROME_DOWNLOAD_MANAGER_DELEGATE_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_DOWNLOAD_CHROME_DOWNLOAD_MANAGER_DELEGATE_H_

// Make IsDownloadReadyForCompletion virtual and friend the Brave subclass so it
// can call the Chromium implementation. This is the completion gate used to run
// post-download work (e.g. image metadata stripping) before the file is
// renamed/quarantined.
#define IsDownloadReadyForCompletion         \
  IsDownloadReadyForCompletionUnused();      \
  friend class BraveDownloadManagerDelegate; \
  virtual bool IsDownloadReadyForCompletion

#include <chrome/browser/download/chrome_download_manager_delegate.h>  // IWYU pragma: export

#undef IsDownloadReadyForCompletion

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_DOWNLOAD_CHROME_DOWNLOAD_MANAGER_DELEGATE_H_
