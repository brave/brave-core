/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_TESTING_TEST_FOO_H_
#define BRAVE_IOS_TESTING_TEST_FOO_H_

#import <Foundation/Foundation.h>
#include <string>
#include <vector>

NS_ASSUME_NONNULL_BEGIN

struct CppFoo {
  CppFoo(const CppFoo&);
  CppFoo(bool b, int i, std::string s, std::vector<double> ds);
  ~CppFoo();

  bool boolean;
  int integer;
  std::string stringObject;
  std::vector<double> numbers;
};

@interface TestFoo : NSObject
@property(nonatomic, assign) BOOL boolean;
@property(nonatomic, assign) int integer;
@property(nonatomic, copy) NSString* stringObject;
@property(nonatomic, copy) NSArray<NSNumber*>* numbers;
- (instancetype)initWithCppFoo:(const CppFoo&)foo;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_TESTING_TEST_FOO_H_
