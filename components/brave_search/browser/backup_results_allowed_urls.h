/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SEARCH_BROWSER_BACKUP_RESULTS_ALLOWED_URLS_H_
#define BRAVE_COMPONENTS_BRAVE_SEARCH_BROWSER_BACKUP_RESULTS_ALLOWED_URLS_H_

#include "url/gurl.h"

namespace brave_search {

bool IsBackupResultURLAllowed(const GURL& url);

}  // namespace brave_search

#endif  // BRAVE_COMPONENTS_BRAVE_SEARCH_BROWSER_BACKUP_RESULTS_ALLOWED_URLS_H_
