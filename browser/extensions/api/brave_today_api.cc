// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/extensions/api/brave_today_api.h"

#include <memory>
#include <string>

#include "base/json/json_writer.h"
#include "base/values.h"
#include "brave/common/extensions/api/brave_theme.h"
#include "brave/components/brave_today/common/urls.h"

namespace extensions {
namespace api {

ExtensionFunction::ResponseAction BraveTodayGetHostnameFunction::Run() {
  return RespondNow(OneArgument(base::Value(brave_today::GetHostname())));
}

}  // namespace api
}  // namespace extensions
