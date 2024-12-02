/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <optional>
#include <vector>

#include "base/containers/span.h"
#include "build/build_config.h"

#if BUILDFLAG(IS_ANDROID)
namespace importer {
std::optional<std::vector<uint8_t>> ReencodeFavicon(
    base::span<const uint8_t> src) {
  return std::nullopt;
}
}  // namespace importer
#endif

#include "src/chrome/utility/importer/bookmark_html_reader.cc"
