// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/time/time.h"
#include "components/history/core/browser/features.h"

// In Chromium, this is 3, which vastly reduces the number of useful matches you
// get when searching for the title of an article you've visited. For now, we're
// testing a year.
constexpr int kBraveLowQualityMatchAgeLimitInDays = 365;

// We override the (only) use of base::Days(kLowQualityMatchAgeLimitInDays) in
// the file to use our overridden constant if the flag is on. Additionally, we
// add a DCHECK to make sure we haven't accidentally overriden something else,
// if the file is changed in the future.
#define Days(num)                                                        \
  Days(base::FeatureList::IsEnabled(history::kHistoryMoreSearchResults)  \
           ? kBraveLowQualityMatchAgeLimitInDays                         \
           : num));                                                      \
  /* DCHECK, to make sure we aren't overriding something we shouldn't */ \
  DCHECK(num == kLowQualityMatchAgeLimitInDays

#include "src/components/history/core/browser/url_database.cc"

#undef Days
