/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/ntp_tiles/popular_sites.h"

// Replace MaybeStartFetch with an innocuous call, as we don't want to download
// the popular sites
#define MaybeStartFetch(...) sections()
#define BRAVE_MOST_VISITED_SITES_CREATE_POPULAR_SITES_SECTIONS return sections;

#include "src/components/ntp_tiles/most_visited_sites.cc"

#undef BRAVE_MOST_VISITED_SITES_CREATE_POPULAR_SITES_SECTIONS
#undef MaybeStartFetch
