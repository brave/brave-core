/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EXTENSIONS_API_BRAVE_THEME_API_H_
#define BRAVE_BROWSER_EXTENSIONS_API_BRAVE_THEME_API_H_

#include "extensions/browser/extension_function.h"

namespace extensions {
namespace api {

class BraveThemeGetBraveThemeListFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveTheme.getBraveThemeList", UNKNOWN)

 protected:
  ~BraveThemeGetBraveThemeListFunction() override {}

  ResponseAction Run() override;
};

class BraveThemeGetBraveThemeTypeFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveTheme.getBraveThemeType", UNKNOWN)

 protected:
  ~BraveThemeGetBraveThemeTypeFunction() override {}

  ResponseAction Run() override;
};

class BraveThemeSetBraveThemeTypeFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveTheme.setBraveThemeType", UNKNOWN)

 protected:
  ~BraveThemeSetBraveThemeTypeFunction() override {}

  ResponseAction Run() override;
};

}  // namespace api
}  // namespace extensions

#endif  // BRAVE_BROWSER_EXTENSIONS_API_BRAVE_THEME_API_H_
