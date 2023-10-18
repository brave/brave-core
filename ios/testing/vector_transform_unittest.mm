/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#import "testing/gtest/include/gtest/gtest.h"
#import "testing/platform_test.h"

#import "base/containers/flat_map.h"
#import "brave/build/ios/mojom/cpp_transformations.h"
#import "test_foo.h"

using VectorTransformTest = PlatformTest;

TEST_F(VectorTransformTest, PrimitiveVectorToNSNumberArray) {
  std::vector<int> v{1, 2, 3};
  auto array = NSArrayFromVector(v);
  EXPECT_TRUE(array.count == 3);
  EXPECT_TRUE(array[0].intValue == 1 && array[1].intValue == 2 &&
              array[2].intValue == 3);
}

TEST_F(VectorTransformTest, NSNumberArrayToPrimitiveVector) {
  NSArray<NSNumber*>* a = @[ @(1), @(2), @(3) ];
  std::vector<int> v = VectorFromNSArray<int>(a);
  EXPECT_TRUE(v.size() == 3);
  EXPECT_TRUE(v[0] == 1 && v[1] == 2 && v[2] == 3);
}

TEST_F(VectorTransformTest, StringVectorToStringArray) {
  std::vector<std::string> v{"1", "2", "3"};
  auto array = NSArrayFromVector(v);
  EXPECT_TRUE(array.count == 3);
  EXPECT_TRUE([array[0] isEqualToString:@"1"] &&
              [array[1] isEqualToString:@"2"] &&
              [array[2] isEqualToString:@"3"]);
}

TEST_F(VectorTransformTest, StringArrayToStringVector) {
  NSArray<NSString*>* a = @[ @"1", @"2", @"3" ];
  std::vector<std::string> v = VectorFromNSArray(a);
  EXPECT_TRUE(v.size() == 3);
  EXPECT_TRUE(v[0] == "1" && v[1] == "2" && v[2] == "3");
}

TEST_F(VectorTransformTest, VectorObjectsToArrayObjects) {
  std::vector<CppFoo> foos{
      CppFoo(true, 10, "test", {1.0, 2.0, 3.0}),
      CppFoo(false, 7, "tset", {3.0, 2.0, 1.0}),
  };
  const auto array = NSArrayFromVector(foos, ^TestFoo*(const CppFoo& foo) {
    return [[TestFoo alloc] initWithCppFoo:foo];
  });
  EXPECT_TRUE(array.count == 2);
  EXPECT_TRUE(array[0].boolean == true && array[0].integer == 10 &&
              [array[0].stringObject isEqualToString:@"test"] &&
              array[0].numbers.count == 3);
  EXPECT_TRUE(array[1].boolean == false && array[1].integer == 7 &&
              [array[1].stringObject isEqualToString:@"tset"] &&
              array[1].numbers.count == 3);
}
