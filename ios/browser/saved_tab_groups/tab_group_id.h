// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_SAVED_TAB_GROUPS_TAB_GROUP_ID_H_
#define BRAVE_IOS_BROWSER_SAVED_TAB_GROUPS_TAB_GROUP_ID_H_

#include <stdint.h>

#include "base/containers/span.h"
#include "components/tab_groups/tab_group_id.h"

namespace brave {

// `bytes` are a UUID in RFC 4122 order. The first 8 bytes are the big-endian
// high word of the `base::Token`, and the last 8 are the big-endian low word.
// `TabGroupId::FromRawToken` uses that token, and the conversion is its own
// inverse.
tab_groups::TabGroupId TabGroupIdFromUuidBytes(
    base::span<const uint8_t, 16> bytes);
void UuidBytesFromTabGroupId(const tab_groups::TabGroupId& group_id,
                             base::span<uint8_t, 16> bytes);

// A `web::WebStateID`, which is never 0.
int32_t MakeSyncTabID();

}  // namespace brave

#endif  // BRAVE_IOS_BROWSER_SAVED_TAB_GROUPS_TAB_GROUP_ID_H_
