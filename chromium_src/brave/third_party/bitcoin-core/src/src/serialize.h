/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_BRAVE_THIRD_PARTY_BITCOIN_CORE_SRC_SRC_SERIALIZE_H_
#define BRAVE_CHROMIUM_SRC_BRAVE_THIRD_PARTY_BITCOIN_CORE_SRC_SRC_SERIALIZE_H_

#include <ios>
#include <string>

#include "base/check.h"

namespace std {
namespace brave {
using string = ::std::string;
}
}  // namespace std

#define throw CHECK(false) <<
#define ios_base brave
#define failure string
#include "../../../../../../../brave/third_party/bitcoin-core/src/src/serialize.h"
#undef throw
#undef ios_base
#undef string

#endif  // BRAVE_CHROMIUM_SRC_BRAVE_THIRD_PARTY_BITCOIN_CORE_SRC_SRC_SERIALIZE_H_
