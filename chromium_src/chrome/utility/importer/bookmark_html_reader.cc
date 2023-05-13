/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <vector>
#include "build/build_config.h"

#if BUILDFLAG(IS_ANDROID)
namespace importer {
bool ReencodeFavicon(const unsigned char* src_data,
                     size_t src_len,
                     std::vector<unsigned char>* png_data) {
  return false;
}
}  // namespace importer
#endif

#include "src/chrome/utility/importer/bookmark_html_reader.cc"
