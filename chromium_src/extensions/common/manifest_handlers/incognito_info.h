/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_EXTENSIONS_COMMON_MANIFEST_HANDLERS_INCOGNITO_INFO_H_
#define BRAVE_CHROMIUM_SRC_EXTENSIONS_COMMON_MANIFEST_HANDLERS_INCOGNITO_INFO_H_

#define IncognitoInfo IncognitoInfoBase
#define IsSplitMode IsSplitMode_ChromiumImpl
#include "../../../../../extensions/common/manifest_handlers/incognito_info.h"
#undef IsSplitMode
#undef IncognitoInfo

namespace extensions {

struct IncognitoInfo : public IncognitoInfoBase {
  using IncognitoInfoBase::IncognitoInfoBase;
  ~IncognitoInfo() override = default;

  // This is used in patches when there are multiple usages of IsSplitMode in
  // the original file that don't have matching argument caluclation.
  static bool IsSplitMode2(const Extension* extension, bool is_tor);

  // Factory method to use with IsSplitMode.
  static std::unique_ptr<IncognitoInfo> ForSplitModeCheck(bool is_tor);

  // Non-static that can be accessed via IncognitoInfo::ForSplitModeCheck()->
  virtual bool IsSplitMode(const Extension* extension);
};

struct IncognitoInfoForTor : public IncognitoInfo {
  using IncognitoInfo::IncognitoInfo;
  ~IncognitoInfoForTor() override = default;

  bool IsSplitMode(const Extension* extension) override;
};

}  // namespace extensions

#endif  // BRAVE_CHROMIUM_SRC_EXTENSIONS_COMMON_MANIFEST_HANDLERS_INCOGNITO_INFO_H_
