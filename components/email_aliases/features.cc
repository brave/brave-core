// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/email_aliases/features.h"

#include "brave/components/brave_account/features.h"

namespace email_aliases::features {

BASE_FEATURE(kEmailAliases, base::FEATURE_DISABLED_BY_DEFAULT);

bool IsEmailAliasesEnabled() {
  return brave_account::features::IsBraveAccountEnabled() &&
         base::FeatureList::IsEnabled(kEmailAliases);
}

}  // namespace email_aliases::features
