/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "extensions/browser/blocklist.h"

#include "extensions/browser/extensions_browser_client.h"
#include "extensions/common/extension_id.h"

namespace extensions {
namespace {

// Forces the IDs on Brave's distributed malicious-extension list to
// BLOCKLISTED_MALWARE in the resolved state map. These IDs come from a Brave
// component, so they never enter the upstream `blocklisted_ids` set and thus
// never reach GetBlocklistStateForIDs / BlocklistStateFetcher.
void MergeBraveMalwareIDs(std::set<ExtensionId> brave_malware_ids,
                          Blocklist::GetBlocklistedIDsCallback callback,
                          const Blocklist::BlocklistStateMap& state_map) {
  Blocklist::BlocklistStateMap result = state_map;
  for (const auto& id : brave_malware_ids) {
    result[id] = BLOCKLISTED_MALWARE;
  }
  std::move(callback).Run(result);
}

}  // namespace
}  // namespace extensions

// Injected at the top of Blocklist::GetBlocklistedIDs. Computes which of the
// queried `ids` are on Brave's local malware list and wraps `callback` so those
// IDs are forced to BLOCKLISTED_MALWARE in the final map. Wrapping before the
// early-return path means it also applies when no upstream DB manager is
// present.
#define BRAVE_GET_BLOCKLISTED_IDS                                              \
  {                                                                            \
    std::set<ExtensionId> brave_malware_ids;                                   \
    for (const auto& id : ids) {                                               \
      if (ExtensionsBrowserClient::Get()->IsOnBraveMalwareExtensionList(id)) { \
        brave_malware_ids.insert(id);                                          \
      }                                                                        \
    }                                                                          \
    callback =                                                                 \
        base::BindOnce(&MergeBraveMalwareIDs, std::move(brave_malware_ids),    \
                       std::move(callback));                                   \
  }

#include <extensions/browser/blocklist.cc>

#undef BRAVE_GET_BLOCKLISTED_IDS
