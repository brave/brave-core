/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BUILD_IOS_MOJOM_PUBLIC_BASE_BASE_VALUES_H_
#define BRAVE_BUILD_IOS_MOJOM_PUBLIC_BASE_BASE_VALUES_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
NS_SWIFT_NAME(MojoBase)
@interface _MojoBaseValuesMojom : NSObject
- (instancetype)init NS_UNAVAILABLE;
@end

typedef NS_ENUM(NSInteger, MojoBaseValueTag) {
  MojoBaseValueTagNull = 0,
  MojoBaseValueTagBoolValue,
  MojoBaseValueTagIntValue,
  MojoBaseValueTagDoubleValue,
  MojoBaseValueTagStringValue,
  MojoBaseValueTagBinaryValue,
  MojoBaseValueTagDictionaryValue,
  MojoBaseValueTagListValue,
} NS_SWIFT_NAME(MojoBase.ValueTag);

OBJC_EXPORT
NS_SWIFT_NAME(MojoBase.Value)
@interface MojoBaseValue : NSObject <NSCopying>
@property(readonly) MojoBaseValueTag tag;
- (instancetype)init;
- (instancetype)initWithBoolValue:(bool)boolValue;
- (instancetype)initWithIntValue:(int32_t)intValue;
- (instancetype)initWithDoubleValue:(double)doubleValue;
- (instancetype)initWithStringValue:(NSString*)stringValue;
- (instancetype)initWithBinaryValue:(NSArray<NSNumber*>*)binaryValue;
- (instancetype)initWithDictionaryValue:
    (NSDictionary<NSString*, MojoBaseValue*>*)dictionaryValue;
- (instancetype)initWithListValue:(NSArray<MojoBaseValue*>*)listValue;
@property(nonatomic) bool boolValue;
@property(nonatomic) int32_t intValue;
@property(nonatomic) double doubleValue;
@property(nonatomic, copy, nullable) NSString* stringValue;
@property(nonatomic, copy, nullable) NSArray<NSNumber*>* binaryValue;
@property(nonatomic, copy, nullable)
    NSDictionary<NSString*, MojoBaseValue*>* dictionaryValue;
@property(nonatomic, copy, nullable) NSArray<MojoBaseValue*>* listValue;
@end

@interface MojoBaseValue (JSON)
- (nullable instancetype)initWithJSONString:(NSString*)json;
@property(nonatomic, readonly, nullable) NSString* jsonString;
@property(nonatomic, readonly, nullable) id jsonObject;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_BUILD_IOS_MOJOM_PUBLIC_BASE_BASE_VALUES_H_
