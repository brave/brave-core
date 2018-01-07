/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMMON_EXTENSIONS_API_BRAVE_SHIELDS_API_H_
#define BRAVE_COMMON_EXTENSIONS_API_BRAVE_SHIELDS_API_H_

#include "extensions/browser/extension_function.h"

namespace extensions {
namespace api {

class BraveShieldsDummyFunction : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveShields.dummy", UNKNOWN)

 protected:
  ~BraveShieldsDummyFunction() override;

  ResponseAction Run() override;
};

}  // namespace api
}  // namespace extensions

#endif  // BRAVE_COMMON_EXTENSIONS_API_BRAVE_SHIELDS_API_H_
