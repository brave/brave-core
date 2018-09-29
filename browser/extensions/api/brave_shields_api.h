/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EXTENSIONS_API_BRAVE_SHIELDS_API_H_
#define BRAVE_BROWSER_EXTENSIONS_API_BRAVE_SHIELDS_API_H_

#include "extensions/browser/extension_function.h"

namespace extensions {
namespace api {

class BraveShieldsAllowScriptsOnceFunction : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveShields.allowScriptsOnce", UNKNOWN)

 protected:
  ~BraveShieldsAllowScriptsOnceFunction() override;

  ResponseAction Run() override;
};

class BraveShieldsContentSettingGetFunction
    : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveShields.get", UNKNOWN)

 protected:
  ~BraveShieldsContentSettingGetFunction() override {}

  // ExtensionFunction:
  ResponseAction Run() override;
};

class BraveShieldsContentSettingSetFunction
    : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveShields.set", UNKNOWN)

 protected:
  ~BraveShieldsContentSettingSetFunction() override {}

  // ExtensionFunction:
  ResponseAction Run() override;
};

}  // namespace api
}  // namespace extensions

#endif  // BRAVE_BROWSER_EXTENSIONS_API_BRAVE_SHIELDS_API_H_
