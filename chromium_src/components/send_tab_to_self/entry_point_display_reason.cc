/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <optional>

#include "components/send_tab_to_self/entry_point_display_reason.h"

#define GetEntryPointDisplayReason GetEntryPointDisplayReason_ChromiumImpl
#include "src/components/send_tab_to_self/entry_point_display_reason.cc"
#undef GetEntryPointDisplayReason

namespace send_tab_to_self::internal {

std::optional<EntryPointDisplayReason> GetEntryPointDisplayReason(
    const GURL& url_to_share,
    syncer::SyncService* sync_service,
    SendTabToSelfModel* send_tab_to_self_model,
    PrefService* pref_service) {
  std::optional<send_tab_to_self::EntryPointDisplayReason> reason =
      GetEntryPointDisplayReason_ChromiumImpl(
          url_to_share, sync_service, send_tab_to_self_model, pref_service);
  if (!reason) {
    return reason;
  }
  // We do not want to show any UI suggesting that the user signs into their
  // account. There used to be an upstream flag that disabled this
  // functionality, but it was removed. Even without the flag, we are not
  // hitting either of these right now, but if the upstream code changes we'd
  // still want to prevent the UI from showing.
  if (*reason == EntryPointDisplayReason::kInformNoTargetDevice ||
      *reason == EntryPointDisplayReason::kOfferSignIn) {
    return std::nullopt;
  }

  return reason;
}

}  // namespace send_tab_to_self::internal
