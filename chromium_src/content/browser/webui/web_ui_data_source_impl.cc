// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "content/browser/webui/web_ui_data_source_impl.h"

#include "base/strings/strcat.h"

namespace {
std::string GetBraveTrustedTypesValue(
    network::mojom::CSPDirectiveName directive,
    std::string_view value) {
  if (directive == network::mojom::CSPDirectiveName::TrustedTypes &&
      !value.empty()) {
    CHECK(value.ends_with(";")) << "Invalid CSP value: " << value;

    // Truncate the trailing semicolon so we can append our default trusted
    // types.
    return base::StrCat(
        {value.substr(0, value.size() - 1), " svelte-trusted-html default;"});
  }
  return std::string(value);
}
}  // namespace

#include <content/browser/webui/web_ui_data_source_impl.cc>
