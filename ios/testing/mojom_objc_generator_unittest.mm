/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <optional>
#include <string>
#include <vector>

#include "base/functional/bind.h"
#include "base/strings/sys_string_conversions.h"
#include "base/task/thread_pool.h"
#include "base/test/bind.h"
#include "base/test/run_until.h"
#include "base/types/expected.h"
#import "brave/ios/testing/mojom_objc_generator_test.mojom.objc+private.h"
#include "ios/web/public/test/web_task_environment.h"
#include "ios/web/public/thread/web_task_traits.h"
#include "ios/web/public/thread/web_thread.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
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
  void AddOptionalObserver(
      mojo::PendingRemote<mojom_objc_test::mojom::EventObserver> observer)
      override {
    if (observer.is_valid()) {
      observers_.Add(std::move(observer));
    }
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

TEST_F(MojomObjcGeneratorPendingRemoteTest, NullableObserver) {
  [event_source_ addOptionalObserver:nil];

  MojomObjcTestTestEventObserver* observer =
      [[MojomObjcTestTestEventObserver alloc] init];
  [event_source_ addOptionalObserver:observer];
  ASSERT_TRUE(
      base::test::RunUntil([&] { return impl_.observer_count() >= 1u; }));

  // Messages on a pipe are handled in order, so the nil registration has been
  // handled by now and shouldn't have added an observer.
  EXPECT_EQ(impl_.observer_count(), 1u);
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

namespace {

// Holds on to callbacks without ever running them until told to answer.
class TestResponder : public mojom_objc_test::mojom::ResponderInterface {
 public:
  TestResponder() = default;
  ~TestResponder() override = default;

  mojo::PendingRemote<mojom_objc_test::mojom::ResponderInterface>
  BindNewRemote() {
    return receiver_.BindNewPipeAndPassRemote();
  }

  void Disconnect() {
    // Close the pipe before dropping callbacks, mojo doesn't allow dropping a
    // reply callback whose pipe is still open.
    receiver_.reset();
    ask_callbacks_.clear();
    verify_callbacks_.clear();
  }

  size_t ask_count() const { return ask_callbacks_.size(); }
  size_t verify_count() const { return verify_callbacks_.size(); }

  void AnswerAll() {
    for (auto& callback : ask_callbacks_) {
      std::move(callback).Run(
          "answer", 42, mojom_objc_test::mojom::SomeEnum::kGamma,
          mojom_objc_test::mojom::NullableEnumStruct::New(), nullptr);
    }
    ask_callbacks_.clear();
  }

  // mojom::ResponderInterface:
  void Ask(const std::string& question, AskCallback callback) override {
    ask_callbacks_.push_back(std::move(callback));
  }
  void Verify(const std::string& token, VerifyCallback callback) override {
    verify_callbacks_.push_back(std::move(callback));
  }

 private:
  mojo::Receiver<mojom_objc_test::mojom::ResponderInterface> receiver_{this};
  std::vector<AskCallback> ask_callbacks_;
  std::vector<VerifyCallback> verify_callbacks_;
};

}  // namespace

class MojomObjcGeneratorDroppedCallbackTest : public PlatformTest {
 protected:
  void SetUp() override {
    PlatformTest::SetUp();
    responder_ = [[MojomObjcTestResponderInterfaceMojoImpl alloc]
        initWithResponderInterface:impl_.BindNewRemote()];
  }

  void TearDown() override {
    responder_ = nil;
    PlatformTest::TearDown();
  }

  web::WebTaskEnvironment task_environment_;
  TestResponder impl_;
  MojomObjcTestResponderInterfaceMojoImpl* responder_ = nil;
  int call_count_ = 0;
  bool dropped_ = false;
};

// A disconnect with a call in flight should still run the completion, with
// default values, so callers awaiting it are not left hanging.
TEST_F(MojomObjcGeneratorDroppedCallbackTest, DisconnectRunsCompletionOnce) {
  __block NSString* answer = nil;
  __block int32_t count = -1;
  __block MojomObjcTestSomeEnum kind = MojomObjcTestSomeEnumGamma;
  __block MojomObjcTestNullableEnumStruct* info = nil;
  __block MojomObjcTestNullableEnumStruct* maybe_info = nil;

  [responder_ ask:@"question"
       completion:^(NSString* a, int32_t c, MojomObjcTestSomeEnum k,
                    MojomObjcTestNullableEnumStruct* i,
                    MojomObjcTestNullableEnumStruct* _Nullable m) {
         ++call_count_;
         answer = a;
         count = c;
         kind = k;
         info = i;
         maybe_info = m;
       }];
  ASSERT_TRUE(base::test::RunUntil([&] { return impl_.ask_count() == 1u; }));
  EXPECT_EQ(call_count_, 0);

  impl_.Disconnect();
  ASSERT_TRUE(base::test::RunUntil([&] { return call_count_ > 0; }));

  EXPECT_EQ(call_count_, 1);
  EXPECT_NSEQ(answer, @"");
  EXPECT_EQ(count, 0);
  EXPECT_EQ(kind, MojomObjcTestSomeEnumAlpha);
  EXPECT_NE(info, nil);
  EXPECT_EQ(maybe_info, nil);
}

TEST_F(MojomObjcGeneratorDroppedCallbackTest, AnsweredCompletionRunsOnce) {
  __block NSString* answer = nil;
  [responder_ ask:@"question"
       completion:^(NSString* a, int32_t c, MojomObjcTestSomeEnum k,
                    MojomObjcTestNullableEnumStruct* i,
                    MojomObjcTestNullableEnumStruct* _Nullable m) {
         ++call_count_;
         answer = a;
       }];
  ASSERT_TRUE(base::test::RunUntil([&] { return impl_.ask_count() == 1u; }));

  impl_.AnswerAll();
  ASSERT_TRUE(base::test::RunUntil([&] { return call_count_ > 0; }));
  impl_.Disconnect();

  // A call on the now disconnected remote is dropped, and completes with
  // defaults, once the disconnect has been observed on the Obj-C side. The
  // already answered completion mustn't run again by then.
  [responder_ ask:@"another question"
       completion:^(NSString* a, int32_t c, MojomObjcTestSomeEnum k,
                    MojomObjcTestNullableEnumStruct* i,
                    MojomObjcTestNullableEnumStruct* _Nullable m) {
         dropped_ = true;
       }];
  ASSERT_TRUE(base::test::RunUntil([&] { return dropped_; }));

  EXPECT_EQ(call_count_, 1);
  EXPECT_NSEQ(answer, @"answer");
}

// A dropped `result<S, F>` callback reports a default failure.
TEST_F(MojomObjcGeneratorDroppedCallbackTest, DisconnectReportsResultFailure) {
  __block MojomObjcTestNullableEnumStruct* success = nil;
  __block NSString* failure = nil;
  [responder_ verify:@"token"
          completion:^(MojomObjcTestNullableEnumStruct* _Nullable s,
                       NSString* _Nullable f) {
            ++call_count_;
            success = s;
            failure = f;
          }];
  ASSERT_TRUE(base::test::RunUntil([&] { return impl_.verify_count() == 1u; }));

  impl_.Disconnect();
  ASSERT_TRUE(base::test::RunUntil([&] { return call_count_ > 0; }));

  EXPECT_EQ(call_count_, 1);
  EXPECT_EQ(success, nil);
  EXPECT_NSEQ(failure, @"");
}

namespace {

class TestEmptyResponseHost
    : public mojom_objc_test::mojom::EmptyResponseInterfaceHost {
 public:
  mojo::PendingRemote<mojom_objc_test::mojom::EmptyResponseInterfaceHost>
  BindNewRemote() {
    return receiver_.BindNewPipeAndPassRemote();
  }

  // mojom::EmptyResponseInterfaceHost:
  void SetInterface(
      mojo::PendingRemote<mojom_objc_test::mojom::EmptyResponseInterface>
          remote) override {
    remote_.Bind(std::move(remote));
  }

  mojo::Remote<mojom_objc_test::mojom::EmptyResponseInterface> remote_;

 private:
  mojo::Receiver<mojom_objc_test::mojom::EmptyResponseInterfaceHost> receiver_{
      this};
};

class TestResponderHost
    : public mojom_objc_test::mojom::ResponderInterfaceHost {
 public:
  mojo::PendingRemote<mojom_objc_test::mojom::ResponderInterfaceHost>
  BindNewRemote() {
    return receiver_.BindNewPipeAndPassRemote();
  }

  // mojom::ResponderInterfaceHost:
  void SetResponder(
      mojo::PendingRemote<mojom_objc_test::mojom::ResponderInterface> remote)
      override {
    remote_.Bind(std::move(remote));
  }

  mojo::Remote<mojom_objc_test::mojom::ResponderInterface> remote_;

 private:
  mojo::Receiver<mojom_objc_test::mojom::ResponderInterfaceHost> receiver_{
      this};
};

}  // namespace

class MojomObjcGeneratorPendingRemoteWithRepliesTest : public PlatformTest {
 protected:
  web::WebTaskEnvironment task_environment_;
};

TEST_F(MojomObjcGeneratorPendingRemoteWithRepliesTest, EmptyResponse) {
  TestEmptyResponseHost host;
  auto* host_wrapper = [[MojomObjcTestEmptyResponseInterfaceHostMojoImpl alloc]
      initWithEmptyResponseInterfaceHost:host.BindNewRemote()];

  __block std::vector<std::string> received;
  MojomObjcTestTestEmptyResponseInterface* impl =
      [[MojomObjcTestTestEmptyResponseInterface alloc] init];
  impl._withParam = ^(NSString* value, void (^completion)(void)) {
    received.push_back(base::SysNSStringToUTF8(value));
    completion();
  };
  impl._withoutParams = ^(void (^completion)(void)) {
    received.push_back("no params");
    completion();
  };
  [host_wrapper setInterface:impl];

  ASSERT_TRUE(base::test::RunUntil([&] { return host.remote_.is_bound(); }));

  int replies = 0;
  host.remote_->WithParam("value",
                          base::BindLambdaForTesting([&] { replies++; }));
  host.remote_->WithoutParams(base::BindLambdaForTesting([&] { replies++; }));
  ASSERT_TRUE(base::test::RunUntil([&] { return replies == 2; }));
  EXPECT_EQ(received, (std::vector<std::string>{"value", "no params"}));
}

TEST_F(MojomObjcGeneratorPendingRemoteWithRepliesTest, ResponseValues) {
  TestResponderHost host;
  auto* host_wrapper = [[MojomObjcTestResponderInterfaceHostMojoImpl alloc]
      initWithResponderInterfaceHost:host.BindNewRemote()];

  MojomObjcTestTestResponderInterface* impl =
      [[MojomObjcTestTestResponderInterface alloc] init];
  impl._ask =
      ^(NSString* question,
        void (^completion)(NSString*, int32_t, MojomObjcTestSomeEnum,
                           MojomObjcTestNullableEnumStruct*,
                           MojomObjcTestNullableEnumStruct* _Nullable)) {
        completion([question stringByAppendingString:@"!"], 7,
                   MojomObjcTestSomeEnumGamma,
                   [[MojomObjcTestNullableEnumStruct alloc] init], nil);
      };
  impl._verify =
      ^(NSString* token,
        void (^completion)(MojomObjcTestNullableEnumStruct* _Nullable,
                           NSString* _Nullable)) {
        if ([token isEqualToString:@"good"]) {
          completion([[MojomObjcTestNullableEnumStruct alloc] init], nil);
        } else {
          completion(nil, @"bad token");
        }
      };
  [host_wrapper setResponder:impl];
  ASSERT_TRUE(base::test::RunUntil([&] { return host.remote_.is_bound(); }));

  std::optional<std::string> answer;
  int32_t count = 0;
  mojom_objc_test::mojom::SomeEnum kind =
      mojom_objc_test::mojom::SomeEnum::kAlpha;
  bool has_info = false;
  bool has_maybe_info = true;
  host.remote_->Ask(
      "why", base::BindLambdaForTesting(
                 [&](const std::string& a, int32_t c,
                     mojom_objc_test::mojom::SomeEnum k,
                     mojom_objc_test::mojom::NullableEnumStructPtr info,
                     mojom_objc_test::mojom::NullableEnumStructPtr maybe_info) {
                   answer = a;
                   count = c;
                   kind = k;
                   has_info = !!info;
                   has_maybe_info = !!maybe_info;
                 }));
  ASSERT_TRUE(base::test::RunUntil([&] { return answer.has_value(); }));
  EXPECT_EQ(*answer, "why!");
  EXPECT_EQ(count, 7);
  EXPECT_EQ(kind, mojom_objc_test::mojom::SomeEnum::kGamma);
  EXPECT_TRUE(has_info);
  EXPECT_FALSE(has_maybe_info);

  using Result = base::expected<mojom_objc_test::mojom::NullableEnumStructPtr,
                                std::string>;
  std::optional<bool> good;
  host.remote_->Verify("good", base::BindLambdaForTesting([&](Result result) {
                         good = result.has_value();
                       }));
  ASSERT_TRUE(base::test::RunUntil([&] { return good.has_value(); }));
  EXPECT_TRUE(*good);

  std::optional<Result> bad;
  host.remote_->Verify(
      "bad", base::BindLambdaForTesting([&](Result r) { bad = std::move(r); }));
  ASSERT_TRUE(base::test::RunUntil([&] { return bad.has_value(); }));
  ASSERT_FALSE(bad->has_value());
  EXPECT_EQ(bad->error(), "bad token");
}

TEST_F(MojomObjcGeneratorPendingRemoteWithRepliesTest,
       CompletionFromBackgroundThreadRepliesOnUIThread) {
  TestResponderHost host;
  auto* host_wrapper = [[MojomObjcTestResponderInterfaceHostMojoImpl alloc]
      initWithResponderInterfaceHost:host.BindNewRemote()];

  MojomObjcTestTestResponderInterface* impl =
      [[MojomObjcTestTestResponderInterface alloc] init];
  impl._verify =
      ^(NSString* token,
        void (^completion)(MojomObjcTestNullableEnumStruct* _Nullable,
                           NSString* _Nullable)) {
        base::ThreadPool::PostTask(FROM_HERE, base::BindOnce(^{
                                     completion(nil, @"async failure");
                                   }));
      };
  [host_wrapper setResponder:impl];
  ASSERT_TRUE(base::test::RunUntil([&] { return host.remote_.is_bound(); }));

  using Result = base::expected<mojom_objc_test::mojom::NullableEnumStructPtr,
                                std::string>;
  std::optional<Result> result;
  bool replied_on_ui_thread = false;
  host.remote_->Verify(
      "token", base::BindLambdaForTesting([&](Result r) {
        replied_on_ui_thread =
            web::GetUIThreadTaskRunner({})->RunsTasksInCurrentSequence();
        result = std::move(r);
      }));
  ASSERT_TRUE(base::test::RunUntil([&] { return result.has_value(); }));
  ASSERT_FALSE(result->has_value());
  EXPECT_EQ(result->error(), "async failure");
  EXPECT_TRUE(replied_on_ui_thread);
}

using MojomObjcGeneratorMapTest = PlatformTest;

TEST_F(MojomObjcGeneratorMapTest, ObjCToCppConvertsAllEntries) {
  MojomObjcTestMapStruct* obj = [[MojomObjcTestMapStruct alloc] init];
  NSMutableDictionary<NSString*, NSNumber*>* numbers =
      [NSMutableDictionary dictionary];
  for (int i = 0; i < 100; ++i) {
    numbers[[NSString stringWithFormat:@"key%d", i]] = @(i);
  }
  obj.numbers = numbers;
  MojomObjcTestNullableEnumStruct* value =
      [[MojomObjcTestNullableEnumStruct alloc] init];
  value.requiredEnumField = MojomObjcTestSomeEnumGamma;
  obj.structs = @{@"b" : value, @"a" : value};

  mojom_objc_test::mojom::MapStructPtr cpp_obj = obj.cppObjPtr;
  ASSERT_EQ(cpp_obj->numbers.size(), 100u);
  EXPECT_EQ(cpp_obj->numbers.at("key0"), 0);
  EXPECT_EQ(cpp_obj->numbers.at("key99"), 99);
  ASSERT_EQ(cpp_obj->structs.size(), 2u);
  EXPECT_EQ(cpp_obj->structs.at("a")->required_enum_field,
            mojom_objc_test::mojom::SomeEnum::kGamma);
}

TEST_F(MojomObjcGeneratorMapTest, EmptyMap) {
  MojomObjcTestMapStruct* obj = [[MojomObjcTestMapStruct alloc] init];
  mojom_objc_test::mojom::MapStructPtr cpp_obj = obj.cppObjPtr;
  EXPECT_TRUE(cpp_obj->numbers.empty());
  EXPECT_TRUE(cpp_obj->structs.empty());
}
