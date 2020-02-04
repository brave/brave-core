/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "brave/browser/browsing_data/counters/shields_settings_counter.h"
#include "brave/components/content_settings/core/browser/brave_host_content_settings_map.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"

#define HANDLE_BRAVE_PREFS \
  if (pref_name == browsing_data::prefs::kDeleteShieldsSettings) { \
    return std::make_unique<ShieldsSettingsCounter>( \
        static_cast<BraveHostContentSettingsMap*>( \
            HostContentSettingsMapFactory::GetForProfile(profile))); \
    return nullptr; \
  }

#include "../../../../../../chrome/browser/browsing_data/counters/browsing_data_counter_factory.cc"

#undef HANDLE_BRAVE_PREFS
