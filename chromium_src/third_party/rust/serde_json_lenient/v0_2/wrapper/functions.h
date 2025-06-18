/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_RUST_SERDE_JSON_LENIENT_V0_2_WRAPPER_SERDE_JSON_LENIENT_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_RUST_SERDE_JSON_LENIENT_V0_2_WRAPPER_SERDE_JSON_LENIENT_H_

#include "src/third_party/rust/serde_json_lenient/v0_2/wrapper/functions.h"  // IWYU pragma: export

namespace serde_json_lenient {
void list_append_i64(List& ctx, int64_t val);
void list_append_u64(List& ctx, uint64_t val);
void dict_set_i64(Dict& ctx, rust::Str key, int64_t val);
void dict_set_u64(Dict& ctx, rust::Str key, uint64_t val);
}  // namespace serde_json_lenient

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_RUST_SERDE_JSON_LENIENT_V0_2_WRAPPER_SERDE_JSON_LENIENT_H_
