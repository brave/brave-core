/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "extensions/browser/blocklist.h"

#include <set>
#include <utility>

#include "extensions/browser/blocklist_state.h"
#include "extensions/browser/extensions_browser_client.h"
#include "extensions/common/extension_id.h"

namespace extensions {
namespace {

// Bound over Blocklist::GetBlocklistedIDs()'s callback by
// //brave/rewrite/extensions/browser/blocklist.cc.yaml.
void AddEmbedderMalwareStates(std::set<ExtensionId> ids,
                              Blocklist::GetBlocklistedIDsCallback callback,
                              const Blocklist::BlocklistStateMap& state_map) {
  Blocklist::BlocklistStateMap merged = state_map;
  for (const ExtensionId& id : ids) {
    if (ExtensionsBrowserClient::Get()->IsMalwareBlocklisted(id)) {
      merged[id] = BLOCKLISTED_MALWARE;
    }
  }
  std::move(callback).Run(merged);
}

}  // namespace
}  // namespace extensions

#include <extensions/browser/blocklist.cc>
