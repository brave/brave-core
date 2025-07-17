/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_RUST_SERDE_JSON_LENIENT_V0_2_WRAPPER_FUNCTIONS_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_RUST_SERDE_JSON_LENIENT_V0_2_WRAPPER_FUNCTIONS_H_

#include <stdint.h>

#include "brave/components/json/buildflags/buildflags.h"
#include "third_party/rust/cxx/v1/cxx.h"

// We only disable 64-bit integer support when building redirect_cc, which means
// that `chromium_src` shadow inclusions like the one below do not work, and
// therefore we have to cancel it, that both header and override file are being
// added by the visitor.
#if BUILDFLAG(ENABLE_JSON_64BIT_INT_SUPPORT)
#include "src/third_party/rust/serde_json_lenient/v0_2/wrapper/functions.h"  // IWYU pragma: export
#endif

namespace base {
class DictValue;
class ListValue;
}  // namespace base

namespace serde_json_lenient {
using Dict = base::DictValue;
using List = base::ListValue;

void list_append_i64(List& ctx, int64_t val);
void list_append_u64(List& ctx, uint64_t val);
void dict_set_i64(Dict& ctx, rust::Str key, int64_t val);
void dict_set_u64(Dict& ctx, rust::Str key, uint64_t val);
}  // namespace serde_json_lenient

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_RUST_SERDE_JSON_LENIENT_V0_2_WRAPPER_FUNCTIONS_H_
