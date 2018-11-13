/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/themes/brave_theme_util.h"

#include <Cocoa/Cocoa.h>

#if !defined(MAC_OS_X_VERSION_10_14) || \
    MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_14

typedef NSString* NSAppearanceName;

@interface NSApplication (ForwardDeclare)
@property(readonly, strong) NSAppearance* effectiveAppearance;
@end

@interface NSAppearance (ForwardDeclare)
- (NSAppearanceName)bestMatchFromAppearancesWithNames:
    (NSArray<NSAppearanceName>*)appearances;
@end

#endif

namespace brave {

bool GetPlatformThemeType(BraveThemeType* type) {
  if (@available(macOS 10.14, *)) {
    NSAppearanceName appearance =
        [[NSApp effectiveAppearance] bestMatchFromAppearancesWithNames:@[
          NSAppearanceNameAqua, NSAppearanceNameDarkAqua
        ]];
    *type =
        [appearance isEqual:NSAppearanceNameDarkAqua] ? BRAVE_THEME_TYPE_DARK
                                                      : BRAVE_THEME_TYPE_LIGHT;
    return true;
  }
  return false;
}

}  // namespace brave
