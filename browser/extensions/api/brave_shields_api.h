/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EXTENSIONS_API_BRAVE_SHIELDS_API_H_
#define BRAVE_BROWSER_EXTENSIONS_API_BRAVE_SHIELDS_API_H_

#include <memory>
#include <string>

#include "extensions/browser/extension_function.h"

namespace extensions {
namespace api {

class BraveShieldsAddSiteCosmeticFilterFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveShields.addSiteCosmeticFilter", UNKNOWN)

 protected:
  ~BraveShieldsAddSiteCosmeticFilterFunction() override {}

  ResponseAction Run() override;
};

class BraveShieldsOpenFilterManagementPageFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveShields.openFilterManagementPage", UNKNOWN)

 protected:
  ~BraveShieldsOpenFilterManagementPageFunction() override {}

  ResponseAction Run() override;
};

class BraveShieldsAllowScriptsOnceFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveShields.allowScriptsOnce", UNKNOWN)

 protected:
  ~BraveShieldsAllowScriptsOnceFunction() override {}

  ResponseAction Run() override;
};

class BraveShieldsOpenBrowserActionUIFunction :
    public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveShields.openBrowserActionUI", UNKNOWN)

 protected:
  ~BraveShieldsOpenBrowserActionUIFunction() override;

  ResponseAction Run() override;
};

class BraveShieldsSetBraveShieldsEnabledFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveShields.setBraveShieldsEnabled", UNKNOWN)

 protected:
  ~BraveShieldsSetBraveShieldsEnabledFunction() override {}

  ResponseAction Run() override;
};

class BraveShieldsGetBraveShieldsEnabledFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveShields.getBraveShieldsEnabled", UNKNOWN)

 protected:
  ~BraveShieldsGetBraveShieldsEnabledFunction() override {}

  ResponseAction Run() override;
};

class BraveShieldsSetCosmeticFilteringControlTypeFunction :
    public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveShields.setCosmeticFilteringControlType",
                             UNKNOWN)

 protected:
  ~BraveShieldsSetCosmeticFilteringControlTypeFunction() override {}

  ResponseAction Run() override;
};

class BraveShieldsIsFirstPartyCosmeticFilteringEnabledFunction :
    public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION(
      "braveShields.isFirstPartyCosmeticFilteringEnabled",
      UNKNOWN)

 protected:
  ~BraveShieldsIsFirstPartyCosmeticFilteringEnabledFunction() override {}

  ResponseAction Run() override;
};

class BraveShieldsSetAdControlTypeFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveShields.setAdControlType", UNKNOWN)

 protected:
  ~BraveShieldsSetAdControlTypeFunction() override {}

  ResponseAction Run() override;
};

class BraveShieldsGetAdControlTypeFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveShields.getAdControlType", UNKNOWN)

 protected:
  ~BraveShieldsGetAdControlTypeFunction() override {}

  ResponseAction Run() override;
};

class BraveShieldsSetCookieControlTypeFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveShields.setCookieControlType", UNKNOWN)

 protected:
  ~BraveShieldsSetCookieControlTypeFunction() override {}

  ResponseAction Run() override;
};

class BraveShieldsGetCookieControlTypeFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveShields.getCookieControlType", UNKNOWN)

 protected:
  ~BraveShieldsGetCookieControlTypeFunction() override {}

  ResponseAction Run() override;
};

class BraveShieldsSetFingerprintingControlTypeFunction
    : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveShields.setFingerprintingControlType",
                             UNKNOWN)

 protected:
  ~BraveShieldsSetFingerprintingControlTypeFunction() override {}

  ResponseAction Run() override;
};

class BraveShieldsGetFingerprintingControlTypeFunction
    : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveShields.getFingerprintingControlType",
                             UNKNOWN)

 protected:
  ~BraveShieldsGetFingerprintingControlTypeFunction() override {}

  ResponseAction Run() override;
};

class BraveShieldsSetHTTPSEverywhereEnabledFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveShields.setHTTPSEverywhereEnabled",
                             UNKNOWN)

 protected:
  ~BraveShieldsSetHTTPSEverywhereEnabledFunction() override {}

  ResponseAction Run() override;
};

class BraveShieldsGetHTTPSEverywhereEnabledFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveShields.getHTTPSEverywhereEnabled",
                             UNKNOWN)

 protected:
  ~BraveShieldsGetHTTPSEverywhereEnabledFunction() override {}

  ResponseAction Run() override;
};

class BraveShieldsSetNoScriptControlTypeFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveShields.setNoScriptControlType", UNKNOWN)

 protected:
  ~BraveShieldsSetNoScriptControlTypeFunction() override {}

  ResponseAction Run() override;
};

class BraveShieldsGetNoScriptControlTypeFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveShields.getNoScriptControlType", UNKNOWN)

 protected:
  ~BraveShieldsGetNoScriptControlTypeFunction() override {}

  ResponseAction Run() override;
};

// Notifies the browser that the shields panel was shown to the user.
class BraveShieldsOnShieldsPanelShownFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveShields.onShieldsPanelShown", UNKNOWN)

 protected:
  ~BraveShieldsOnShieldsPanelShownFunction() override {}

  // ExtensionFunction:
  ResponseAction Run() override;
};

class BraveShieldsReportBrokenSiteFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveShields.reportBrokenSite", UNKNOWN)

 protected:
  ~BraveShieldsReportBrokenSiteFunction() override {}

  ResponseAction Run() override;
};

}  // namespace api
}  // namespace extensions

#endif  // BRAVE_BROWSER_EXTENSIONS_API_BRAVE_SHIELDS_API_H_
