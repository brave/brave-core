/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <Foundation/Foundation.h>

#ifndef BRAVE_BUILD_IOS_MOJOM_UTIL_BASE_VALUE_UTIL_H_
#define BRAVE_BUILD_IOS_MOJOM_UTIL_BASE_VALUE_UTIL_H_

namespace base {
class Value;
}  // namespace base

namespace brave {

// Clone the contents of a `base::Value` whos type is `base::Value::Type::LIST`
// into an Obj-C NSArray container. Any types found within the `base::Value`
// that are unsupported or `NONE` will become `NSNull`
NSArray* NSArrayFromBaseValue(const base::Value);

// Clone the contents of a `base::Value` whos type is
// `base::Value::Type::DICTIONARY`.  Any types found within the `base::Value`
// that are unsupported or `NONE` will become `NSNull`
NSDictionary<NSString*, id>* NSDictionaryFromBaseValue(const base::Value);

// Clone the contents of an NSArray into a `base::Value` with the type
// `base::Value::Type::LIST`
base::Value BaseValueFromNSArray(NSArray*);

// Clone the contents of an NSDictionary into a `base::Value` with the type
// `base::Value::Type::DICTIONARY`
base::Value BaseValueFromNSDictionary(NSDictionary<NSString*, id>*);

}  // namespace brave

#endif  // BRAVE_BUILD_IOS_MOJOM_UTIL_BASE_VALUE_UTIL_H_
