// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/commander/common/constants.h"

#include <iterator>

namespace commander {
const char16_t kCommandPrefix[] = u":>";
const uint16_t kCommandPrefixLength =
    std::size(kCommandPrefix) - 1;  // -1 for the \0
}  // namespace commander
