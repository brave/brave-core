/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/base/apple/base_value_bridge.h"

#include <optional>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/base/apple/base_value_bridge+private.h"

@interface BaseValueBridge ()
@property(nonatomic, readwrite) BaseValueBridgeTag tag;
@end

@implementation BaseValueBridge

@synthesize boolValue = _boolValue;
@synthesize intValue = _intValue;
@synthesize doubleValue = _doubleValue;
@synthesize stringValue = _stringValue;
@synthesize binaryValue = _binaryValue;
@synthesize dictionaryValue = _dictionaryValue;
@synthesize listValue = _listValue;

- (instancetype)init {
  if ((self = [super init])) {
    self.tag = BaseValueBridgeTagNull;
  }
  return self;
}

- (instancetype)initWithBoolValue:(bool)value {
  if ((self = [super init])) {
    self.tag = BaseValueBridgeTagBoolValue;
    self.boolValue = value;
  }
  return self;
}

- (bool)boolValue {
  if (self.tag == BaseValueBridgeTagBoolValue) {
    return _boolValue;
  }
  return 0;
}

- (void)setBoolValue:(bool)value {
  self.tag = BaseValueBridgeTagBoolValue;
  _boolValue = value;
}

- (instancetype)initWithIntValue:(int32_t)value {
  if ((self = [super init])) {
    self.tag = BaseValueBridgeTagIntValue;
    self.intValue = value;
  }
  return self;
}

- (int32_t)intValue {
  if (self.tag == BaseValueBridgeTagIntValue) {
    return _intValue;
  }
  return 0;
}

- (void)setIntValue:(int32_t)value {
  self.tag = BaseValueBridgeTagIntValue;
  _intValue = value;
}

- (instancetype)initWithDoubleValue:(double)value {
  if ((self = [super init])) {
    self.tag = BaseValueBridgeTagDoubleValue;
    self.doubleValue = value;
  }
  return self;
}

- (double)doubleValue {
  if (self.tag == BaseValueBridgeTagDoubleValue) {
    return _doubleValue;
  }
  return 0;
}

- (void)setDoubleValue:(double)value {
  self.tag = BaseValueBridgeTagDoubleValue;
  _doubleValue = value;
}

- (instancetype)initWithStringValue:(NSString*)value {
  if ((self = [super init])) {
    self.tag = BaseValueBridgeTagStringValue;
    self.stringValue = value;
  }
  return self;
}

- (NSString*)stringValue {
  if (self.tag == BaseValueBridgeTagStringValue) {
    return _stringValue;
  }
  return 0;
}

- (void)setStringValue:(NSString*)value {
  self.tag = BaseValueBridgeTagStringValue;
  _stringValue = [value copy];
}

- (instancetype)initWithBinaryValue:(NSArray<NSNumber*>*)value {
  if ((self = [super init])) {
    self.tag = BaseValueBridgeTagBinaryValue;
    self.binaryValue = value;
  }
  return self;
}

- (NSArray<NSNumber*>*)binaryValue {
  if (self.tag == BaseValueBridgeTagBinaryValue) {
    return _binaryValue;
  }
  return 0;
}

- (void)setBinaryValue:(NSArray<NSNumber*>*)value {
  self.tag = BaseValueBridgeTagBinaryValue;
  _binaryValue = [value copy];
}

- (instancetype)initWithDictionaryValue:
    (NSDictionary<NSString*, BaseValueBridge*>*)value {
  if ((self = [super init])) {
    self.tag = BaseValueBridgeTagDictionaryValue;
    self.dictionaryValue = value;
  }
  return self;
}

- (NSDictionary<NSString*, BaseValueBridge*>*)dictionaryValue {
  if (self.tag == BaseValueBridgeTagDictionaryValue) {
    return _dictionaryValue;
  }
  return 0;
}

- (void)setDictionaryValue:(NSDictionary<NSString*, BaseValueBridge*>*)value {
  self.tag = BaseValueBridgeTagDictionaryValue;
  _dictionaryValue = [value copy];
}

- (instancetype)initWithListValue:(NSArray*)value {
  if ((self = [super init])) {
    self.tag = BaseValueBridgeTagListValue;
    self.listValue = value;
  }
  return self;
}

- (NSArray<BaseValueBridge*>*)listValue {
  if (self.tag == BaseValueBridgeTagListValue) {
    return _listValue;
  }
  return 0;
}

- (void)setListValue:(NSArray<BaseValueBridge*>*)value {
  self.tag = BaseValueBridgeTagListValue;
  _listValue = [value copy];
}

- (id)copyWithZone:(nullable NSZone*)zone {
  auto copy = [[BaseValueBridge alloc] init];
  switch (self.tag) {
    case BaseValueBridgeTagBoolValue:
      copy.boolValue = self.boolValue;
      break;
    case BaseValueBridgeTagIntValue:
      copy.intValue = self.intValue;
      break;
    case BaseValueBridgeTagDoubleValue:
      copy.doubleValue = self.doubleValue;
      break;
    case BaseValueBridgeTagStringValue:
      copy.stringValue = self.stringValue;
      break;
    case BaseValueBridgeTagBinaryValue:
      copy.binaryValue = self.binaryValue;
      break;
    case BaseValueBridgeTagDictionaryValue:
      copy.dictionaryValue = self.dictionaryValue;
      break;
    case BaseValueBridgeTagListValue:
      copy.listValue = self.listValue;
      break;
    case BaseValueBridgeTagNull:
      copy.tag = self.tag;
      break;
    default:
      copy.tag = self.tag;
      break;
  }
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
        self.tag = BaseValueBridgeTagNull;
        break;
    }
  }
  return self;
}

- (base::Value)value {
  switch (self.tag) {
    case BaseValueBridgeTagBoolValue: {
      return base::Value(self.boolValue);
    }
    case BaseValueBridgeTagIntValue: {
      return base::Value(self.intValue);
    }
    case BaseValueBridgeTagDoubleValue: {
      return base::Value(self.doubleValue);
    }
    case BaseValueBridgeTagStringValue: {
      return base::Value(
          base::SysNSStringToUTF8(static_cast<NSString*>(self.stringValue)));
    }
    case BaseValueBridgeTagBinaryValue: {
      std::vector<uint8_t> blob;
      for (NSNumber* obj in self.binaryValue) {
        blob.push_back(obj.unsignedCharValue);
      }
      return base::Value(blob);
    }
    case BaseValueBridgeTagDictionaryValue: {
      return brave::BaseValueFromNSDictionary(self.dictionaryValue);
    }
    case BaseValueBridgeTagListValue: {
      return brave::BaseValueFromNSArray(self.listValue);
    }
    case BaseValueBridgeTagNull:
    default:
      return base::Value(base::Value::Type::NONE);
  }
}

@end

@implementation BaseValueBridge (JSON)

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
  std::optional<std::string> input_json = base::WriteJson(self.value);
  if (!input_json || input_json->empty()) {
    return nil;
  }
  return base::SysUTF8ToNSString(*input_json);
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

NSArray<BaseValueBridge*>* NSArrayFromBaseValue(base::Value value) {
  auto result = [[NSMutableArray alloc] init];
  for (auto&& item : value.GetList()) {
    [result addObject:[[BaseValueBridge alloc] initWithValue:item.Clone()]];
  }
  return result;
}

NSDictionary<NSString*, BaseValueBridge*>* NSDictionaryFromBaseValue(
    base::Value value) {
  auto result = [[NSMutableDictionary alloc] init];
  for (auto kv : value.GetDict()) {
    result[base::SysUTF8ToNSString(kv.first)] =
        [[BaseValueBridge alloc] initWithValue:kv.second.Clone()];
  }
  return result;
}

base::Value BaseValueFromNSArray(NSArray<BaseValueBridge*>* array) {
  base::Value value(base::Value::Type::LIST);
  base::Value::List& list = *value.GetIfList();
  for (BaseValueBridge* obj in array) {
    list.Append(obj.value);
  }
  return value;
}

base::Value::List BaseValueListFromNSArray(NSArray<BaseValueBridge*>* array) {
  base::Value::List value;
  for (BaseValueBridge* obj in array) {
    value.Append(obj.value);
  }
  return value;
}

base::Value BaseValueFromNSDictionary(
    NSDictionary<NSString*, BaseValueBridge*>* dictionary) {
  base::Value result(base::Value::Type::DICT);
  base::Value::Dict& dict = result.GetDict();
  for (NSString* key in dictionary) {
    BaseValueBridge* value = dictionary[key];
    dict.Set(base::SysNSStringToUTF8(key), value.value);
  }
  return result;
}

NSDictionary<NSString*, BaseValueBridge*>* NSDictionaryFromBaseValueDict(
    base::Value::Dict value) {
  auto result = [[NSMutableDictionary alloc] init];
  for (auto kv : value) {
    result[base::SysUTF8ToNSString(kv.first)] =
        [[BaseValueBridge alloc] initWithValue:kv.second.Clone()];
  }
  return result;
}

base::Value::Dict BaseValueDictFromNSDictionary(
    NSDictionary<NSString*, BaseValueBridge*>* dictionary) {
  base::Value::Dict dict;
  for (NSString* key in dictionary) {
    BaseValueBridge* value = dictionary[key];
    dict.Set(base::SysNSStringToUTF8(key), value.value);
  }
  return dict;
}

}  // namespace brave
