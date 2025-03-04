/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef THIRD_PARTY_RUST_IDNA_V0_2_CRATE_IDNA_H_
#define THIRD_PARTY_RUST_IDNA_V0_2_CRATE_IDNA_H_

#include "third_party/rust/cxx/v1/cxx.h"

namespace idna {

struct IdnaResult;
void InitializeICUForTesting();
IdnaResult DomainToASCII(rust::Str host);

}  // namespace idna

#endif  // THIRD_PARTY_RUST_IDNA_V0_2_CRATE_IDNA_H_
