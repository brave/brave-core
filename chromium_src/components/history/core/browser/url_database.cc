// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/time/time.h"
#include "components/history/core/browser/features.h"

#define Days(num)                                                        \
  Days(base::FeatureList::IsEnabled(history::kHistoryMoreSearchResults)  \
           ? 365                                                         \
           : num));                                                      \
  /* DCHECK, to make sure we aren't overriding something we shouldn't */ \
  DCHECK(num == kLowQualityMatchAgeLimitInDays

#include "src/components/history/core/browser/url_database.cc"

#undef Days
