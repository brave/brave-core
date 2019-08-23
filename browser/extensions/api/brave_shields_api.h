/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EXTENSIONS_API_BRAVE_SHIELDS_API_H_
#define BRAVE_BROWSER_EXTENSIONS_API_BRAVE_SHIELDS_API_H_

#include "extensions/browser/extension_function.h"

namespace extensions {
namespace api {

class BraveShieldsHostnameCosmeticResourcesFunction : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveShields.hostnameCosmeticResources", UNKNOWN)

 protected:
  ~BraveShieldsHostnameCosmeticResourcesFunction() override {}

  ResponseAction Run() override;
};

class BraveShieldsClassIdStylesheetFunction : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveShields.classIdStylesheet", UNKNOWN)

 protected:
  ~BraveShieldsClassIdStylesheetFunction() override {}

  ResponseAction Run() override;
};

class BraveShieldsAllowScriptsOnceFunction : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveShields.allowScriptsOnce", UNKNOWN)

 protected:
  ~BraveShieldsAllowScriptsOnceFunction() override {}

  ResponseAction Run() override;
};

class BraveShieldsSetBraveShieldsEnabledFunction
    : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveShields.setBraveShieldsEnabled", UNKNOWN)

 protected:
  ~BraveShieldsSetBraveShieldsEnabledFunction() override {}

  ResponseAction Run() override;
};

class BraveShieldsGetBraveShieldsEnabledFunction
    : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveShields.getBraveShieldsEnabled", UNKNOWN)

 protected:
  ~BraveShieldsGetBraveShieldsEnabledFunction() override {}

  ResponseAction Run() override;
};

class BraveShieldsSetCosmeticFilteredElementsFunction : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveShields.setCosmeticFilteredElements", UNKNOWN)

 protected:
  ~BraveShieldsSetCosmeticFilteredElementsFunction() override {}

  ResponseAction Run() override;
};

class BraveShieldsGetCosmeticFilteredElementsFunction : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveShields.getCosmeticFilteredElements", UNKNOWN)

 protected:
  ~BraveShieldsGetCosmeticFilteredElementsFunction() override {}

  ResponseAction Run() override;
};

class BraveShieldsSetAdControlTypeFunction : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveShields.setAdControlType", UNKNOWN)

 protected:
  ~BraveShieldsSetAdControlTypeFunction() override {}

  ResponseAction Run() override;
};

class BraveShieldsGetAdControlTypeFunction : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveShields.getAdControlType", UNKNOWN)

 protected:
  ~BraveShieldsGetAdControlTypeFunction() override {}

  ResponseAction Run() override;
};

class BraveShieldsSetCookieControlTypeFunction
    : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveShields.setCookieControlType", UNKNOWN)

 protected:
  ~BraveShieldsSetCookieControlTypeFunction() override {}

  ResponseAction Run() override;
};

class BraveShieldsGetCookieControlTypeFunction
    : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveShields.getCookieControlType", UNKNOWN)

 protected:
  ~BraveShieldsGetCookieControlTypeFunction() override {}

  ResponseAction Run() override;
};

class BraveShieldsSetFingerprintingControlTypeFunction
    : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveShields.setFingerprintingControlType",
                             UNKNOWN)

 protected:
  ~BraveShieldsSetFingerprintingControlTypeFunction() override {}

  ResponseAction Run() override;
};

class BraveShieldsGetFingerprintingControlTypeFunction
    : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveShields.getFingerprintingControlType",
                             UNKNOWN)

 protected:
  ~BraveShieldsGetFingerprintingControlTypeFunction() override {}

  ResponseAction Run() override;
};

class BraveShieldsSetHTTPSEverywhereEnabledFunction
    : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveShields.setHTTPSEverywhereEnabled",
                             UNKNOWN)

 protected:
  ~BraveShieldsSetHTTPSEverywhereEnabledFunction() override {}

  ResponseAction Run() override;
};

class BraveShieldsGetHTTPSEverywhereEnabledFunction
    : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveShields.getHTTPSEverywhereEnabled",
                             UNKNOWN)

 protected:
  ~BraveShieldsGetHTTPSEverywhereEnabledFunction() override {}

  ResponseAction Run() override;
};

class BraveShieldsSetNoScriptControlTypeFunction
    : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveShields.setNoScriptControlType", UNKNOWN)

 protected:
  ~BraveShieldsSetNoScriptControlTypeFunction() override {}

  ResponseAction Run() override;
};

class BraveShieldsGetNoScriptControlTypeFunction
    : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveShields.getNoScriptControlType", UNKNOWN)

 protected:
  ~BraveShieldsGetNoScriptControlTypeFunction() override {}

  ResponseAction Run() override;
};

}  // namespace api
}  // namespace extensions

#endif  // BRAVE_BROWSER_EXTENSIONS_API_BRAVE_SHIELDS_API_H_
