/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define BRAVE_COPY_WITH_DEFAULT_PORT                     \
  if (text_records()) {                                  \
    std::vector<std::string> copy_text_records;          \
    for (const auto& record : text_records().value())    \
      copy_text_records.push_back(record);               \
    copy.set_text_records(std::move(copy_text_records)); \
  }

#include "../../../../net/dns/host_cache.cc"
#undef BRAVE_COPY_WITH_DEFAULT_PORT
