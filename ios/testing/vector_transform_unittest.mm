/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#import <string>
#import <vector>

#import "base/containers/flat_map.h"
#import "brave/build/ios/mojom/cpp_transformations.h"
#import "test_foo.h"
#import "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"
#import "testing/platform_test.h"

using VectorTransformTest = PlatformTest;

TEST_F(VectorTransformTest, PrimitiveVectorToNSNumberArray) {
  std::vector<int> v{1, 2, 3};
  auto array = NSArrayFromVector(v);
  ASSERT_EQ(array.count, 3u);
  EXPECT_EQ(array[0].intValue, 1);
  EXPECT_EQ(array[1].intValue, 2);
  EXPECT_EQ(array[2].intValue, 3);
}

TEST_F(VectorTransformTest, NSNumberArrayToPrimitiveVector) {
  NSArray<NSNumber*>* a = @[ @(1), @(2), @(3) ];
  std::vector<int> v = VectorFromNSArray<int>(a);
  EXPECT_EQ(v, std::vector<int>({1, 2, 3}));
}

TEST_F(VectorTransformTest, StringVectorToStringArray) {
  std::vector<std::string> v{"1", "2", "3"};
  auto array = NSArrayFromVector(v);
  EXPECT_NSEQ(array, (@[ @"1", @"2", @"3" ]));
}

TEST_F(VectorTransformTest, StringArrayToStringVector) {
  NSArray<NSString*>* a = @[ @"1", @"2", @"3" ];
  std::vector<std::string> v = VectorFromNSArray(a);
  EXPECT_EQ(v, std::vector<std::string>({"1", "2", "3"}));
}

TEST_F(VectorTransformTest, VectorObjectsToArrayObjects) {
  std::vector<CppFoo> foos{
      CppFoo(true, 10, "test", {1.0, 2.0, 3.0}),
      CppFoo(false, 7, "tset", {3.0, 2.0, 1.0}),
  };
  const auto array = NSArrayFromVector(foos, ^TestFoo*(const CppFoo& foo) {
    return [[TestFoo alloc] initWithCppFoo:foo];
  });
  ASSERT_EQ(array.count, 2u);

  EXPECT_TRUE(array[0].boolean);
  EXPECT_EQ(array[0].integer, 10);
  EXPECT_NSEQ(array[0].stringObject, @"test");
  EXPECT_NSEQ(array[0].numbers, (@[ @1.0, @2.0, @3.0 ]));

  EXPECT_FALSE(array[1].boolean);
  EXPECT_EQ(array[1].integer, 7);
  EXPECT_NSEQ(array[1].stringObject, @"tset");
  EXPECT_NSEQ(array[1].numbers, (@[ @3.0, @2.0, @1.0 ]));
}
