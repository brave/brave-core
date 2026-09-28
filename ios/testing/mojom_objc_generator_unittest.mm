/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <optional>
#include <string>
#include <vector>

#include "base/strings/sys_string_conversions.h"
#include "base/test/run_until.h"
#import "brave/ios/testing/mojom_objc_generator_test.mojom.objc+private.h"
#include "ios/web/public/test/web_task_environment.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote_set.h"
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

using MojomObjcGeneratorEmptyResponseTest = PlatformTest;

TEST_F(MojomObjcGeneratorEmptyResponseTest, MethodWithParamTakesCompletion) {
  MojomObjcTestTestEmptyResponseInterface* test_interface =
      [[MojomObjcTestTestEmptyResponseInterface alloc] init];

  __block NSString* receivedValue = nil;
  test_interface._withParam = ^(NSString* value, void (^completion)(void)) {
    receivedValue = value;
    completion();
  };

  id<MojomObjcTestEmptyResponseInterface> proto = test_interface;

  __block BOOL completionCalled = NO;
  [proto withParam:@"value"
        completion:^{
          completionCalled = YES;
        }];
  EXPECT_NSEQ(receivedValue, @"value");
  EXPECT_TRUE(completionCalled);
}

TEST_F(MojomObjcGeneratorEmptyResponseTest,
       MethodWithoutParamsTakesCompletion) {
  MojomObjcTestTestEmptyResponseInterface* test_interface =
      [[MojomObjcTestTestEmptyResponseInterface alloc] init];

  test_interface._withoutParams = ^(void (^completion)(void)) {
    completion();
  };

  id<MojomObjcTestEmptyResponseInterface> proto = test_interface;

  __block BOOL completionCalled = NO;
  [proto withoutParams:^{
    completionCalled = YES;
  }];
  EXPECT_TRUE(completionCalled);
}

namespace {

class TestEventSource : public mojom_objc_test::mojom::EventSource {
 public:
  TestEventSource() = default;
  ~TestEventSource() override = default;

  mojo::PendingRemote<mojom_objc_test::mojom::EventSource> BindNewRemote() {
    return receiver_.BindNewPipeAndPassRemote();
  }

  void Notify(const std::string& value) {
    for (auto& observer : observers_) {
      observer->OnEvent(value);
    }
  }

  void ClearObservers() { observers_.Clear(); }

  size_t observer_count() const { return observers_.size(); }

  // mojom::EventSource:
  void AddObserver(mojo::PendingRemote<mojom_objc_test::mojom::EventObserver>
                       observer) override {
    observers_.Add(std::move(observer));
  }

 private:
  mojo::Receiver<mojom_objc_test::mojom::EventSource> receiver_{this};
  mojo::RemoteSet<mojom_objc_test::mojom::EventObserver> observers_;
};

}  // namespace

class MojomObjcGeneratorPendingRemoteTest : public PlatformTest {
 protected:
  void SetUp() override {
    PlatformTest::SetUp();
    event_source_ = [[MojomObjcTestEventSourceMojoImpl alloc]
        initWithEventSource:impl_.BindNewRemote()];
  }

  void TearDown() override {
    event_source_ = nil;
    PlatformTest::TearDown();
  }

  // Waits for `count` observers to be registered with the C++ event source.
  [[nodiscard]] bool WaitForObserverCount(size_t count) {
    return base::test::RunUntil(
        [&] { return impl_.observer_count() == count; });
  }

  web::WebTaskEnvironment task_environment_;
  TestEventSource impl_;
  MojomObjcTestEventSourceMojoImpl* event_source_ = nil;
  std::vector<std::string> received_;
  int call_count_ = 0;
};

TEST_F(MojomObjcGeneratorPendingRemoteTest, ForwardsMessagesToObserver) {
  MojomObjcTestTestEventObserver* observer =
      [[MojomObjcTestTestEventObserver alloc] init];
  observer._onEvent = ^(NSString* value) {
    received_.push_back(base::SysNSStringToUTF8(value));
  };

  [event_source_ addObserver:observer];
  ASSERT_TRUE(WaitForObserverCount(1u));

  impl_.Notify("hello");
  ASSERT_TRUE(base::test::RunUntil([&] { return received_.size() == 1u; }));
  EXPECT_EQ(received_, std::vector<std::string>{"hello"});
}

// The C++ side should see the remote disconnect once the Obj-C observer is
// deallocated.
TEST_F(MojomObjcGeneratorPendingRemoteTest,
       DeallocatingObserverDisconnectsRemote) {
  __weak MojomObjcTestTestEventObserver* weak_observer = nil;
  @autoreleasepool {
    MojomObjcTestTestEventObserver* observer =
        [[MojomObjcTestTestEventObserver alloc] init];
    observer._onEvent = ^(NSString* value) {
    };
    weak_observer = observer;
    [event_source_ addObserver:observer];
    ASSERT_TRUE(WaitForObserverCount(1u));
  }
  EXPECT_EQ(weak_observer, nil);

  ASSERT_TRUE(WaitForObserverCount(0u));

  // Notifying after the observer is gone should be a no-op.
  impl_.Notify("hello");
}

TEST_F(MojomObjcGeneratorPendingRemoteTest,
       SameObserverCanBeAddedMultipleTimes) {
  MojomObjcTestTestEventObserver* observer =
      [[MojomObjcTestTestEventObserver alloc] init];
  observer._onEvent = ^(NSString* value) {
    ++call_count_;
  };

  [event_source_ addObserver:observer];
  [event_source_ addObserver:observer];
  ASSERT_TRUE(WaitForObserverCount(2u));

  impl_.Notify("hello");
  ASSERT_TRUE(base::test::RunUntil([&] { return call_count_ == 2; }));
}

// The same Obj-C observer can be registered again after its remote disconnects.
TEST_F(MojomObjcGeneratorPendingRemoteTest,
       CanReregisterAfterRemoteDisconnect) {
  MojomObjcTestTestEventObserver* observer =
      [[MojomObjcTestTestEventObserver alloc] init];
  observer._onEvent = ^(NSString* value) {
    ++call_count_;
  };

  [event_source_ addObserver:observer];
  ASSERT_TRUE(WaitForObserverCount(1u));

  impl_.ClearObservers();
  ASSERT_TRUE(WaitForObserverCount(0u));

  // Registering again after a disconnect still works.
  [event_source_ addObserver:observer];
  ASSERT_TRUE(WaitForObserverCount(1u));
  impl_.Notify("hello");
  ASSERT_TRUE(base::test::RunUntil([&] { return call_count_ == 1; }));
}

// The observer's registration is independent of the lifetime of the Obj-C
// wrapper it was registered through.
TEST_F(MojomObjcGeneratorPendingRemoteTest, ObserverOutlivesMojoImpl) {
  MojomObjcTestTestEventObserver* observer =
      [[MojomObjcTestTestEventObserver alloc] init];
  observer._onEvent = ^(NSString* value) {
    ++call_count_;
  };

  [event_source_ addObserver:observer];
  ASSERT_TRUE(WaitForObserverCount(1u));
  event_source_ = nil;

  impl_.Notify("hello");
  ASSERT_TRUE(base::test::RunUntil([&] { return call_count_ == 1; }));
  EXPECT_EQ(impl_.observer_count(), 1u);
}
