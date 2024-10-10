// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/webcompat_reporter/common/webcompat_reporter_utils.h"

#include <unordered_set>

#include "base/no_destructor.h"
#include "brave/components/brave_shields/core/common/brave_shield_constants.h"

namespace webcompat_reporter {
bool NeedsToGetComponentInfo(const std::string& component_id) {
  static const base::NoDestructor<std::unordered_set<std::string>>
      kComponentIds({
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
  return kComponentIds->contains(component_id);
}
}  // namespace webcompat_reporter
