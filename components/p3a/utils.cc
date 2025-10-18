/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/utils.h"

#include "base/values.h"

namespace p3a {

bool ParseValueList(const base::Value* value, base::Value::List* field) {
  if (!value || !value->is_list()) {
    return true;
  }

  *field = value->GetList().Clone();

  return true;
}

bool ParseValue(const base::Value* value, base::Value* field) {
  if (!value) {
    return true;
  }
  *field = value->Clone();
  return true;
}

bool ParseDict(const base::Value* value, base::Value::Dict* field) {
  if (!value || !value->is_dict()) {
    return true;
  }

  *field = value->GetDict().Clone();

  return true;
}

}  // namespace p3a
