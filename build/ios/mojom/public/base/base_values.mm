/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/build/ios/mojom/public/base/base_values.h"

#include <optional>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/build/ios/mojom/public/base/base_values+private.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface MojoBaseValue ()
@property(nonatomic, readwrite) MojoBaseValueTag tag;
@end

@implementation MojoBaseValue

@synthesize boolValue = _boolValue;
@synthesize intValue = _intValue;
@synthesize doubleValue = _doubleValue;
@synthesize stringValue = _stringValue;
@synthesize binaryValue = _binaryValue;
@synthesize dictionaryValue = _dictionaryValue;
@synthesize listValue = _listValue;

- (instancetype)init {
  if ((self = [super init])) {
    self.tag = MojoBaseValueTagNull;
  }
  return self;
}

- (instancetype)initWithBoolValue:(bool)value {
  if ((self = [super init])) {
    self.tag = MojoBaseValueTagBoolValue;
    self.boolValue = value;
  }
  return self;
}

- (bool)boolValue {
  if (self.tag == MojoBaseValueTagBoolValue) {
    return _boolValue;
  }
  return 0;
}

- (void)setBoolValue:(bool)value {
  self.tag = MojoBaseValueTagBoolValue;
  _boolValue = value;
}

- (instancetype)initWithIntValue:(int32_t)value {
  if ((self = [super init])) {
    self.tag = MojoBaseValueTagIntValue;
    self.intValue = value;
  }
  return self;
}

- (int32_t)intValue {
  if (self.tag == MojoBaseValueTagIntValue) {
    return _intValue;
  }
  return 0;
}

- (void)setIntValue:(int32_t)value {
  self.tag = MojoBaseValueTagIntValue;
  _intValue = value;
}

- (instancetype)initWithDoubleValue:(double)value {
  if ((self = [super init])) {
    self.tag = MojoBaseValueTagDoubleValue;
    self.doubleValue = value;
  }
  return self;
}

- (double)doubleValue {
  if (self.tag == MojoBaseValueTagDoubleValue) {
    return _doubleValue;
  }
  return 0;
}

- (void)setDoubleValue:(double)value {
  self.tag = MojoBaseValueTagDoubleValue;
  _doubleValue = value;
}

- (instancetype)initWithStringValue:(NSString*)value {
  if ((self = [super init])) {
    self.tag = MojoBaseValueTagStringValue;
    self.stringValue = value;
  }
  return self;
}

- (NSString*)stringValue {
  if (self.tag == MojoBaseValueTagStringValue) {
    return _stringValue;
  }
  return 0;
}

- (void)setStringValue:(NSString*)value {
  self.tag = MojoBaseValueTagStringValue;
  _stringValue = [value copy];
}

- (instancetype)initWithBinaryValue:(NSArray<NSNumber*>*)value {
  if ((self = [super init])) {
    self.tag = MojoBaseValueTagBinaryValue;
    self.binaryValue = value;
  }
  return self;
}

- (NSArray<NSNumber*>*)binaryValue {
  if (self.tag == MojoBaseValueTagBinaryValue) {
    return _binaryValue;
  }
  return 0;
}

- (void)setBinaryValue:(NSArray<NSNumber*>*)value {
  self.tag = MojoBaseValueTagBinaryValue;
  _binaryValue = [value copy];
}

- (instancetype)initWithDictionaryValue:
    (NSDictionary<NSString*, MojoBaseValue*>*)value {
  if ((self = [super init])) {
    self.tag = MojoBaseValueTagDictionaryValue;
    self.dictionaryValue = value;
  }
  return self;
}

- (NSDictionary<NSString*, MojoBaseValue*>*)dictionaryValue {
  if (self.tag == MojoBaseValueTagDictionaryValue) {
    return _dictionaryValue;
  }
  return 0;
}

- (void)setDictionaryValue:(NSDictionary<NSString*, MojoBaseValue*>*)value {
  self.tag = MojoBaseValueTagDictionaryValue;
  _dictionaryValue = [value copy];
}

- (instancetype)initWithListValue:(NSArray*)value {
  if ((self = [super init])) {
    self.tag = MojoBaseValueTagListValue;
    self.listValue = value;
  }
  return self;
}

- (NSArray<MojoBaseValue*>*)listValue {
  if (self.tag == MojoBaseValueTagListValue) {
    return _listValue;
  }
  return 0;
}

- (void)setListValue:(NSArray<MojoBaseValue*>*)value {
  self.tag = MojoBaseValueTagListValue;
  _listValue = [value copy];
}

- (id)copyWithZone:(nullable NSZone*)zone {
  auto copy = [[MojoBaseValue alloc] init];
  copy.tag = self.tag;
  copy.boolValue = self.boolValue;
  copy.intValue = self.intValue;
  copy.doubleValue = self.doubleValue;
  copy.stringValue = self.stringValue;
  copy.binaryValue = self.binaryValue;
  copy.dictionaryValue = self.dictionaryValue;
  copy.listValue = self.listValue;
  return copy;
}

- (instancetype)initWithValue:(const base::Value)value {
  if ((self = [super init])) {
    switch (value.type()) {
      case base::Value::Type::BOOLEAN:
        self.boolValue = value.GetBool();
        break;
      case base::Value::Type::INTEGER:
        self.intValue = value.GetInt();
        break;
      case base::Value::Type::DOUBLE:
        self.doubleValue = value.GetDouble();
        break;
      case base::Value::Type::STRING:
        self.stringValue = base::SysUTF8ToNSString(value.GetString());
        break;
      case base::Value::Type::BINARY:
        self.binaryValue = [param = std::cref(value.GetBlob())] {
          const auto a = [[NSMutableArray alloc] init];
          for (const auto& o : param.get()) {
            [a addObject:@(o)];
          }
          return a;
        }();
        break;
      case base::Value::Type::DICT:
        self.dictionaryValue = brave::NSDictionaryFromBaseValue(value.Clone());
        break;
      case base::Value::Type::LIST:
        self.listValue = brave::NSArrayFromBaseValue(value.Clone());
        break;
      case base::Value::Type::NONE:
      default:
        self.tag = MojoBaseValueTagNull;
        break;
    }
  }
  return self;
}

- (base::Value)cppObjPtr {
  switch (self.tag) {
    case MojoBaseValueTagBoolValue: {
      return base::Value(self.boolValue);
    }
    case MojoBaseValueTagIntValue: {
      return base::Value(self.intValue);
    }
    case MojoBaseValueTagDoubleValue: {
      return base::Value(self.doubleValue);
    }
    case MojoBaseValueTagStringValue: {
      return base::Value(
          base::SysNSStringToUTF8(static_cast<NSString*>(self.stringValue)));
    }
    case MojoBaseValueTagBinaryValue: {
      std::vector<uint8_t> blob;
      for (NSNumber* obj in self.binaryValue) {
        blob.push_back(obj.unsignedCharValue);
      }
      return base::Value(blob);
    }
    case MojoBaseValueTagDictionaryValue: {
      return brave::BaseValueFromNSDictionary(self.dictionaryValue);
    }
    case MojoBaseValueTagListValue: {
      return brave::BaseValueFromNSArray(self.listValue);
    }
    case MojoBaseValueTagNull:
    default:
      return base::Value(base::Value::Type::NONE);
  }
}

@end

@implementation MojoBaseValue (JSON)

- (nullable instancetype)initWithJSONString:(NSString*)json {
  auto string = base::SysNSStringToUTF8(json);
  std::optional<base::Value> response = base::JSONReader::Read(
      string, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                  base::JSONParserOptions::JSON_PARSE_RFC);
  if (response) {
    return [self initWithValue:response->Clone()];
  }
  return nil;
}

- (nullable NSString*)jsonString {
  std::string input_json;
  if (!base::JSONWriter::Write(self.cppObjPtr, &input_json) ||
      input_json.empty()) {
    return nil;
  }
  return base::SysUTF8ToNSString(input_json);
}

- (nullable id)jsonObject {
  NSData* jsonData = [self.jsonString dataUsingEncoding:NSUTF8StringEncoding];
  if (!jsonData) {
    return nil;
  }
  NSError* jsonError = nil;
  id object =
      [NSJSONSerialization JSONObjectWithData:jsonData
                                      options:NSJSONReadingFragmentsAllowed
                                        error:&jsonError];
  if (jsonError || !object) {
    return nil;
  }
  return object;
}

@end

namespace brave {

NSArray<MojoBaseValue*>* NSArrayFromBaseValue(base::Value value) {
  auto result = [[NSMutableArray alloc] init];
  for (auto&& item : value.GetList()) {
    [result addObject:[[MojoBaseValue alloc] initWithValue:item.Clone()]];
  }
  return result;
}

NSDictionary<NSString*, MojoBaseValue*>* NSDictionaryFromBaseValue(
    base::Value value) {
  auto result = [[NSMutableDictionary alloc] init];
  for (auto kv : value.GetDict()) {
    result[base::SysUTF8ToNSString(kv.first)] =
        [[MojoBaseValue alloc] initWithValue:kv.second.Clone()];
  }
  return result;
}

base::Value BaseValueFromNSArray(NSArray<MojoBaseValue*>* array) {
  base::Value value(base::Value::Type::LIST);
  base::Value::List& list = *value.GetIfList();
  for (MojoBaseValue* obj in array) {
    list.Append(obj.cppObjPtr);
  }
  return value;
}

base::Value::List BaseValueListFromNSArray(NSArray<MojoBaseValue*>* array) {
  base::Value::List value;
  for (MojoBaseValue* obj in array) {
    value.Append(obj.cppObjPtr);
  }
  return value;
}

base::Value BaseValueFromNSDictionary(
    NSDictionary<NSString*, MojoBaseValue*>* dictionary) {
  base::Value result(base::Value::Type::DICT);
  base::Value::Dict& dict = result.GetDict();
  for (NSString* key in dictionary) {
    MojoBaseValue* value = dictionary[key];
    dict.Set(base::SysNSStringToUTF8(key), value.cppObjPtr);
  }
  return result;
}

NSDictionary<NSString*, MojoBaseValue*>* NSDictionaryFromBaseValueDict(
    base::Value::Dict value) {
  auto result = [[NSMutableDictionary alloc] init];
  for (auto kv : value) {
    result[base::SysUTF8ToNSString(kv.first)] =
        [[MojoBaseValue alloc] initWithValue:kv.second.Clone()];
  }
  return result;
}

base::Value::Dict BaseValueDictFromNSDictionary(
    NSDictionary<NSString*, MojoBaseValue*>* dictionary) {
  base::Value::Dict dict;
  for (NSString* key in dictionary) {
    MojoBaseValue* value = dictionary[key];
    dict.Set(base::SysNSStringToUTF8(key), value.cppObjPtr);
  }
  return dict;
}

}  // namespace brave
