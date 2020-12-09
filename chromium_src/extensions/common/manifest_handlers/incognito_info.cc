/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "extensions/common/manifest_handlers/incognito_info.h"

#define IncognitoInfo IncognitoInfoBase
#define IsSplitMode IsSplitMode_ChromiumImpl
#include "../../../../../extensions/common/manifest_handlers/incognito_info.cc"
#undef IsSplitMode
#undef IncognitoInfo

namespace extensions {

// static
bool IncognitoInfo::IsSplitMode2(const Extension* extension, bool is_tor) {
  if (is_tor)
    return true;
  return IncognitoInfoBase::IsSplitMode_ChromiumImpl(extension);
}

// static
std::unique_ptr<IncognitoInfo> IncognitoInfo::ForSplitModeCheck(bool is_tor) {
  // Mode passed to the constructor doesn't matter.
  if (is_tor) {
    return std::make_unique<IncognitoInfoForTor>(
        api::incognito::INCOGNITO_MODE_SPLIT);
  } else {
    return std::make_unique<IncognitoInfo>(
        api::incognito::INCOGNITO_MODE_SPLIT);
  }
}

bool IncognitoInfo::IsSplitMode(const Extension* extension) {
  return IncognitoInfoBase::IsSplitMode_ChromiumImpl(extension);
}

bool IncognitoInfoForTor::IsSplitMode(const Extension* extension) {
  return true;
}

}  // namespace extensions
