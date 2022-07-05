/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/diagnostics/diagnostic_util.h"

#include <utility>

#include "base/check.h"
#include "base/values.h"

namespace ads {

namespace {

constexpr char kNameKey[] = "name";
constexpr char kValueKey[] = "value";

}  // namespace

base::Value ToValue(const DiagnosticMap& diagnostics) {
  base::Value list(base::Value::Type::LIST);

  for (const auto& diagnostic : diagnostics) {
    DiagnosticEntryInterface* entry = diagnostic.second.get();
    DCHECK(entry);

    base::Value dictionary(base::Value::Type::DICTIONARY);
    dictionary.SetStringKey(kNameKey, entry->GetName());
    dictionary.SetStringKey(kValueKey, entry->GetValue());
    list.Append(std::move(dictionary));
  }

  return list;
}

}  // namespace ads
