// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/webcompat_reporter/browser/webcompat_reporter_utils.h"

#include <string>
#include <string_view>

#include "base/containers/fixed_flat_set.h"
#include "brave/components/brave_shields/core/common/brave_shield_constants.h"
#include "components/crx_file/id_util.h"

namespace {

// These IDs are hardcoded rather than derived from a hash because their filter
// lists' public keys arrive at runtime from the filter-list catalog, not as a
// compile-time constant in this codebase.
constexpr auto kComponentIdsToReport =
    base::MakeFixedFlatSet<std::string_view>({
        "adcocjohghhfpidemphmcmlmhnfgikei",  // Brave Ad Block First Party
                                             // Filters (plaintext)
        "bfpgedeaaibpoidldhjcknekahbikncb",  // Fanboy's Mobile Notifications
                                             // (plaintext)
        "cdbbhgbmjhfnhnmgeddbliobbofkgdhe",  // EasyList Cookie (plaintext)
        "gkboaolpopklhgplhaaiboijnklogmbc",  // Regional Catalog
        "iodkpdagapdfkphljnddpjlldadblomo",  // Brave Ad Block Updater
                                             // (plaintext)
        "jcfckfokjmopfomnoebdkdhbhcgjfnbi",  // Brave Experimental Adblock
                                             // Rules (plaintext)
    });

}  // namespace

namespace webcompat_reporter {

bool SendComponentVersionInReport(std::string_view component_id) {
  // Brave Ad Block Updater (Resources). Computed from the pinned hash
  // rather than duplicated as a literal, since the hash is the only source
  // of truth for this component's identity.
  return kComponentIdsToReport.contains(component_id) ||
         component_id ==
             crx_file::id_util::GenerateIdFromHash(
                 brave_shields::kAdBlockResourceComponentPublicKeySHA256);
}

std::string BoolToString(bool value) {
  return value ? "true" : "false";
}

}  // namespace webcompat_reporter
