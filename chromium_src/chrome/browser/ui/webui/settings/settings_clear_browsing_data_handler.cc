/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/webui/settings/settings_clear_browsing_data_handler.h"

#define HANDLE_BROWSING_DATA_TYPE_SHIELDS_SETTINGS \
  case BrowsingDataType::SHIELDS_SETTINGS: \
    remove_mask |= \
        ChromeBrowsingDataRemoverDelegate::DATA_TYPE_SHIELDS_SETTINGS; \
      break;

#include "../../../../../../../chrome/browser/ui/webui/settings/settings_clear_browsing_data_handler.cc"  // NOLINT

#undef HANDLE_BROWSING_DATA_TYPE_SHIELDS_SETTINGS
