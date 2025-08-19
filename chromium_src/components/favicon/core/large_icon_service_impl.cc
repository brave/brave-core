/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/favicon/core/large_icon_service_impl.h"

#define server_url_(URL) \
  server_url_("https://favicons.proxy.brave.com/faviconV2")

#include "src/components/favicon/core/large_icon_service_impl.cc"
#undef server_url_
