// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/common/pref_names.h"

#include "components/prefs/pref_service.h"

namespace brave_news {
bool GetIsEnabled(PrefService* prefs) {
  bool should_show = prefs->GetBoolean(prefs::kNewTabPageShowToday);
  bool opted_in = prefs->GetBoolean(prefs::kBraveNewsOptedIn);
  bool is_enabled = (should_show && opted_in);
  return is_enabled;
}
}  // namespace brave_news
