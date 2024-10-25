/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BUILD_IOS_MOJOM_PUBLIC_BASE_BASE_VALUES_PRIVATE_H_
#define BRAVE_BUILD_IOS_MOJOM_PUBLIC_BASE_BASE_VALUES_PRIVATE_H_

#include "base/values.h"
#include "brave/build/ios/mojom/public/base/base_values.h"

@class MojoBaseValue;

namespace brave {

// Clone the contents of a `base::Value` whos type is `base::Value::Type::LIST`
// into an Obj-C NSArray container. Any types found within the `base::Value`
// that are unsupported or `NONE` will become `NSNull`
NSArray<MojoBaseValue*>* NSArrayFromBaseValue(const base::Value);

// Clone the contents of a `base::Value` whos type is
// `base::Value::Type::DICT`.  Any types found within the `base::Value`
// that are unsupported or `NONE` will become `NSNull`
NSDictionary<NSString*, MojoBaseValue*>* NSDictionaryFromBaseValue(
    const base::Value);

// Clone the contents of an NSArray into a `base::Value` with the type
// `base::Value::Type::LIST`
base::Value BaseValueFromNSArray(NSArray<MojoBaseValue*>*);

// Clone the contents of an NSArray into a `base::Value::List`
base::Value::List BaseValueListFromNSArray(NSArray<MojoBaseValue*>*);

// Clone the contents of an NSDictionary into a `base::Value` with the type
// `base::Value::Type::DICT`
base::Value BaseValueFromNSDictionary(NSDictionary<NSString*, MojoBaseValue*>*);

NSDictionary<NSString*, MojoBaseValue*>* NSDictionaryFromBaseValueDict(
    const base::Value::Dict);
base::Value::Dict BaseValueDictFromNSDictionary(
    NSDictionary<NSString*, MojoBaseValue*>*);

}  // namespace brave

@interface MojoBaseValue (Private)
- (instancetype)initWithValue:(const base::Value)value;
- (base::Value)cppObjPtr;
@end

#endif  // BRAVE_BUILD_IOS_MOJOM_PUBLIC_BASE_BASE_VALUES_PRIVATE_H_
