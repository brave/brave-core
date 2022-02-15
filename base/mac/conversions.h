/* Copyright (c) 2022 The Brave Authors. All rights reserved.
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
namespace brave {
template <typename T, typename U>
struct is_objc_convertible {
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
  static const bool value = is_objc_convertible<NSObject, T>::value;
};

/// Converts from an std::vector<T> to the equivalent NSArray
template <typename T>
NSArray* vector_to_ns(const std::vector<T>& vector) {
  NSMutableArray* array = [[NSMutableArray alloc] init];
  for (const T& value : vector) {
    if constexpr (is_objc_type<T>::value) {
      /// Convert any `std::vector<Objective-C*>` to `NSArray<ObjectiveC*>`
      [array addObject:value];
    } else if constexpr (std::is_fundamental<T>::value) {
      /// Converts any `std::vector<Fundamental-Type>` (primitives, etc..) to
      /// `NSArray<Wrapped<FundamentalType>*>`
      [array addObject:@(value)];
    } else if constexpr (std::is_same<T, std::string>::value) {
      /// Converts any `std::vector<std::string>` to `NSArray<NSString*>`
      [array addObject:[NSString stringWithUTF8String:value.c_str()]];
    } else {
      return nullptr;
    }
  }

  return array;
}

/// Converts from an NSArray to the equivalent std::vector<T>
/// Throws an Objective-C exception if conversion cannot happen
template <typename T>
std::vector<T> ns_to_vector(NSArray* array) {
  std::vector<T> vector;

  /// Convert any `NSArray<NSNumber*>` to `std::vector<NSNumber*>`
  /// Convert any `NSArray<NSString*>` to `std::vector<NSString*>`
  /// Convert any `NSArray<NSObject*>` to `std::vector<NSObject*>`
  if constexpr (is_objc_type<T>::value) {
    for (T value : array) {
      vector.emplace_back(value);
    }
  } else if constexpr (std::is_fundamental<T>::value) {  // NOLINT
    /// Converts any `NSArray<Wrapped<FundamentalType>*>` to
    /// `std::vector<Fundamental-Type>` (primitives, etc..) When converting
    /// `[1, 1.2, 3, 4, 5]` an ASSERTION will be thrown on `1.2` if `T` is of
    /// type integer.
    for (NSNumber* value : array) {
      if (CFGetTypeID((__bridge CFTypeRef)value) != CFNumberGetTypeID()) {
        NSCAssert(false, @"NSNumber Conversion Failed - Not A NSNumber.");
      }

      // Convert to the largest supported types,
      // then truncate to the type `T`.
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
      } else if constexpr (std::is_integral<T>::value &&  // NOLINT
                           std::is_unsigned<T>::value) {
        UInt64 result = 0;
        if (CFNumberGetValue(ref, CFNumberGetType(ref), &result)) {
          vector.push_back(static_cast<T>(result));
        } else {
          NSCAssert(
              false,
              @"NSNumber Conversion to Unsigned Integer Primitive Failed");
        }
      } else if constexpr (std::is_integral<T>::value &&  // NOLINT
                           std::is_signed<T>::value) {
        SInt64 result = 0;
        if (CFNumberGetValue(ref, CFNumberGetType(ref), &result)) {
          vector.push_back(static_cast<T>(result));
        } else {
          NSCAssert(false,
                    @"NSNumber Conversion to Signed Integer Primitive Failed");
        }
      } else if constexpr (std::is_floating_point<T>::value) {
        NSCAssert(false,
                  @"NSNumber Conversion to Floating-Point Primitive Failed");
      } else {
        NSCAssert(false,
                  @"NSNumber Conversion to Unknown Primitive Type Failed");
      }
    }
  } else if constexpr (std::is_same<T, std::string>::value) {  // NOLINT
    /// Converts any `NSArray<NSString*>` to `std::vector<std::string>`
    for (NSString* value : array) {
      vector.emplace_back([value UTF8String]);
    }
  }
  return vector;
}

}  // namespace brave

#endif  // BRAVE_BASE_MAC_CONVERSIONS_H_
