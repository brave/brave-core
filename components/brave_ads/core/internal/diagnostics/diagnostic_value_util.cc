/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/diagnostics/diagnostic_value_util.h"

#include <string_view>

#include "brave/components/brave_ads/core/internal/diagnostics/entries/diagnostic_entry_interface.h"

namespace brave_ads {

namespace {

constexpr std::string_view kNameKey = "name";
constexpr std::string_view kValueKey = "value";

}  // namespace

void AppendDiagnosticEntry(base::ListValue& list,
                           const DiagnosticEntryInterface& entry) {
  list.Append(base::DictValue()
                  .Set(kNameKey, entry.GetName())
                  .Set(kValueKey, entry.GetValue()));
}

}  // namespace brave_ads
