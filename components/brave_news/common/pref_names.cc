// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/common/pref_names.h"

#include "components/prefs/pref_service.h"

namespace brave_news {

bool IsEnabled(PrefService* prefs) {
  return prefs->GetBoolean(prefs::kNewTabPageShowToday) &&
         prefs->GetBoolean(prefs::kBraveNewsOptedIn);
}

}  // namespace brave_news
