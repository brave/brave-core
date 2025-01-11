/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EXTENSIONS_API_WEB_DISCOVERY_API_H_
#define BRAVE_BROWSER_EXTENSIONS_API_WEB_DISCOVERY_API_H_

#include "extensions/browser/extension_function.h"

namespace extensions {
namespace api {

class WebDiscoveryIsWebDiscoveryNativeEnabledFunction
    : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("webDiscovery.isWebDiscoveryNativeEnabled",
                             UNKNOWN)

 protected:
  ~WebDiscoveryIsWebDiscoveryNativeEnabledFunction() override {}

  ResponseAction Run() override;
};

}  // namespace api
}  // namespace extensions

#endif  // BRAVE_BROWSER_EXTENSIONS_API_WEB_DISCOVERY_API_H_
