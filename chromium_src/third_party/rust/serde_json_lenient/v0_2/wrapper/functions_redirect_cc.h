/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_RUST_SERDE_JSON_LENIENT_V0_2_WRAPPER_SERDE_JSON_LENIENT_REDIRECT_CC_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_RUST_SERDE_JSON_LENIENT_V0_2_WRAPPER_SERDE_JSON_LENIENT_REDIRECT_CC_H_

// This override file is necessary to provide the stubs for the definitions
// provided by cxxbridge. Unfortunately, cxxbridge does not support conditional
// compilation, so in order to be able support the use of `#[cfg(feature =
// "large_integers")]` in the visitor, we need to provide these stubs for
// redirect.cc builds specifically otherwise it will result in linking errors.
//
// These functions are not supposed to be called. All callers are guarded behind
// the "large_integers" feature flag, and we should have linker errors if they
// are called in the redirect.cc build. We are also suppressing dead code
// warnings for these functions when building redirect.cc.

#ifdef IS_REDIRECT_CC_BUILD

namespace serde_json_lenient {
void list_append_i64(List& ctx, int64_t val);
void list_append_u64(List& ctx, uint64_t val);
void dict_set_i64(Dict& ctx, rust::Str key, int64_t val);
void dict_set_u64(Dict& ctx, rust::Str key, uint64_t val);
}  // namespace serde_json_lenient

#endif  // IS_REDIRECT_CC_BUILD

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_RUST_SERDE_JSON_LENIENT_V0_2_WRAPPER_SERDE_JSON_LENIENT_REDIRECT_CC_H_
