/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BASE_APPLE_BASE_VALUE_BRIDGE_H_
#define BRAVE_BASE_APPLE_BASE_VALUE_BRIDGE_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, BaseValueBridgeTag) {
  BaseValueBridgeTagNull = 0,
  BaseValueBridgeTagBoolValue,
  BaseValueBridgeTagIntValue,
  BaseValueBridgeTagDoubleValue,
  BaseValueBridgeTagStringValue,
  BaseValueBridgeTagBinaryValue,
  BaseValueBridgeTagDictionaryValue,
  BaseValueBridgeTagListValue,
} NS_SWIFT_NAME(BaseValue.Tag);

OBJC_EXPORT
NS_SWIFT_NAME(BaseValue)
@interface BaseValueBridge : NSObject <NSCopying>
@property(readonly) BaseValueBridgeTag tag;
- (instancetype)init;
- (instancetype)initWithBoolValue:(bool)boolValue;
- (instancetype)initWithIntValue:(int32_t)intValue;
- (instancetype)initWithDoubleValue:(double)doubleValue;
- (instancetype)initWithStringValue:(NSString*)stringValue;
- (instancetype)initWithBinaryValue:(NSArray<NSNumber*>*)binaryValue;
- (instancetype)initWithDictionaryValue:
    (NSDictionary<NSString*, BaseValueBridge*>*)dictionaryValue;
- (instancetype)initWithListValue:(NSArray<BaseValueBridge*>*)listValue;
@property(nonatomic) bool boolValue;
@property(nonatomic) int32_t intValue;
@property(nonatomic) double doubleValue;
@property(nonatomic, copy, nullable) NSString* stringValue;
@property(nonatomic, copy, nullable) NSArray<NSNumber*>* binaryValue;
@property(nonatomic, copy, nullable)
    NSDictionary<NSString*, BaseValueBridge*>* dictionaryValue;
@property(nonatomic, copy, nullable) NSArray<BaseValueBridge*>* listValue;
@end

@interface BaseValueBridge (JSON)
- (nullable instancetype)initWithJSONString:(NSString*)json;
@property(nonatomic, readonly, nullable) NSString* jsonString;
@property(nonatomic, readonly, nullable) id jsonObject;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_BASE_APPLE_BASE_VALUE_BRIDGE_H_
