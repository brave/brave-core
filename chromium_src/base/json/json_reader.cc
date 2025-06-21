/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/json/json_reader.h"

#include "third_party/rust/serde_json_lenient/v0_2/wrapper/lib.rs.h"

#define allow_x_escapes                                                  \
  allow_64bit_numbers = (options & base::JSON_ALLOW_64BIT_NUMBERS) != 0, \
  .allow_x_escapes

#include "src/base/json/json_reader.cc"

#undef allow_x_escapes

// These are some extra functions to be called by serde_json_lenient to handle
// 64-bit integers as blobs.

namespace serde_json_lenient {

void list_append_i64(base::Value::List& ctx, int64_t val) {
  std::string value = base::NumberToString(val);
  ctx.Append(base::Value(base::as_byte_span(value)));
}
void list_append_u64(base::Value::List& ctx, uint64_t val) {
  std::string value = base::NumberToString(val);
  ctx.Append(base::Value(base::as_byte_span(value)));
}
void dict_set_i64(base::Value::Dict& ctx, rust::Str key, int64_t val) {
  std::string value = base::NumberToString(val);
  ctx.Set(base::RustStrToStringView(key),
          base::Value(base::as_byte_span(value)));
}
void dict_set_u64(base::Value::Dict& ctx, rust::Str key, uint64_t val) {
  std::string value = base::NumberToString(val);
  ctx.Set(base::RustStrToStringView(key),
          base::Value(base::as_byte_span(value)));
}

}  // namespace serde_json_lenient
