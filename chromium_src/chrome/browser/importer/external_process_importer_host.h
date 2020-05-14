/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_IMPORTER_EXTERNAL_PROCESS_IMPORTER_HOST_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_IMPORTER_EXTERNAL_PROCESS_IMPORTER_HOST_H_

#define BRAVE_EXTERNAL_PROCESS_IMPORTER_HOST_H \
  friend class BraveExternalProcessImporterHost;

#define NotifyImportEnded virtual NotifyImportEnded
#include "../../../../../chrome/browser/importer/external_process_importer_host.h"
#undef BRAVE_EXTERNAL_PROCESS_IMPORTER_HOST_H
#undef NotifyImportEnded

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_IMPORTER_EXTERNAL_PROCESS_IMPORTER_HOST_H_
