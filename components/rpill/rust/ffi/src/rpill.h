/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_RPILL_RUST_FFI_SRC_RPILL_H_
#define BRAVE_COMPONENTS_RPILL_RUST_FFI_SRC_RPILL_H_

extern "C++" {
#include "lib.h"  // NOLINT
}

namespace rpill {

bool exec_call();
}  // namespace rpill

#endif  // BRAVE_COMPONENTS_RPILL_RUST_FFI_SRC_RPILL_H_
