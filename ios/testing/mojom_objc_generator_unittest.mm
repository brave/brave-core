// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/run_loop.h"
#include "brave/ios/testing/some_service.mojom-objc.h"
#include "brave/ios/testing/some_service.mojom.objc+private.h"
#include "brave/ios/testing/some_service.mojom.objc.h"
#include "ios/web/public/test/web_task_environment.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

namespace {

class SomeServiceImpl : public mojom::objc::SomeService {
 public:
  void ProcessArg(mojom::objc::SomeStructPtr some_struct,
                  ProcessArgCallback callback) override {
    auto result = mojom::objc::SomeStruct::New();
    result->my_value = some_struct->my_value + some_struct->my_value;
    result->my_count = some_struct->my_count + some_struct->my_count;
    std::move(callback).Run(std::move(result));
  }

  void ProcessArray(std::vector<mojom::objc::SomeStructPtr> some_structs,
                    ProcessArrayCallback callback) override {
    std::vector<mojom::objc::SomeStructPtr> result;
    for (const auto& item : some_structs) {
      auto out = mojom::objc::SomeStruct::New();
      out->my_value = item->my_value + item->my_value;
      out->my_count = item->my_count + item->my_count;
      result.push_back(std::move(out));
    }
    std::move(callback).Run(std::move(result));
  }

  void ProcessWrapper(mojom::objc::WrapperStructPtr wrapper_struct,
                      ProcessWrapperCallback callback) override {
    auto result = mojom::objc::WrapperStruct::New();
    result->value = wrapper_struct->value + wrapper_struct->value;
    result->some_struct = mojom::objc::SomeStruct::New();
    result->some_struct->my_value = wrapper_struct->some_struct->my_value +
                                    wrapper_struct->some_struct->my_value;
    result->some_struct->my_count = wrapper_struct->some_struct->my_count +
                                    wrapper_struct->some_struct->my_count;
    std::move(callback).Run(std::move(result));
  }

  void ProcessArgOpt(mojom::objc::SomeStructPtr some_struct,
                     ProcessArgOptCallback callback) override {
    if (!some_struct) {
      std::move(callback).Run(nullptr);
      return;
    }
    auto result = mojom::objc::SomeStruct::New();
    result->my_value = some_struct->my_value + some_struct->my_value;
    result->my_count = some_struct->my_count + some_struct->my_count;
    std::move(callback).Run(std::move(result));
  }

  void ProcessArrayOpt(std::vector<mojom::objc::SomeStructPtr> some_structs,
                       ProcessArrayOptCallback callback) override {
    std::vector<mojom::objc::SomeStructPtr> result;
    for (auto& item : some_structs) {
      if (!item) {
        result.push_back(nullptr);
        continue;
      }
      auto out = mojom::objc::SomeStruct::New();
      out->my_value = item->my_value + item->my_value;
      out->my_count = item->my_count + item->my_count;
      result.push_back(std::move(out));
    }
    std::move(callback).Run(std::move(result));
  }

  void ProcessWrapperOpt(mojom::objc::WrapperOptStructPtr wrapper_struct,
                         ProcessWrapperOptCallback callback) override {
    auto result = mojom::objc::WrapperOptStruct::New();
    result->value = wrapper_struct->value + wrapper_struct->value;
    if (wrapper_struct->some_struct) {
      result->some_struct = mojom::objc::SomeStruct::New();
      result->some_struct->my_value = wrapper_struct->some_struct->my_value +
                                      wrapper_struct->some_struct->my_value;
      result->some_struct->my_count = wrapper_struct->some_struct->my_count +
                                      wrapper_struct->some_struct->my_count;
    }
    std::move(callback).Run(std::move(result));
  }
};

}  // namespace

using MojomTraitsTest = PlatformTest;

TEST_F(MojomTraitsTest, ProcessArgRoundTrip) {
  web::WebTaskEnvironment task_environment;

  mojo::PendingRemote<mojom::objc::SomeService> pending_remote;
  mojo::MakeSelfOwnedReceiver(std::make_unique<SomeServiceImpl>(),
                              pending_remote.InitWithNewPipeAndPassReceiver());

  MojomSomeServiceMojoImpl* service = [[MojomSomeServiceMojoImpl alloc]
      initWithSomeService:std::move(pending_remote)];

  MojomSomeStruct* input_wrapper = [[MojomSomeStruct alloc] init];
  input_wrapper.myValue = @"brave";
  input_wrapper.myCount = 7;

  __block MojomSomeStruct* result = nil;
  base::RunLoop run_loop;
  base::RunLoop* run_loop_ptr = &run_loop;
  [service processArg:input_wrapper
           completion:^(MojomSomeStruct* r) {
             result = r;
             run_loop_ptr->Quit();
           }];
  run_loop.Run();

  ASSERT_NE(result, nil);
  EXPECT_TRUE([result.myValue isEqualToString:@"bravebrave"]);
  EXPECT_EQ(result.myCount, 14);
}

TEST_F(MojomTraitsTest, ProcessArrayRoundTrip) {
  web::WebTaskEnvironment task_environment;

  mojo::PendingRemote<mojom::objc::SomeService> pending_remote;
  mojo::MakeSelfOwnedReceiver(std::make_unique<SomeServiceImpl>(),
                              pending_remote.InitWithNewPipeAndPassReceiver());
  MojomSomeServiceMojoImpl* service = [[MojomSomeServiceMojoImpl alloc]
      initWithSomeService:std::move(pending_remote)];

  MojomSomeStruct* a = [[MojomSomeStruct alloc] init];
  a.myValue = @"foo";
  a.myCount = 1;
  MojomSomeStruct* b = [[MojomSomeStruct alloc] init];
  b.myValue = @"bar";
  b.myCount = 2;
  NSArray<MojomSomeStruct*>* input = @[ a, b ];

  __block NSArray<MojomSomeStruct*>* result = nil;
  base::RunLoop run_loop;
  base::RunLoop* run_loop_ptr = &run_loop;
  [service processArray:input
             completion:^(NSArray<MojomSomeStruct*>* r) {
               result = r;
               run_loop_ptr->Quit();
             }];
  run_loop.Run();

  ASSERT_EQ(result.count, 2u);
  EXPECT_TRUE([result[0].myValue isEqualToString:@"foofoo"]);
  EXPECT_EQ(result[0].myCount, 2);
  EXPECT_TRUE([result[1].myValue isEqualToString:@"barbar"]);
  EXPECT_EQ(result[1].myCount, 4);
}

TEST_F(MojomTraitsTest, ProcessWrapperRoundTrip) {
  web::WebTaskEnvironment task_environment;

  mojo::PendingRemote<mojom::objc::SomeService> pending_remote;
  mojo::MakeSelfOwnedReceiver(std::make_unique<SomeServiceImpl>(),
                              pending_remote.InitWithNewPipeAndPassReceiver());
  MojomSomeServiceMojoImpl* service = [[MojomSomeServiceMojoImpl alloc]
      initWithSomeService:std::move(pending_remote)];

  MojomSomeStruct* inner_struct = [[MojomSomeStruct alloc] init];
  inner_struct.myValue = @"hello";
  inner_struct.myCount = 3;
  MojomWrapperStruct* input =
      [[MojomWrapperStruct alloc] initWithSomeStruct:inner_struct
                                               value:@"outer"];

  __block MojomWrapperStruct* result = nil;
  base::RunLoop run_loop;
  base::RunLoop* run_loop_ptr = &run_loop;
  [service processWrapper:input
               completion:^(MojomWrapperStruct* r) {
                 result = r;
                 run_loop_ptr->Quit();
               }];
  run_loop.Run();

  ASSERT_NE(result, nil);
  EXPECT_TRUE([result.value isEqualToString:@"outerouter"]);
  EXPECT_TRUE([result.someStruct.myValue isEqualToString:@"hellohello"]);
  EXPECT_EQ(result.someStruct.myCount, 6);
}

TEST_F(MojomTraitsTest, ProcessArgOptRoundTrip) {
  web::WebTaskEnvironment task_environment;

  mojo::PendingRemote<mojom::objc::SomeService> pending_remote;
  mojo::MakeSelfOwnedReceiver(std::make_unique<SomeServiceImpl>(),
                              pending_remote.InitWithNewPipeAndPassReceiver());
  MojomSomeServiceMojoImpl* service = [[MojomSomeServiceMojoImpl alloc]
      initWithSomeService:std::move(pending_remote)];

  {
    __block MojomSomeStruct* result = nil;
    base::RunLoop run_loop;
    base::RunLoop* run_loop_ptr = &run_loop;
    [service processArgOpt:nil
                completion:^(MojomSomeStruct* _Nullable r) {
                  result = r;
                  run_loop_ptr->Quit();
                }];
    run_loop.Run();

    EXPECT_EQ(result, nil);
  }

  {
    MojomSomeStruct* input_wrapper = [[MojomSomeStruct alloc] init];
    input_wrapper.myValue = @"brave";
    input_wrapper.myCount = 7;

    __block MojomSomeStruct* result = nil;
    base::RunLoop run_loop;
    base::RunLoop* run_loop_ptr = &run_loop;
    [service processArgOpt:input_wrapper
                completion:^(MojomSomeStruct* _Nullable r) {
                  result = r;
                  run_loop_ptr->Quit();
                }];
    run_loop.Run();

    ASSERT_NE(result, nil);
    EXPECT_TRUE([result.myValue isEqualToString:@"bravebrave"]);
    EXPECT_EQ(result.myCount, 14);
  }
}

TEST_F(MojomTraitsTest, ProcessArrayOptRoundTrip) {
  web::WebTaskEnvironment task_environment;

  mojo::PendingRemote<mojom::objc::SomeService> pending_remote;
  mojo::MakeSelfOwnedReceiver(std::make_unique<SomeServiceImpl>(),
                              pending_remote.InitWithNewPipeAndPassReceiver());
  MojomSomeServiceMojoImpl* service = [[MojomSomeServiceMojoImpl alloc]
      initWithSomeService:std::move(pending_remote)];

  MojomSomeStruct* a = [[MojomSomeStruct alloc] init];
  a.myValue = @"foo";
  a.myCount = 1;
  MojomSomeStruct* c = [[MojomSomeStruct alloc] init];
  c.myValue = @"bar";
  c.myCount = 2;
  NSMutableArray<MojomSomeStruct*>* input = [NSMutableArray array];
  [input addObject:a];
  [input addObject:(MojomSomeStruct*)[NSNull null]];
  [input addObject:c];

  __block NSArray<MojomSomeStruct*>* result = nil;
  base::RunLoop run_loop;
  base::RunLoop* run_loop_ptr = &run_loop;
  [service processArrayOpt:input
                completion:^(NSArray<MojomSomeStruct*>* r) {
                  result = r;
                  run_loop_ptr->Quit();
                }];
  run_loop.Run();

  ASSERT_EQ(result.count, 3u);
  ASSERT_NE(result[0], nil);
  EXPECT_TRUE([result[0].myValue isEqualToString:@"foofoo"]);
  EXPECT_EQ(result[0].myCount, 2);
  EXPECT_EQ(result[1], (MojomSomeStruct*)[NSNull null]);
  ASSERT_NE(result[2], nil);
  EXPECT_TRUE([result[2].myValue isEqualToString:@"barbar"]);
  EXPECT_EQ(result[2].myCount, 4);
}

TEST_F(MojomTraitsTest, ProcessWrapperOptRoundTrip) {
  web::WebTaskEnvironment task_environment;

  mojo::PendingRemote<mojom::objc::SomeService> pending_remote;
  mojo::MakeSelfOwnedReceiver(std::make_unique<SomeServiceImpl>(),
                              pending_remote.InitWithNewPipeAndPassReceiver());
  MojomSomeServiceMojoImpl* service = [[MojomSomeServiceMojoImpl alloc]
      initWithSomeService:std::move(pending_remote)];

  {
    MojomWrapperOptStruct* input =
        [[MojomWrapperOptStruct alloc] initWithSomeStruct:nil value:@"outer"];

    __block MojomWrapperOptStruct* result = nil;
    base::RunLoop run_loop;
    base::RunLoop* run_loop_ptr = &run_loop;
    [service processWrapperOpt:input
                    completion:^(MojomWrapperOptStruct* r) {
                      result = r;
                      run_loop_ptr->Quit();
                    }];
    run_loop.Run();

    ASSERT_NE(result, nil);
    EXPECT_TRUE([result.value isEqualToString:@"outerouter"]);
    EXPECT_EQ(result.someStruct, nil);
  }

  {
    MojomSomeStruct* inner_struct = [[MojomSomeStruct alloc] init];
    inner_struct.myValue = @"hello";
    inner_struct.myCount = 3;
    MojomWrapperOptStruct* input =
        [[MojomWrapperOptStruct alloc] initWithSomeStruct:inner_struct
                                                    value:@"outer"];

    __block MojomWrapperOptStruct* result = nil;
    base::RunLoop run_loop;
    base::RunLoop* run_loop_ptr = &run_loop;
    [service processWrapperOpt:input
                    completion:^(MojomWrapperOptStruct* r) {
                      result = r;
                      run_loop_ptr->Quit();
                    }];
    run_loop.Run();

    ASSERT_NE(result, nil);
    EXPECT_TRUE([result.value isEqualToString:@"outerouter"]);
    ASSERT_NE(result.someStruct, nil);
    EXPECT_TRUE([result.someStruct.myValue isEqualToString:@"hellohello"]);
    EXPECT_EQ(result.someStruct.myCount, 6);
  }
}
