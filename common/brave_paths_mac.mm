/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/common/brave_paths_mac.h"

#include "base/logging.h"
#include "base/strings/sys_string_conversions.h"

namespace brave {

std::string GetChannelSuffixForDataDir(NSBundle* chrome_bundle) {
#if defined(OFFICIAL_BUILD)
  // chrome::GetChannelString() returns valid value only at first call.
  // From second call, it returns empty string. So, reimplemented for fetching
  // channel string here.
  NSString* channel = [chrome_bundle objectForInfoDictionaryKey:@"KSChannelID"];
  NSMutableString* suffix = [@"-" mutableCopy];

  if (!channel) {
    // For the stable channel, KSChannelID is not set.
    return std::string();
  } else if ([channel isEqual:@"beta"] ||
             [channel isEqual:@"dev"] ||
             [channel isEqual:@"canary"]) {
    [suffix appendString:channel];
  } else {
    NOTREACHED();
    [suffix appendString:@"unkown"];
  }

  std::string result = base::SysNSStringToUTF8(suffix);
  result[1] = std::toupper(result[1]);
  return result;
#else  // OFFICIAL_BUILD
  return "-Development";
#endif
}

}  // namespace brave
