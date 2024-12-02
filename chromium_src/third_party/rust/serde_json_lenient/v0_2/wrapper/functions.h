/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_RUST_SERDE_JSON_LENIENT_V0_2_WRAPPER_SERDE_JSON_LENIENT_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_RUST_SERDE_JSON_LENIENT_V0_2_WRAPPER_SERDE_JSON_LENIENT_H_

#define BRAVE_SERDE_JSON_LENIENT_FUNCTIONS                                  \
  void (*list_append_i64_fn)(ContextPointer&, int64_t);                     \
  void (*list_append_u64_fn)(ContextPointer&, uint64_t);                    \
  void (*dict_set_i64_fn)(ContextPointer&, rust::Str, int64_t);             \
  void (*dict_set_u64_fn)(ContextPointer&, rust::Str, uint64_t);            \
  void list_append_i64(ContextPointer& c, int64_t val) const {              \
    list_append_i64_fn(c, val);                                             \
  }                                                                         \
  void list_append_u64(ContextPointer& c, uint64_t val) const {             \
    list_append_u64_fn(c, val);                                             \
  }                                                                         \
  void dict_set_i64(ContextPointer& c, rust::Str key, int64_t val) const {  \
    dict_set_i64_fn(c, key, val);                                           \
  }                                                                         \
  void dict_set_u64(ContextPointer& c, rust::Str key, uint64_t val) const { \
    dict_set_u64_fn(c, key, val);                                           \
  }

#include "src/third_party/rust/serde_json_lenient/v0_2/wrapper/functions.h"

#undef BRAVE_SERDE_JSON_LENIENT_FUNCTIONS

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_RUST_SERDE_JSON_LENIENT_V0_2_WRAPPER_SERDE_JSON_LENIENT_H_
