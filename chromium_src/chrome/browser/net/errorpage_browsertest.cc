/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/browsing_data/core/browsing_data_utils.h"

// Add brave-today-cdn to the exception list, since it's loaded at startup when
// BraveNewsTabHelper is initialized and will affect the outcome of the
// ErrorPageAutoReloadTest.AutoLoad test
#define DeprecatedGetOriginAsURL                         \
  host().find("brave-today-cdn") != std::string::npos) { \
    return false;                                        \
  }                                                      \
if (params->url_request.url.DeprecatedGetOriginAsURL

#include "src/chrome/browser/net/errorpage_browsertest.cc"

#undef DeprecatedGetOriginAsURL
