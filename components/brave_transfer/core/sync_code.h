// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_TRANSFER_CORE_SYNC_CODE_H_
#define BRAVE_COMPONENTS_BRAVE_TRANSFER_CORE_SYNC_CODE_H_

#include <optional>
#include <string>

#include "brave/components/brave_transfer/core/trust_keys.h"

namespace brave_transfer {

// The chain seed for a 24-word Brave Sync code (PROTOCOL.md §3.1). Returns
// nullopt for anything else, including codes libwally would accept with fewer
// words.
std::optional<Key> SeedFromSyncCode(const std::string& sync_code);

}  // namespace brave_transfer

#endif  // BRAVE_COMPONENTS_BRAVE_TRANSFER_CORE_SYNC_CODE_H_
