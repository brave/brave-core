/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/base/json/json_reader.cc"

#include "base/notimplemented.h"
#include "third_party/rust/serde_json_lenient/v0_2/wrapper/lib.rs.h"

namespace serde_json_lenient {

// TODO(https://github.com/brave/brave-browser/issues/47120): These methods are
// not implemented for now, but their purpose is to eventually hook up the
// the `serde_json_lenient` to `base::Value`, and let us store binary blobs
// whenever dealing with 64bit integers that cannot be represented in
// `base::Value`.

void list_append_i64(base::Value::List& ctx, int64_t val) {
  NOTIMPLEMENTED();
}

void list_append_u64(base::Value::List& ctx, uint64_t val) {
  NOTIMPLEMENTED();
}

void dict_set_i64(base::Value::Dict& ctx, rust::Str key, int64_t val) {
  NOTIMPLEMENTED();
}

void dict_set_u64(base::Value::Dict& ctx, rust::Str key, uint64_t val) {
  NOTIMPLEMENTED();
}

}  // namespace serde_json_lenient
