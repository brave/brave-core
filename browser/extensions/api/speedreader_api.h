/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EXTENSIONS_API_SPEEDREADER_API_H_
#define BRAVE_BROWSER_EXTENSIONS_API_SPEEDREADER_API_H_

#include "extensions/browser/extension_function.h"

namespace extensions {
namespace api {

class SpeedreaderDisableOnceFunction : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("speedreader.disableOnce", UNKNOWN)

 protected:
  ~SpeedreaderDisableOnceFunction() override;

  ResponseAction Run() override;
};

class SpeedreaderGetFunction
    : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("speedreader.get", UNKNOWN)

 protected:
  ~SpeedreaderGetFunction() override {}

  // ExtensionFunction:
  ResponseAction Run() override;
};

class SpeedreaderSetFunction
    : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("speedreader.set", UNKNOWN)

 protected:
  ~SpeedreaderSetFunction() override {}

  // ExtensionFunction:
  ResponseAction Run() override;
};

}  // namespace api
}  // namespace extensions

#endif  // BRAVE_BROWSER_EXTENSIONS_API_SPEEDREADER_API_H_
