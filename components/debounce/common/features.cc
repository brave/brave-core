// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/debounce/common/features.h"

#include "base/feature_list.h"

namespace debounce {
namespace features {

// When enabled, Brave will try to parse tracking URLs of the form
// https://tracker.example.com/?url=http://final.example.com/
// and automatically redirect to the final URL without sending any
// network requests to the tracker.
const base::Feature kBraveDebounce{"BraveDebounce",
                                   base::FEATURE_ENABLED_BY_DEFAULT};

}  // namespace features
}  // namespace debounce
