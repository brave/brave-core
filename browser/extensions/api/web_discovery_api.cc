/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/web_discovery_api.h"

#include "brave/components/web_discovery/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_WEB_DISCOVERY_NATIVE)
#include "base/feature_list.h"
#include "brave/components/web_discovery/common/features.h"
#endif

namespace extensions::api {

ExtensionFunction::ResponseAction
WebDiscoveryIsWebDiscoveryNativeEnabledFunction::Run() {
  bool result = false;
#if BUILDFLAG(ENABLE_WEB_DISCOVERY_NATIVE)
  result = base::FeatureList::IsEnabled(
      web_discovery::features::kBraveWebDiscoveryNative);
#endif
  return RespondNow(WithArguments(result));
}

}  // namespace extensions::api
