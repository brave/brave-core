/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_DRIVER_SYNC_INTERNALS_UTIL_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_DRIVER_SYNC_INTERNALS_UTIL_H_

#define ConstructAboutInformation ConstructAboutInformation_ChromiumImpl

#include "src/components/sync/driver/sync_internals_util.h"

#undef ConstructAboutInformation

namespace syncer {

namespace sync_ui_util {

std::unique_ptr<base::DictionaryValue> ConstructAboutInformation(
    IncludeSensitiveData include_sensitive_data,
    SyncService* service,
    const std::string& channel);

}  // namespace sync_ui_util

}  // namespace syncer

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_DRIVER_SYNC_INTERNALS_UTIL_H_
