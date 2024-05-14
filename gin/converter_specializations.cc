/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/gin/converter_specializations.h"

#include <memory>
#include <utility>

#include "content/public/renderer/v8_value_converter.h"

namespace gin {
bool Converter<base::Value::List>::FromV8(v8::Isolate* isolate,
                                          v8::Local<v8::Value> v8_value,
                                          base::Value::List* out) {
  std::unique_ptr<base::Value> base_value =
      content::V8ValueConverter::Create()->FromV8Value(
          v8_value, isolate->GetCurrentContext());
  if (!base_value || !base_value->is_list()) {
    return false;
  }

  *out = std::move(*base_value).TakeList();
  return true;
}
}  // namespace gin
