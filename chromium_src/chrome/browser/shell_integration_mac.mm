/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/mac/bundle_locations.h"
#include "base/mac/foundation_util.h"
#include "base/mac/mac_util.h"
#include "base/mac/scoped_cftyperef.h"
#include "base/strings/sys_string_conversions.h"
#include "build/branding_buildflags.h"
#include "chrome/common/channel_info.h"
#include "components/version_info/version_info.h"
#import "third_party/mozilla/NSWorkspace+Utils.h"

// All above headers copied from original shell_integration_mac.mm are
// included to prevent below GOOGLE_CHROME_BUILD affect them.

#undef BUILDFLAG_INTERNAL_GOOGLE_CHROME_BRANDING
#define BUILDFLAG_INTERNAL_GOOGLE_CHROME_BRANDING() (1)

#define GetDefaultWebClientSetPermission GetDefaultWebClientSetPermission_Unused
#include "src/chrome/browser/shell_integration_mac.mm"
#undef GetDefaultWebClientSetPermission
#undef BUILDFLAG_INTERNAL_GOOGLE_CHROME_BRANDING

namespace shell_integration {

DefaultWebClientSetPermission GetDefaultWebClientSetPermission() {
  return SET_DEFAULT_UNATTENDED;
}

}  // shell_integration
