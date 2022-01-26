/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/certificate/utils/brave_certificate_utils.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/sys_string_conversions.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace certificate {
namespace utils {
NSData* NSDataFromString(const std::string& str) {
  return str.empty()
             ? [[NSData alloc] init]
             : [NSData dataWithBytes:reinterpret_cast<const std::uint8_t*>(
                                         str.c_str())
                              length:str.size()];
}
}  // namespace utils
}  // namespace certificate
