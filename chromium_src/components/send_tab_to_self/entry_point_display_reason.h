/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SEND_TAB_TO_SELF_ENTRY_POINT_DISPLAY_REASON_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SEND_TAB_TO_SELF_ENTRY_POINT_DISPLAY_REASON_H_

#define GetEntryPointDisplayReason GetEntryPointDisplayReason_ChromiumImpl
#include <optional>

#include "src/components/send_tab_to_self/entry_point_display_reason.h"  // IWYU pragma: export
#undef GetEntryPointDisplayReason

namespace send_tab_to_self {

namespace internal {

std::optional<EntryPointDisplayReason> GetEntryPointDisplayReason(
    const GURL& url_to_share,
    syncer::SyncService* sync_service,
    SendTabToSelfModel* send_tab_to_self_model,
    PrefService* pref_service);

}  // namespace internal

}  // namespace send_tab_to_self

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SEND_TAB_TO_SELF_ENTRY_POINT_DISPLAY_REASON_H_
