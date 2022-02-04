/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_CERTIFICATE_UTILS_BRAVE_CERTIFICATE_UTILS_H_
#define BRAVE_IOS_BROWSER_API_CERTIFICATE_UTILS_BRAVE_CERTIFICATE_UTILS_H_

#import <Foundation/Foundation.h>
#include <string>

namespace certificate {
namespace utils {
NSData* NSStringToData(const std::string& str);
}  // namespace utils
}  // namespace certificate

#endif  //  BRAVE_IOS_BROWSER_API_CERTIFICATE_UTILS_BRAVE_CERTIFICATE_UTILS_H_
