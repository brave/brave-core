/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/image_fetcher/core/image_data_fetcher.h"
#include "brave/components/constants/brave_services_key.h"

#define BRAVE_ADJUST_HEADERS(request_ptr)                                  \
  if ((request_ptr)->url.DomainIs("favicons.proxy.brave.com") &&           \
      (request_ptr)->url.path_piece().starts_with("/faviconV2")) {         \
    LOG(ERROR) << "BRAVE_ADJUST_HEADERS" << request_ptr->url.spec();       \
    (request_ptr)                                                          \
        ->headers.SetHeader("x-brave-key", BUILDFLAG(BRAVE_SERVICES_KEY)); \
  }

#include "src/components/image_fetcher/core/image_data_fetcher.cc"
#undef BRAVE_ADJUST_HEADERS
