/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/browsing_data/counters/brave_site_settings_counter.h"

#define SiteSettingsCounter BraveSiteSettingsCounter
#include "../../../../../../chrome/browser/browsing_data/counters/browsing_data_counter_factory.cc"  // NOLINT
#undef SiteSettingsCounter
