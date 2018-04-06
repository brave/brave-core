/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMMON_BRAVE_PATHS_MAC_H_
#define BRAVE_COMMON_BRAVE_PATHS_MAC_H_

#import <Foundation/Foundation.h>

#include <string>

namespace brave {

std::string GetChannelSuffixForDataDir(NSBundle* chrome_bundle);

}  // namespace brave

#endif  // BRAVE_COMMON_BRAVE_PATHS_MAC_H_
