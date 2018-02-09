/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/brave_shields_api.h"

namespace extensions {
namespace api {

BraveShieldsDummyFunction::~BraveShieldsDummyFunction() {
}

ExtensionFunction::ResponseAction BraveShieldsDummyFunction::Run() {
  return RespondNow(NoArguments());
}

}  // namespace api
}  // namespace extensions
