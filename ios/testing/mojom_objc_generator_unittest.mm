/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#import "brave/ios/testing/mojom_objc_generator_test.mojom.objc+private.h"
#import "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"
#import "testing/platform_test.h"

using MojomObjcGeneratorOptionalEnumTest = PlatformTest;

// A nullable enum field with no default value should default to nil on
// the Obj-C side (rather than an illegal `nullable` non-pointer type).
TEST_F(MojomObjcGeneratorOptionalEnumTest, StructDefaultsOptionalFieldToNil) {
  MojomObjcTestNullableEnumStruct* obj =
      [[MojomObjcTestNullableEnumStruct alloc] init];
  EXPECT_EQ(obj.requiredEnumField, MojomObjcTestNullableEnumAlpha);
  EXPECT_EQ(obj.optionalEnumField, nil);
  EXPECT_NSEQ(obj.optionalEnumFieldWithDefault,
              @(MojomObjcTestNullableEnumBeta));
}

TEST_F(MojomObjcGeneratorOptionalEnumTest, StructObjCToCppRoundTripsValue) {
  MojomObjcTestNullableEnumStruct* obj =
      [[MojomObjcTestNullableEnumStruct alloc] init];
  obj.optionalEnumField = @(MojomObjcTestNullableEnumGamma);

  mojom_objc_test::mojom::NullableEnumStructPtr cpp_obj = obj.cppObjPtr;
  ASSERT_TRUE(cpp_obj->optional_enum_field.has_value());
  EXPECT_EQ(*cpp_obj->optional_enum_field,
            mojom_objc_test::mojom::NullableEnum::kGamma);
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
  cpp_obj.required_enum_field = mojom_objc_test::mojom::NullableEnum::kAlpha;
  cpp_obj.optional_enum_field = mojom_objc_test::mojom::NullableEnum::kGamma;

  MojomObjcTestNullableEnumStruct* obj =
      [[MojomObjcTestNullableEnumStruct alloc]
          initWithNullableEnumStruct:cpp_obj];
  EXPECT_NSEQ(obj.optionalEnumField, @(MojomObjcTestNullableEnumGamma));
}

TEST_F(MojomObjcGeneratorOptionalEnumTest, StructCppToObjCRoundTripsNil) {
  mojom_objc_test::mojom::NullableEnumStruct cpp_obj;
  cpp_obj.required_enum_field = mojom_objc_test::mojom::NullableEnum::kAlpha;
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
  MojomObjcTestTestNullableEnumInterface* testInterface =
      [[MojomObjcTestTestNullableEnumInterface alloc] init];

  __block NSNumber* receivedParam = nil;
  testInterface._doSomething = ^(NSNumber* _Nullable maybeEnum,
                                 void (^completion)(NSNumber* _Nullable)) {
    receivedParam = maybeEnum;
    completion(maybeEnum);
  };

  id<MojomObjcTestNullableEnumInterface> proto = testInterface;

  __block NSNumber* receivedResult = nil;
  __block BOOL completionCalled = NO;
  [proto doSomething:@(MojomObjcTestNullableEnumGamma)
          completion:^(NSNumber* _Nullable result) {
            completionCalled = YES;
            receivedResult = result;
          }];
  EXPECT_NSEQ(receivedParam, @(MojomObjcTestNullableEnumGamma));
  EXPECT_TRUE(completionCalled);
  EXPECT_NSEQ(receivedResult, @(MojomObjcTestNullableEnumGamma));

  completionCalled = NO;
  [proto doSomething:nil
          completion:^(NSNumber* _Nullable result) {
            completionCalled = YES;
            receivedResult = result;
          }];
  EXPECT_EQ(receivedParam, nil);
  EXPECT_TRUE(completionCalled);
  EXPECT_EQ(receivedResult, nil);
}
