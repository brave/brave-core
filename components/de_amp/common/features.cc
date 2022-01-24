// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/de_amp/common/features.h"

#include "base/feature_list.h"

namespace de_amp {
namespace features {

// When enabled, Brave will try to de-AMP a page i.e. load the canonical,
// non-AMP version if the page is an AMP page.
const base::Feature kBraveDeAMP{"BraveDeAMP", base::FEATURE_ENABLED_BY_DEFAULT};

}  // namespace features
}  // namespace de_amp
