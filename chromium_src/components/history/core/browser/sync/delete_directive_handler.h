/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_HISTORY_CORE_BROWSER_SYNC_DELETE_DIRECTIVE_HANDLER_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_HISTORY_CORE_BROWSER_SYNC_DELETE_DIRECTIVE_HANDLER_H_

#define CreateTimeRangeDeleteDirective                    \
  CreateUrlDeleteDirective_ChromiumImpl(const GURL& url); \
  bool CreateTimeRangeDeleteDirective

#include "src/components/history/core/browser/sync/delete_directive_handler.h"  // IWYU pragma: export

#undef CreateTimeRangeDeleteDirective

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_HISTORY_CORE_BROWSER_SYNC_DELETE_DIRECTIVE_HANDLER_H_
