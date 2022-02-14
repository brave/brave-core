/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BASE_MAC_CONVERSIONS_H_
#define BRAVE_BASE_MAC_CONVERSIONS_H_

#import <Foundation/Foundation.h>

#include <string>
#include <type_traits>
#include <vector>

/// This file is for converting from STL to Objective-C and vice-versa.
/// Notes: Once we have C++17 support, we can remove all the template
/// specializations and use `if constexpr` so that one function handles all
/// types.
namespace brave {
template <typename T, typename U>
struct is_objc_convertible {
 public:
  static const bool value =
      std::is_base_of<typename std::remove_pointer<T>::type,
                      typename std::remove_pointer<U>::type>::value ||
      std::is_convertible<typename std::remove_pointer<U>::type,
                          typename std::remove_pointer<T>::type*>::value;
};

/// Type-Trait to determine if a type is an Objective-C type
/// (Object/Interface/Protocol, etc)
template <typename T>
struct is_objc_type {
 public:
  static const bool value = is_objc_convertible<NSObject, T>::value;
};

/// Convert any `std::vector<Objective-C*>` to `NSArray<ObjectiveC*>`
template <typename T>
auto vector_to_ns(const std::vector<T>& vector) ->
    typename std::enable_if<is_objc_type<T>::value, NSArray<T>*>::type {
  NSMutableArray* array = [[NSMutableArray alloc] init];
  for (const T& value : vector) {
    [array addObject:value];
  }
  return array;
}

/// Converts any `std::vector<Fundamental-Type>` (primitives, etc..) to
/// `NSArray<Wrapped<FundamentalType>*>`
template <typename T>
auto vector_to_ns(const std::vector<T>& vector) ->
    typename std::enable_if<std::is_fundamental<T>::value,
                            NSArray<NSNumber*>*>::type {
  NSMutableArray* array = [[NSMutableArray alloc] init];
  for (const T& value : vector) {
    [array addObject:@(value)];
  }
  return array;
}

/// Converts any `std::vector<std::string>` to `NSArray<NSString*>`
template <typename T>
auto vector_to_ns(const std::vector<T>& vector) ->
    typename std::enable_if<std::is_same<T, std::string>::value,
                            NSArray<NSString*>*>::type {
  NSMutableArray* array = [[NSMutableArray alloc] init];
  for (const T& value : vector) {
    [array addObject:[NSString stringWithUTF8String:value.c_str()]];
  }
  return array;
}

/// Converts any `std::vector<string_literal>` to `NSArray<NSString*>`
template <typename T>
auto vector_to_ns(const std::vector<T>& vector) ->
    typename std::enable_if<std::is_same<T, char*>::value ||
                                std::is_same<T, const char*>::value ||
                                std::is_same<T, char[]>::value,
                            NSArray<NSString*>*>::type {
  NSMutableArray* array = [[NSMutableArray alloc] init];
  for (const T& value : vector) {
    [array addObject:[NSString stringWithUTF8String:value]];
  }
  return array;
}

/// Convert any `NSArray<NSNumber*>` to `std::vector<NSNumber*>`
/// Convert any `NSArray<NSString*>` to `std::vector<NSString*>`
/// Convert any `NSArray<NSObject*>` to `std::vector<NSObject*>`
template <typename T>
auto ns_to_vector(NSArray* array) ->
    typename std::enable_if<is_objc_convertible<NSNumber, T>::value ||
                                is_objc_convertible<NSString, T>::value ||
                                is_objc_convertible<NSObject, T>::value,
                            std::vector<T>>::type {
  std::vector<T> vector;
  for (T value : array) {
    vector.emplace_back(value);
  }
  return vector;
}

/// Converts any `NSArray<Wrapped<FundamentalType>*>` to
/// `std::vector<Fundamental-Type>` (primitives, etc..) When converting
/// `[1, 1.2, 3, 4, 5]` an ASSERTION will be thrown on `1.2` if `T` is of type
/// integer.
template <typename T>
auto ns_to_vector(NSArray* array) ->
    typename std::enable_if<std::is_fundamental<T>::value,
                            std::vector<T>>::type {
  std::vector<T> vector;
  for (NSNumber* value : array) {
    if (CFGetTypeID((__bridge CFTypeRef)value) != CFNumberGetTypeID()) {
      NSCAssert(false, @"NSNumber Conversion Failed - Not A NSNumber.");
    }

    // Convert to the largest supported types,
    // then truncate to the type `T`.
    // With C++17, we will use `if constexpr` here instead.
    CFNumberRef ref = (__bridge CFNumberRef)value;
    if (CFNumberIsFloatType(ref)) {
      Float64 result = 0.0;
      if (CFNumberGetValue(ref, CFNumberGetType(ref), &result) &&
          std::is_floating_point<T>::value) {
        vector.push_back(static_cast<T>(result));
      } else {
        NSCAssert(false,
                  @"NSNumber Conversion to Floating-Point Primitive Failed");
      }
    } else if (std::is_integral<T>::value && std::is_unsigned<T>::value) {
      UInt64 result = 0;
      if (CFNumberGetValue(ref, CFNumberGetType(ref), &result)) {
        vector.push_back(static_cast<T>(result));
      } else {
        NSCAssert(false,
                  @"NSNumber Conversion to Unsigned Integer Primitive Failed");
      }
    } else if (std::is_integral<T>::value && std::is_signed<T>::value) {
      SInt64 result = 0;
      if (CFNumberGetValue(ref, CFNumberGetType(ref), &result)) {
        vector.push_back(static_cast<T>(result));
      } else {
        NSCAssert(false,
                  @"NSNumber Conversion to Signed Integer Primitive Failed");
      }
    } else {
      NSCAssert(false, @"NSNumber Conversion to Unknown Primitive Type Failed");
    }
  }

  return vector;
}

/// Converts any `NSArray<NSString*>` to `std::vector<std::string>`
template <typename T>
auto ns_to_vector(NSArray* array) ->
    typename std::enable_if<std::is_same<T, std::string>::value,
                            std::vector<T>>::type {
  std::vector<T> vector;
  for (NSString* value : array) {
    vector.emplace_back([value UTF8String]);
  }
  return vector;
}

}  // namespace brave

#endif  // BRAVE_BASE_MAC_CONVERSIONS_H_
