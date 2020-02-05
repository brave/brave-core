/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define HANDLE_BROWSING_DATA_TYPE_SHIELDS_SETTINGS \
  case browsing_data::BrowsingDataType::SHIELDS_SETTINGS: \
    remove_mask |= \
        ChromeBrowsingDataRemoverDelegate::DATA_TYPE_SHIELDS_SETTINGS; \
    break;

#include "../../../../../../chrome/browser/android/browsing_data/browsing_data_bridge.cc"  // NOLINT
#undef HANDLE_BROWSING_DATA_TYPE_SHIELDS_SETTINGS
