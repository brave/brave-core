// Copyright 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

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
