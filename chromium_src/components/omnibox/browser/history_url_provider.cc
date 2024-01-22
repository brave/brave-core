// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "components/omnibox/common/omnibox_features.h"

#define threshold(age)                                                  \
  threshold =                                                           \
      base::FeatureList::IsEnabled(omnibox::kOmniboxMoreHistoryResults) \
          ? (base::Time::Now() - base::Days(365))                       \
          : history::AutocompleteAgeThreshold()

#include "src/components/omnibox/browser/history_url_provider.cc"

#undef threshold
