/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/json/json_reader.h"

#if BUILDFLAG(BUILD_RUST_JSON_READER)

#include "base/strings/string_view_rust.h"
#include "third_party/rust/serde_json_lenient/v0_2/wrapper/lib.rs.h"

namespace base {

namespace {
using serde_json_lenient::ContextPointer;

template <class T, class As = T>
void ListAppendBinaryValue(ContextPointer& ctx, T v) {
  auto& value = reinterpret_cast<base::Value&>(ctx);
  std::string str = base::NumberToString(As{v});
  value.GetList().Append(base::Value(as_byte_span(str)));
}

template <class T, class As = T>
void DictSetBinaryValue(ContextPointer& ctx, rust::Str key, T v) {
  auto& dict = reinterpret_cast<base::Value&>(ctx).GetDict();
  std::string str = base::NumberToString(As{v});
  dict.Set(base::RustStrToStringView(key), base::Value(as_byte_span(str)));
}

}  // namespace

}  // namespace base

#endif  // BUILDFLAG(BUILD_RUST_JSON_READER)

#define BRAVE_JSON_READER_ALLOW_64BIT_NUMBERS_ARG \
  .allow_64bit_numbers = (options & base::JSON_ALLOW_64BIT_NUMBERS) != 0,

#define BRAVE_JSON_READER_ALLOW_64BIT_NUMBERS_FUNCTIONS  \
  .list_append_i64_fn = ListAppendBinaryValue<int64_t>,  \
  .list_append_u64_fn = ListAppendBinaryValue<uint64_t>, \
  .dict_set_i64_fn = DictSetBinaryValue<int64_t>,        \
  .dict_set_u64_fn = DictSetBinaryValue<uint64_t>,

#include "src/base/json/json_reader.cc"

#undef BRAVE_JSON_READER_ALLOW_64BIT_NUMBERS_ARG
#undef BRAVE_JSON_READER_ALLOW_64BIT_NUMBERS_FUNCTIONS

namespace base {

#if BUILDFLAG(BUILD_RUST_JSON_READER)
JSONReader::Result DecodeJSONInRust(std::string_view json, int options) {
  SCOPED_UMA_HISTOGRAM_TIMER_MICROS(kSecurityJsonParsingTime);
  return DecodeJSONInRust(json, options, internal::kAbsoluteMaxDepth);
}
#endif  // BUILDFLAG(BUILD_RUST_JSON_READER)

}  // namespace base
