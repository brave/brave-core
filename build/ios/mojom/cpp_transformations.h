/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BUILD_IOS_MOJOM_CPP_TRANSFORMATIONS_H_
#define BRAVE_BUILD_IOS_MOJOM_CPP_TRANSFORMATIONS_H_

#import <Foundation/Foundation.h>
#import <objc/runtime.h>
#include <map>
#include <string>
#include <utility>
#include <vector>
#include "base/containers/flat_map.h"

static std::map<const char*, SEL> numberInitMap = {
    {@encode(bool), @selector(numberWithBool:)},
    {@encode(char), @selector(numberWithChar:)},
    {@encode(double), @selector(numberWithDouble:)},
    {@encode(float), @selector(numberWithFloat:)},
    {@encode(int), @selector(numberWithInt:)},
    {@encode(NSInteger), @selector(numberWithInteger:)},
    {@encode(long), @selector(numberWithLong:)},
    {@encode(long long), @selector(numberWithLongLong:)},
    {@encode(short), @selector(numberWithShort:)},
    {@encode(unsigned char), @selector(numberWithUnsignedChar:)},
    {@encode(unsigned int), @selector(numberWithUnsignedInt:)},
    {@encode(NSUInteger), @selector(numberWithUnsignedInteger:)},
    {@encode(unsigned long), @selector(numberWithUnsignedLong:)},
    {@encode(unsigned long long), @selector(numberWithUnsignedLongLong:)},
    {@encode(unsigned short), @selector(numberWithUnsignedShort:)},
};

static std::map<const char*, SEL> numberGetterMap = {
    {@encode(bool), @selector(boolValue)},
    {@encode(char), @selector(charValue)},
    {@encode(double), @selector(doubleValue)},
    {@encode(float), @selector(floatValue)},
    {@encode(int), @selector(intValue)},
    {@encode(NSInteger), @selector(integerValue)},
    {@encode(long), @selector(longValue)},
    {@encode(long long), @selector(longLongValue)},
    {@encode(short), @selector(shortValue)},
    {@encode(unsigned char), @selector(unsignedCharValue)},
    {@encode(unsigned int), @selector(unsignedIntValue)},
    {@encode(NSUInteger), @selector(unsignedIntegerValue)},
    {@encode(unsigned long), @selector(unsignedLongValue)},
    {@encode(unsigned long long), @selector(unsignedLongLongValue)},
    {@encode(unsigned short), @selector(unsignedShortValue)},
};

#pragma mark - Vectors

/// Convert a vector storing primatives to an array of NSNumber's
template <typename T>
NS_INLINE NSArray<NSNumber*>* NSArrayFromVector(std::vector<T> v) {
  const auto a = [NSMutableArray new];
  if (v.empty()) {
    return @[];
  }
  // Since vector's are uniformly typed, we can just use v[0]
  const auto encode = @encode(__typeof__(v[0]));
  const auto selector = numberInitMap[encode];
  if (selector == nullptr) {
    return @[];
  }
  const auto method = class_getClassMethod(NSNumber.class, selector);
  typedef NSNumber* (*NSNumberCall)(id, SEL, T);
  NSNumberCall call = (NSNumberCall)method_getImplementation(method);

  for (const auto& t : v) {
    NSNumber* number =
        reinterpret_cast<NSNumber*>(call(NSNumber.class, selector, t));
    [a addObject:number];
  }
  return a;
}

/// Convert an NSArray storing NSNumber's to a std::vector storing primatives
template <typename T>
NS_INLINE std::vector<T> VectorFromNSArray(NSArray<NSNumber*>* a) {
  std::vector<T> v;
  if (a.count == 0) {
    return v;
  }
  const auto encode = @encode(__typeof__(T));
  const auto selector = numberGetterMap[encode];
  if (selector == nullptr) {
    return v;
  }
  const auto method = class_getInstanceMethod(NSNumber.class, selector);
  typedef T (*NSNumberCall)(id, SEL);
  NSNumberCall call = (NSNumberCall)method_getImplementation(method);

  for (NSNumber* number in a) {
    v.push_back(call(number, selector));
  }
  return v;
}

/// Convert a vector storing strings to an array of NSString's
NS_INLINE NSArray<NSString*>* NSArrayFromVector(std::vector<std::string> v) {
  const auto a = [NSMutableArray new];
  for (const auto& s : v) {
    [a addObject:[NSString stringWithCString:s.c_str()
                                    encoding:NSUTF8StringEncoding]];
  }
  return a;
}

/// Convert an NSArray storing strings to an vector of std::string's
NS_INLINE std::vector<std::string> VectorFromNSArray(NSArray<NSString*>* a) {
  std::vector<std::string> v;
  for (NSString* str in a) {
    v.push_back(std::string(str.UTF8String));
  }
  return v;
}

/// Convert a vector storing objects to an array of transformed objects's
template <typename T, typename U>
NS_INLINE NSArray<T>* NSArrayFromVector(std::vector<U> v,
                                        T (^transformValue)(const U&)) {
  const auto a = [NSMutableArray new];
  for (const auto& o : v) {
    [a addObject:transformValue(o)];
  }
  return a;
}

/// Convert a vector storing objects to an array of transformed objects's
template <typename T, typename U>
NS_INLINE NSArray<T>* NSArrayFromVector(const std::vector<U>* v,
                                        T (^transformValue)(const U&)) {
  const auto a = [NSMutableArray new];
  if (v == nullptr) {
    return a;
  }
  for (const auto& o : *v) {
    [a addObject:transformValue(o)];
  }
  return a;
}

/// Convert a NSArray storing objects to an std::vector of transformed objects's
template <typename T, typename U>
NS_INLINE std::vector<U> VectorFromNSArray(NSArray<T>* a,
                                           U (^transformValue)(T)) {
  std::vector<U> v;
  for (id t in a) {
    v.push_back(transformValue(t));
  }
  return v;
}

#pragma mark - Maps

/// Get an NSNumber object from a primitive type (int, bool, etc.)
template <typename T>
NS_INLINE NSNumber* NumberFromPrimitive(T t) {
  const auto encode = @encode(__typeof__(t));
  const auto selector = numberInitMap[encode];
  if (selector == nullptr) {
    return nil;
  }
  const auto method = class_getClassMethod(NSNumber.class, selector);
  typedef NSNumber* (*NSNumberCall)(id, SEL, T);
  NSNumberCall call =
      reinterpret_cast<NSNumberCall>(method_getImplementation(method));
  return call(NSNumber.class, selector, t);
}

/// Convert a String's to primitives mapping to an NSDictionary<NSString*,
/// NSNumber *>
template <typename T>
NS_INLINE NSDictionary<NSString*, NSNumber*>* NSDictionaryFromMap(
    const std::map<std::string, T>& m) {
  const auto d = [NSMutableDictionary new];
  if (m.empty()) {
    return @{};
  }
  for (const auto& item : m) {
    d[[NSString stringWithCString:item.first.c_str()
                         encoding:NSUTF8StringEncoding]] =
        NumberFromPrimitive(item.second);
  }
  return d;
}

/// Convert a String's to primitives mapping to an NSDictionary<NSString*,
/// NSNumber *>
template <typename T>
NS_INLINE NSDictionary<NSString*, NSNumber*>* NSDictionaryFromMap(
    const base::flat_map<std::string, T>& m) {
  const auto d = [NSMutableDictionary new];
  if (m.empty()) {
    return @{};
  }
  for (const auto& item : m) {
    d[[NSString stringWithCString:item.first.c_str()
                         encoding:NSUTF8StringEncoding]] =
        NumberFromPrimitive(item.second);
  }
  return d;
}

/// Convert a String to String mapping to an NSDictionary
NS_INLINE NSDictionary<NSString*, NSString*>* NSDictionaryFromMap(
    const std::map<std::string, std::string>& m) {
  const auto d = [NSMutableDictionary new];
  if (m.empty()) {
    return @{};
  }
  for (const auto& item : m) {
    d[[NSString stringWithCString:item.first.c_str()
                         encoding:NSUTF8StringEncoding]] =
        [NSString stringWithCString:item.second.c_str()
                           encoding:NSUTF8StringEncoding];
  }
  return d;
}

/// Convert a String to String mapping to an NSDictionary
NS_INLINE NSDictionary<NSString*, NSString*>* NSDictionaryFromMap(
    const base::flat_map<std::string, std::string>& m) {
  const auto d = [NSMutableDictionary new];
  if (m.empty()) {
    return @{};
  }
  for (const auto& item : m) {
    d[[NSString stringWithCString:item.first.c_str()
                         encoding:NSUTF8StringEncoding]] =
        [NSString stringWithCString:item.second.c_str()
                           encoding:NSUTF8StringEncoding];
  }
  return d;
}

/// Convert a String to C++ object mapping to an NSDictionary of String to Obj-C
/// objects
template <typename V, typename ObjCObj>
NS_INLINE NSDictionary<NSString*, ObjCObj>* NSDictionaryFromMap(
    const std::map<std::string, V>& m,
    ObjCObj (^transformValue)(V)) {
  const auto d = [NSMutableDictionary new];
  if (m.empty()) {
    return @{};
  }
  for (const auto& item : m) {
    d[[NSString stringWithCString:item.first.c_str()
                         encoding:NSUTF8StringEncoding]] =
        transformValue(item.second);
  }
  return d;
}

/// Convert a String to C++ object mapping to an NSDictionary of String to Obj-C
/// objects
template <typename V, typename ObjCObj>
NS_INLINE NSDictionary<NSString*, ObjCObj>* NSDictionaryFromMap(
    const base::flat_map<std::string, V>& m,
    ObjCObj (^transformValue)(V)) {
  const auto d = [NSMutableDictionary new];
  if (m.empty()) {
    return @{};
  }
  for (const auto& item : m) {
    d[[NSString stringWithCString:item.first.c_str()
                         encoding:NSUTF8StringEncoding]] =
        transformValue(item.second);
  }
  return d;
}

/// Convert any mapping to an NSDictionary of Obj-C objects by transforming both
/// the key and the value types to Obj-C types
template <typename K, typename KObjC, typename V, typename VObjC>
NS_INLINE NSDictionary<KObjC, VObjC>* NSDictionaryFromMap(
    const std::map<K, V>& m,
    KObjC (^transformKey)(K),
    VObjC (^transformValue)(V)) {
  const auto d = [NSMutableDictionary new];
  if (m.empty()) {
    return @{};
  }
  for (const auto& item : m) {
    d[transformKey(item.first)] = transformValue(item.second);
  }
  return d;
}

/// Convert any mapping to an NSDictionary of Obj-C objects by transforming both
/// the key and the value types to Obj-C types
template <typename K, typename KObjC, typename V, typename VObjC>
NS_INLINE NSDictionary<KObjC, VObjC>* NSDictionaryFromMap(
    const base::flat_map<K, V>& m,
    KObjC (^transformKey)(K),
    VObjC (^transformValue)(V)) {
  const auto d = [NSMutableDictionary new];
  if (m.empty()) {
    return @{};
  }
  for (const auto& item : m) {
    d[transformKey(item.first)] = transformValue(item.second);
  }
  return d;
}

/// Converts an NSDictionary that has NSString keys & values to a base::flat_map
/// with std::string keys & values
NS_INLINE base::flat_map<std::string, std::string> MapFromNSDictionary(
    NSDictionary<NSString*, NSString*>* d) {
  base::flat_map<std::string, std::string> map;
  for (NSString* key in d) {
    map.insert(std::make_pair(key.UTF8String, d[key].UTF8String));
  }
  return map;
}

#endif  // BRAVE_BUILD_IOS_MOJOM_CPP_TRANSFORMATIONS_H_
