/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/shell_integration.h"

#include "base/apple/bundle_locations.h"
#include "base/apple/foundation_util.h"
#include "base/apple/scoped_cftyperef.h"
#include "base/mac/mac_util.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/components/brave_origin/buildflags/buildflags.h"
#include "build/branding_buildflags.h"
#include "chrome/common/channel_info.h"
#include "components/version_info/version_info.h"

// All above headers copied from original shell_integration_mac.mm are
// included to prevent below GOOGLE_CHROME_BUILD affect them.

#undef BUILDFLAG_INTERNAL_GOOGLE_CHROME_BRANDING
#define BUILDFLAG_INTERNAL_GOOGLE_CHROME_BRANDING() (1)

#define GetPlatformSpecificDefaultWebClientSetPermission \
  GetPlatformSpecificDefaultWebClientSetPermission_Unused
#define GetDefaultBrowser GetDefaultBrowser_ChromiumImpl
#define IsDefaultHandlerForUTType IsDefaultHandlerForUTType_ChromiumImpl
#include <chrome/browser/shell_integration_mac.mm>
#undef IsDefaultHandlerForUTType
#undef GetDefaultBrowser
#undef GetPlatformSpecificDefaultWebClientSetPermission
#undef BUILDFLAG_INTERNAL_GOOGLE_CHROME_BRANDING

namespace shell_integration {

namespace {

// Returns true if |identifier| is a Brave Origin bundle ID
// (com.brave.Browser.origin or com.brave.Browser.origin.<channel>).
bool IsBraveOriginBundleId(NSString* identifier) {
  return [identifier isEqualToString:@"com.brave.Browser.origin"] ||
         [identifier hasPrefix:@"com.brave.Browser.origin."];
}

#if !BUILDFLAG(IS_BRAVE_ORIGIN_BRANDED)
// Returns true if |identifier| is a regular (non-Origin) Brave bundle ID
// (com.brave.Browser or com.brave.Browser.<channel>, excluding Origin).
bool IsRegularBraveBundleId(NSString* identifier) {
  return ([identifier isEqualToString:@"com.brave.Browser"] ||
          [identifier hasPrefix:@"com.brave.Browser."]) &&
         !IsBraveOriginBundleId(identifier);
}
#endif

// Returns true if |other_identifier| is another channel of the same Brave
// brand as |my_identifier|. Unlike upstream's IsAnotherChromeChannel() which
// only compares the first 3 bundle ID components, this correctly distinguishes
// between regular Brave and Brave Origin which share the "com.brave.Browser"
// prefix.
bool IsAnotherBraveChannel(NSString* my_identifier,
                           NSString* other_identifier) {
#if BUILDFLAG(IS_BRAVE_ORIGIN_BRANDED)
  return IsBraveOriginBundleId(my_identifier) &&
         IsBraveOriginBundleId(other_identifier);
#else
  return IsRegularBraveBundleId(my_identifier) &&
         IsRegularBraveBundleId(other_identifier);
#endif
}

}  // namespace

DefaultWebClientState GetDefaultBrowser() {
  NSString* my_identifier = base::apple::OuterBundle().bundleIdentifier;
  if (!my_identifier) {
    return UNKNOWN_DEFAULT;
  }

  NSString* default_browser = GetBundleIdForDefaultAppForScheme(@"http");
  if ([default_browser isEqualToString:my_identifier]) {
    return IS_DEFAULT;
  }

  if (IsAnotherBraveChannel(my_identifier, default_browser)) {
    return OTHER_MODE_IS_DEFAULT;
  }
  return NOT_DEFAULT;
}

DefaultWebClientState IsDefaultHandlerForUTType(const std::string& type) {
  if (type.empty()) {
    return UNKNOWN_DEFAULT;
  }
  NSString* my_identifier = base::apple::OuterBundle().bundleIdentifier;
  if (!my_identifier) {
    return UNKNOWN_DEFAULT;
  }
  NSString* default_app =
      GetBundleIdForDefaultAppForUTType(base::SysUTF8ToNSString(type));
  if (!default_app) {
    return UNKNOWN_DEFAULT;
  }
  if ([default_app isEqualToString:my_identifier]) {
    return IS_DEFAULT;
  }
  if (IsAnotherBraveChannel(my_identifier, default_app)) {
    return OTHER_MODE_IS_DEFAULT;
  }
  return NOT_DEFAULT;
}

namespace internal {

DefaultWebClientSetPermission GetPlatformSpecificDefaultWebClientSetPermission(
    WebClientSetMethod method) {
  return SET_DEFAULT_UNATTENDED;
}

}  // namespace internal

}  // namespace shell_integration
