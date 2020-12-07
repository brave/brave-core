/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_RPILL_RUST_FFI_SRC_LIB_H_
#define BRAVE_COMPONENTS_RPILL_RUST_FFI_SRC_LIB_H_

#include <cstddef>
#include <cstdint>
#include <string>

namespace rpill {

extern "C" {

bool exec_ffi();

}  // extern "C"

}  // namespace rpill

#endif  // BRAVE_COMPONENTS_RPILL_RUST_FFI_SRC_LIB_H_
