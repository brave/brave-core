/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BASE_APPLE_BASE_VALUE_BRIDGE_PRIVATE_H_
#define BRAVE_BASE_APPLE_BASE_VALUE_BRIDGE_PRIVATE_H_

#include "base/values.h"
#include "brave/base/apple/base_value_bridge.h"

@class BaseValueBridge;

namespace brave {

// Clone the contents of a `base::Value` whos type is `base::Value::Type::LIST`
// into an Obj-C NSArray container. Any types found within the `base::Value`
// that are unsupported or `NONE` will become `NSNull`
NSArray<BaseValueBridge*>* NSArrayFromBaseValue(const base::Value);

// Clone the contents of a `base::Value` whos type is
// `base::Value::Type::DICT`.  Any types found within the `base::Value`
// that are unsupported or `NONE` will become `NSNull`
NSDictionary<NSString*, BaseValueBridge*>* NSDictionaryFromBaseValue(
    const base::Value);

// Clone the contents of an NSArray into a `base::Value` with the type
// `base::Value::Type::LIST`
base::Value BaseValueFromNSArray(NSArray<BaseValueBridge*>*);

// Clone the contents of an NSArray into a `base::ListValue`
base::ListValue BaseValueListFromNSArray(NSArray<BaseValueBridge*>*);

// Clone the contents of an NSDictionary into a `base::Value` with the type
// `base::Value::Type::DICT`
base::Value BaseValueFromNSDictionary(
    NSDictionary<NSString*, BaseValueBridge*>*);

NSDictionary<NSString*, BaseValueBridge*>* NSDictionaryFromBaseValueDict(
    const base::DictValue);
base::DictValue BaseValueDictFromNSDictionary(
    NSDictionary<NSString*, BaseValueBridge*>*);

}  // namespace brave

@interface BaseValueBridge (Private)
- (instancetype)initWithValue:(const base::Value)value;
- (base::Value)value;
@end

#endif  // BRAVE_BASE_APPLE_BASE_VALUE_BRIDGE_PRIVATE_H_
