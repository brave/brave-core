/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/ntp_tiles/popular_sites.h"

// We don't support downloading popular sites
#define popular_sites_(...) popular_sites_(nullptr)

#include "src/components/ntp_tiles/most_visited_sites.cc"

#undef popular_sites_
