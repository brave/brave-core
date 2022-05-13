/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define BRAVE_GET_TARGET_FOR_DEFAULT_APP_SETTINGS                     \
  if (base::EqualsCaseInsensitiveASCII(protocol, L"ipfs"))            \
    return base::StrCat({kSystemSettingsDefaultAppsPrefix, L"IPFS"}); \
  if (base::EqualsCaseInsensitiveASCII(protocol, L"ipns"))            \
    return base::StrCat({kSystemSettingsDefaultAppsPrefix, L"IPNS"});

#include "src/base/win/default_apps_util.cc"
#undef BRAVE_GET_TARGET_FOR_DEFAULT_APP_SETTINGS
