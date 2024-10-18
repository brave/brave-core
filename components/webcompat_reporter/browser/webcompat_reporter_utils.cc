// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/webcompat_reporter/browser/webcompat_reporter_utils.h"

#include <string_view>

#include "base/containers/contains.h"
#include "base/containers/fixed_flat_set.h"
#include "brave/components/brave_shields/core/common/brave_shield_constants.h"

namespace {

constexpr auto kComponentIds = base::MakeFixedFlatSet<std::string_view>({
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
    brave_shields::kAdBlockResourceComponentId,  // Brave Ad Block Updater
                                                 // (Resources)
});

}  // namespace

namespace webcompat_reporter {

bool NeedsToGetComponentInfo(std::string_view component_id) {
  return base::Contains(kComponentIds, component_id);
}

}  // namespace webcompat_reporter
