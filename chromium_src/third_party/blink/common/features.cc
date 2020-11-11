/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "../../../../../third_party/blink/common/features.cc"

namespace blink {
namespace features {
const base::Feature kBraveEphemeralStorage{"EphemeralStorage",
                                           base::FEATURE_DISABLED_BY_DEFAULT};
}  // namespace features
}  // namespace blink
