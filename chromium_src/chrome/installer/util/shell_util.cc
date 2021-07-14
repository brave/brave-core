/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/strings/stringprintf.h"
#define BRAVE_IPFS L"ipfs"
#define BRAVE_IPNS L"ipns"

#define BRAVE_GET_TARGET_FOR_DEFAULT_APP_SETTINGS                         \
  if (base::EqualsCaseInsensitiveASCII(protocol, BRAVE_IPFS))             \
    return base::StringPrintf(kSystemSettingsDefaultAppsFormat, L"IPFS"); \
  if (base::EqualsCaseInsensitiveASCII(protocol, BRAVE_IPNS))             \
    return base::StringPrintf(kSystemSettingsDefaultAppsFormat, L"IPNS");

#include "../../../../../chrome/installer/util/shell_util.cc"
#undef BRAVE_GET_TARGET_FOR_DEFAULT_APP_SETTINGS
#undef BRAVE_IPFS
#undef BRAVE_IPNS
