/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "extensions/browser/blocklist.h"

#include <set>
#include <utility>

#include "extensions/browser/extensions_browser_client.h"
#include "extensions/common/extension_id.h"

namespace extensions {
namespace {

// Forces the IDs on Brave's distributed malicious-extension list to
// BLOCKLISTED_MALWARE in the resolved state map. These IDs come from a Brave
// component, so they never enter the upstream `blocklisted_ids` set and thus
// never reach GetBlocklistStateForIDs / BlocklistStateFetcher.
void MergeBraveMalwareIDs(std::set<ExtensionId> ids,
                          Blocklist::GetBlocklistedIDsCallback callback,
                          const Blocklist::BlocklistStateMap& state_map) {
  Blocklist::BlocklistStateMap result = state_map;
  for (const auto& id : ids) {
    if (ExtensionsBrowserClient::Get()->IsOnBraveMalwareExtensionList(id)) {
      result[id] = BLOCKLISTED_MALWARE;
    }
  }
  std::move(callback).Run(result);
}

}  // namespace
}  // namespace extensions

#include <extensions/browser/blocklist.cc>
