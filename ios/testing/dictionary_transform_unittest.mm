/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#import "testing/gtest/include/gtest/gtest.h"
#import "testing/platform_test.h"

#import "base/containers/flat_map.h"
#import "brave/build/ios/mojom/cpp_transformations.h"
#import "test_foo.h"

using DictionaryTransformTest = PlatformTest;

TEST_F(DictionaryTransformTest, StringPrimitiveMapToDictionary) {
  std::map<std::string, int> m{{"0", 0}, {"1", 1}, {"2", 2}};
  const auto dict = NSDictionaryFromMap(m);
  EXPECT_EQ(dict.count, static_cast<NSUInteger>(3));
  const auto keys = @[ @"0", @"1", @"2" ];
  const auto values = @[ @(0), @(1), @(2) ];
  EXPECT_TRUE([dict.allKeys isEqualToArray:keys]);
  EXPECT_TRUE([dict.allValues isEqualToArray:values]);
  EXPECT_EQ(dict[@"0"].integerValue, 0);
  EXPECT_EQ(dict[@"1"].integerValue, 1);
  EXPECT_EQ(dict[@"2"].integerValue, 2);
}

TEST_F(DictionaryTransformTest, StringStringMapToDictionary) {
  std::map<std::string, std::string> m{
      {"0", "test0"}, {"1", "test1"}, {"2", "test2"}};
  const auto dict = NSDictionaryFromMap(m);
  EXPECT_EQ(dict.count, static_cast<NSUInteger>(3));
  const auto keys = @[ @"0", @"1", @"2" ];
  const auto values = @[ @"test0", @"test1", @"test2" ];
  EXPECT_TRUE([dict.allKeys isEqualToArray:keys]);
  EXPECT_TRUE([dict.allValues isEqualToArray:values]);
  EXPECT_TRUE([dict[@"0"] isEqualToString:@"test0"]);
  EXPECT_TRUE([dict[@"1"] isEqualToString:@"test1"]);
  EXPECT_TRUE([dict[@"2"] isEqualToString:@"test2"]);
}

TEST_F(DictionaryTransformTest, StringCppStructToDictionary) {
  std::map<std::string, CppFoo> m{
      {"0", CppFoo(true, 10, "test", {1.0, 2.0, 3.0})},
      {"1", CppFoo(false, 7, "test2", {3.0, 2.0, 1.0})},
  };
  const auto dict = NSDictionaryFromMap(m, ^TestFoo*(CppFoo foo) {
    return [[TestFoo alloc] initWithCppFoo:foo];
  });

  EXPECT_EQ(dict.count, static_cast<NSUInteger>(2));
  const auto keys = @[ @"0", @"1" ];
  EXPECT_TRUE([dict.allKeys isEqualToArray:keys]);

  const auto foo = dict[@"0"];
  EXPECT_NE(foo, nil);
  EXPECT_EQ(foo.boolean, true);
  EXPECT_EQ(foo.integer, 10);
  EXPECT_TRUE([foo.stringObject isEqualToString:@"test"]);
  const auto numbers = @[ @(1.0), @(2.0), @(3.0) ];
  EXPECT_TRUE([foo.numbers isEqualToArray:numbers]);

  const auto foo1 = dict[@"1"];
  EXPECT_NE(foo1, nil);
  EXPECT_EQ(foo1.boolean, false);
  EXPECT_EQ(foo1.integer, 7);
  EXPECT_TRUE([foo1.stringObject isEqualToString:@"test2"]);
  const auto numbers2 = @[ @(3.0), @(2.0), @(1.0) ];
  EXPECT_TRUE([foo1.numbers isEqualToArray:numbers2]);
}

TEST_F(DictionaryTransformTest, StringNSDictionaryToStringMap) {
  const auto d = @{@"1" : @"2", @"3" : @"4"};
  base::flat_map<std::string, std::string> map = MapFromNSDictionary(d);
  EXPECT_EQ(map["1"], "2");
  EXPECT_EQ(map["3"], "4");
  EXPECT_EQ(map.find("5"), map.end());
}
