/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/browsing_data/chrome_browsing_data_remover_delegate.h"
#include "build/build_config.h"

#define CHROME_BROWSING_DATA_REMOVER_DELEGATE_GET_HISTORY_SUFFIX \
  case TracingDataType::kIPFSCache:                              \
    return "IPFSCache";

#include "src/chrome/browser/browsing_data/chrome_browsing_data_remover_delegate.cc"
#undef CHROME_BROWSING_DATA_REMOVER_DELEGATE_GET_HISTORY_SUFFIX
