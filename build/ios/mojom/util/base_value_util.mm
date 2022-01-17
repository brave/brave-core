/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base_value_util.h"

#include "base/strings/sys_string_conversions.h"
#include "base/values.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace brave {

NSObject* NSObjectFromBaseValue(base::Value value) {
  switch (value.type()) {
    case base::Value::Type::BOOLEAN:
      return [NSNumber numberWithBool:value.GetBool()];
    case base::Value::Type::INTEGER:
      return [NSNumber numberWithInt:value.GetInt()];
    case base::Value::Type::DOUBLE:
      return [NSNumber numberWithDouble:value.GetDouble()];
    case base::Value::Type::STRING:
      return base::SysUTF8ToNSString(value.GetString());
    case base::Value::Type::BINARY: {
      std::vector<uint8_t> blob = value.GetBlob();
      return [[NSData alloc] initWithBytes:blob.data() length:blob.size()];
    }
    case base::Value::Type::DICTIONARY:
      return NSDictionaryFromBaseValue(value.Clone());
    case base::Value::Type::LIST:
      return NSArrayFromBaseValue(value.Clone());
    case base::Value::Type::NONE:
    default:
      return [NSNull null];
  }
}

NSArray* NSArrayFromBaseValue(base::Value value) {
  auto result = [[NSMutableArray alloc] init];
  for (auto&& item : value.GetList()) {
    [result addObject:NSObjectFromBaseValue(item.Clone())];
  }
  return result;
}

NSDictionary<NSString*, id>* NSDictionaryFromBaseValue(base::Value value) {
  auto result = [[NSMutableDictionary alloc] init];
  for (auto kv : value.DictItems()) {
    result[base::SysUTF8ToNSString(kv.first)] =
        NSObjectFromBaseValue(kv.second.Clone());
  }
  return result;
}

base::Value BaseValueFromNSObject(NSObject* value) {
  // NSString -> String
  if ([value isKindOfClass:NSString.class]) {
    return base::Value(base::SysNSStringToUTF8(static_cast<NSString*>(value)));
  }
  // NSNumber -> {Bool, Int, Double}
  else if ([value isKindOfClass:NSNumber.class]) {
    const auto number = static_cast<NSNumber*>(value);
    std::string type = number.objCType;
    if (type == std::string(@encode(bool))) {
      return base::Value(number.boolValue);
    } else if (type == std::string(@encode(double)) ||
               type == std::string(@encode(float))) {
      return base::Value(number.doubleValue);
    } else {
      return base::Value(number.intValue);
    }
  }
  // NSArray -> List
  else if ([value isKindOfClass:NSArray.self]) {
    return BaseValueFromNSArray(static_cast<NSArray*>(value));
  }
  // NSDictionary -> Dictionary
  else if ([value isKindOfClass:NSDictionary.self]) {
    return BaseValueFromNSDictionary(static_cast<NSDictionary*>(value));
  }
  // NSData -> BlobStorage (vector<uint8>)
  else if ([value isKindOfClass:NSData.self]) {
    const auto data = static_cast<NSData*>(value);
    std::vector<uint8_t> blob(
        static_cast<const uint8_t*>(data.bytes),
        static_cast<const uint8_t*>(data.bytes) + data.length);
    return base::Value(blob);
  }
  // NSNull -> None
  else if ([value isKindOfClass:NSNull.class]) {
    return base::Value(base::Value::Type::NONE);
  }
  // Unsupported
  return base::Value(base::Value::Type::NONE);
}

base::Value BaseValueFromNSArray(NSArray* array) {
  base::Value list(base::Value::Type::LIST);
  for (NSObject* obj in array) {
    list.Append(BaseValueFromNSObject(obj));
  }
  return list;
}

base::Value BaseValueFromNSDictionary(NSDictionary<NSString*, id>* dictionary) {
  base::Value dict(base::Value::Type::DICTIONARY);
  for (NSString* key in dictionary) {
    NSObject* value = dictionary[key];
    dict.SetKey(base::SysNSStringToUTF8(key), BaseValueFromNSObject(value));
  }
  return dict;
}

}  // namespace brave
