/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <optional>

#import "brave/ios/testing/mojom_objc_generator_test.mojom.objc+private.h"
#import "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"
#import "testing/platform_test.h"

using MojomObjcGeneratorOptionalEnumTest = PlatformTest;

// A nullable enum field with no default value should default to nil.
TEST_F(MojomObjcGeneratorOptionalEnumTest, StructDefaultsOptionalFieldToNil) {
  MojomObjcTestNullableEnumStruct* obj =
      [[MojomObjcTestNullableEnumStruct alloc] init];
  EXPECT_EQ(obj.requiredEnumField, MojomObjcTestSomeEnumAlpha);
  EXPECT_EQ(obj.optionalEnumField, nil);
  EXPECT_EQ(obj.optionalEnumFieldWithDefault.value, MojomObjcTestSomeEnumBeta);
}

TEST_F(MojomObjcGeneratorOptionalEnumTest, StructObjCToCppRoundTripsValue) {
  MojomObjcTestNullableEnumStruct* obj =
      [[MojomObjcTestNullableEnumStruct alloc] init];
  obj.optionalEnumField = [[MojomObjcTestSomeEnumBox alloc]
      initWithValue:MojomObjcTestSomeEnumGamma];

  mojom_objc_test::mojom::NullableEnumStructPtr cpp_obj = obj.cppObjPtr;
  ASSERT_TRUE(cpp_obj->optional_enum_field.has_value());
  EXPECT_EQ(*cpp_obj->optional_enum_field,
            mojom_objc_test::mojom::SomeEnum::kGamma);
}

TEST_F(MojomObjcGeneratorOptionalEnumTest, StructObjCToCppRoundTripsNil) {
  MojomObjcTestNullableEnumStruct* obj =
      [[MojomObjcTestNullableEnumStruct alloc] init];
  obj.optionalEnumField = nil;

  mojom_objc_test::mojom::NullableEnumStructPtr cpp_obj = obj.cppObjPtr;
  EXPECT_FALSE(cpp_obj->optional_enum_field.has_value());
}

TEST_F(MojomObjcGeneratorOptionalEnumTest, StructCppToObjCRoundTripsValue) {
  mojom_objc_test::mojom::NullableEnumStruct cpp_obj;
  cpp_obj.required_enum_field = mojom_objc_test::mojom::SomeEnum::kAlpha;
  cpp_obj.optional_enum_field = mojom_objc_test::mojom::SomeEnum::kGamma;

  MojomObjcTestNullableEnumStruct* obj =
      [[MojomObjcTestNullableEnumStruct alloc]
          initWithNullableEnumStruct:cpp_obj];
  EXPECT_EQ(obj.optionalEnumField.value, MojomObjcTestSomeEnumGamma);
}

TEST_F(MojomObjcGeneratorOptionalEnumTest, StructCppToObjCRoundTripsNil) {
  mojom_objc_test::mojom::NullableEnumStruct cpp_obj;
  cpp_obj.required_enum_field = mojom_objc_test::mojom::SomeEnum::kAlpha;
  cpp_obj.optional_enum_field = std::nullopt;

  MojomObjcTestNullableEnumStruct* obj =
      [[MojomObjcTestNullableEnumStruct alloc]
          initWithNullableEnumStruct:cpp_obj];
  EXPECT_EQ(obj.optionalEnumField, nil);
}

// The generated `Test<Interface>` double conforms to the protocol directly
// (no Mojo pipe involved), so it can drive the nullable-enum parameter and
// response/callback parameter paths synchronously.
TEST_F(MojomObjcGeneratorOptionalEnumTest,
       InterfaceParamAndResponseAreNullable) {
  MojomObjcTestTestNullableEnumInterface* test_interface =
      [[MojomObjcTestTestNullableEnumInterface alloc] init];

  __block MojomObjcTestSomeEnumBox* receivedParam = nil;
  test_interface._doSomething =
      ^(MojomObjcTestSomeEnumBox* _Nullable maybeEnum,
        void (^completion)(MojomObjcTestSomeEnumBox* _Nullable)) {
        receivedParam = maybeEnum;
        completion(maybeEnum);
      };

  id<MojomObjcTestNullableEnumInterface> proto = test_interface;

  __block MojomObjcTestSomeEnumBox* receivedResult = nil;
  __block BOOL completionCalled = NO;
  [proto doSomething:[[MojomObjcTestSomeEnumBox alloc]
                         initWithValue:MojomObjcTestSomeEnumGamma]
          completion:^(MojomObjcTestSomeEnumBox* _Nullable result) {
            completionCalled = YES;
            receivedResult = result;
          }];
  EXPECT_EQ(receivedParam.value, MojomObjcTestSomeEnumGamma);
  EXPECT_TRUE(completionCalled);
  EXPECT_EQ(receivedResult.value, MojomObjcTestSomeEnumGamma);

  completionCalled = NO;
  [proto doSomething:nil
          completion:^(MojomObjcTestSomeEnumBox* _Nullable result) {
            completionCalled = YES;
            receivedResult = result;
          }];
  EXPECT_EQ(receivedParam, nil);
  EXPECT_TRUE(completionCalled);
  EXPECT_EQ(receivedResult, nil);
}
