/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_SHELL_INTEGRATION_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_SHELL_INTEGRATION_H_

#define BRAVE_DEFAULT_BROWSER_WORKER_H friend class BraveDefaultBrowserWorker;

#include <chrome/browser/shell_integration.h>  // IWYU pragma: export

#undef BRAVE_DEFAULT_BROWSER_WORKER_H

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_SHELL_INTEGRATION_H_
