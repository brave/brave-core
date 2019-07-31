/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
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
  ~BraveShieldsAllowScriptsOnceFunction() override {}

  ResponseAction Run() override;
};

class BraveShieldsSetBraveShieldsControlTypeFunction
    : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveShields.setBraveShieldsControlType", UNKNOWN)

 protected:
  ~BraveShieldsSetBraveShieldsControlTypeFunction() override {}

  ResponseAction Run() override;
};

class BraveShieldsGetBraveShieldsControlTypeFunction
    : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveShields.getBraveShieldsControlType", UNKNOWN)

 protected:
  ~BraveShieldsGetBraveShieldsControlTypeFunction() override {}

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

class BraveShieldsSetHTTPSEverywhereControlTypeFunction
    : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveShields.setHTTPSEverywhereControlType",
                             UNKNOWN)

 protected:
  ~BraveShieldsSetHTTPSEverywhereControlTypeFunction() override {}

  ResponseAction Run() override;
};

class BraveShieldsGetHTTPSEverywhereControlTypeFunction
    : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveShields.getHTTPSEverywhereControlType",
                             UNKNOWN)

 protected:
  ~BraveShieldsGetHTTPSEverywhereControlTypeFunction() override {}

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
