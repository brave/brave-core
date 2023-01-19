/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/diagnostics/diagnostic_util.h"

#include <utility>

#include "base/check.h"

namespace ads {

namespace {

constexpr char kNameKey[] = "name";
constexpr char kValueKey[] = "value";

}  // namespace

base::Value::List ToValue(const DiagnosticMap& diagnostics) {
  base::Value::List list;

  for (const auto& diagnostic : diagnostics) {
    const DiagnosticEntryInterface* const entry = diagnostic.second.get();
    DCHECK(entry);

    base::Value::Dict dict;
    dict.Set(kNameKey, entry->GetName());
    dict.Set(kValueKey, entry->GetValue());
    list.Append(std::move(dict));
  }

  return list;
}

}  // namespace ads
