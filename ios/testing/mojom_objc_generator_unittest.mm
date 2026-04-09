// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/run_loop.h"
#include "brave/ios/testing/some_service.mojom.h"
#include "brave/ios/testing/some_service.mojom.objc+private.h"
#include "brave/ios/testing/some_service.mojom.objc.h"
#include "brave/ios/testing/some_struct_cpp.h"
#include "ios/web/public/test/web_task_environment.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

namespace {

class SomeServiceImpl : public mojom::SomeService {
 public:
  void ProcessArg(const ::SomeStructCpp& some_struct,
                  ProcessArgCallback callback) override {
    SomeStructCpp result;
    result.value = some_struct.value + some_struct.value;
    result.count = some_struct.count + some_struct.count;
    std::move(callback).Run(result);
  }

  void ProcessArray(const std::vector<::SomeStructCpp>& some_structs,
                    ProcessArrayCallback callback) override {
    std::vector<::SomeStructCpp> result;
    for (const auto& item : some_structs) {
      SomeStructCpp out;
      out.value = item.value + item.value;
      out.count = item.count + item.count;
      result.push_back(out);
    }
    std::move(callback).Run(result);
  }

  void ProcessWrapper(mojom::WrapperStructPtr wrapper_struct,
                      ProcessWrapperCallback callback) override {
    mojom::WrapperStructPtr result = mojom::WrapperStruct::New();
    result->value = wrapper_struct->value + wrapper_struct->value;
    result->some_struct.value =
        wrapper_struct->some_struct.value + wrapper_struct->some_struct.value;
    result->some_struct.count =
        wrapper_struct->some_struct.count + wrapper_struct->some_struct.count;
    std::move(callback).Run(std::move(result));
  }

  void ProcessArgOpt(const std::optional<::SomeStructCpp>& some_struct,
                     ProcessArgOptCallback callback) override {
    if (!some_struct) {
      std::move(callback).Run(std::nullopt);
      return;
    }
    SomeStructCpp result;
    result.value = some_struct->value + some_struct->value;
    result.count = some_struct->count + some_struct->count;
    std::move(callback).Run(result);
  }

  void ProcessArrayOpt(
      const std::vector<std::optional<::SomeStructCpp>>& some_structs,
      ProcessArrayOptCallback callback) override {
    std::vector<std::optional<::SomeStructCpp>> result;
    for (const auto& item : some_structs) {
      if (!item) {
        result.push_back(std::nullopt);
        continue;
      }
      SomeStructCpp out;
      out.value = item->value + item->value;
      out.count = item->count + item->count;
      result.push_back(std::move(out));
    }
    std::move(callback).Run(std::move(result));
  }

  void ProcessWrapperOpt(mojom::WrapperOptStructPtr wrapper_struct,
                         ProcessWrapperOptCallback callback) override {
    mojom::WrapperOptStructPtr result = mojom::WrapperOptStruct::New();
    result->value = wrapper_struct->value + wrapper_struct->value;
    if (wrapper_struct->some_struct) {
      SomeStructCpp out;
      out.value = wrapper_struct->some_struct->value +
                  wrapper_struct->some_struct->value;
      out.count = wrapper_struct->some_struct->count +
                  wrapper_struct->some_struct->count;
      result->some_struct = std::move(out);
    }
    std::move(callback).Run(std::move(result));
  }
};

}  // namespace

using MojomTraitsTest = PlatformTest;

TEST_F(MojomTraitsTest, ProcessArgRoundTrip) {
  web::WebTaskEnvironment task_environment;

  mojo::PendingRemote<mojom::SomeService> pending_remote;
  mojo::MakeSelfOwnedReceiver(std::make_unique<SomeServiceImpl>(),
                              pending_remote.InitWithNewPipeAndPassReceiver());

  MojomSomeServiceMojoImpl* service = [[MojomSomeServiceMojoImpl alloc]
      initWithSomeService:std::move(pending_remote)];

  MojomSomeStruct* input_wrapper = [[MojomSomeStruct alloc] init];
  input_wrapper.value = @"brave";
  input_wrapper.count = 7;

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
  EXPECT_TRUE([result.value isEqualToString:@"bravebrave"]);
  EXPECT_EQ(result.count, 14);
}

TEST_F(MojomTraitsTest, ProcessArrayRoundTrip) {
  web::WebTaskEnvironment task_environment;

  mojo::PendingRemote<mojom::SomeService> pending_remote;
  mojo::MakeSelfOwnedReceiver(std::make_unique<SomeServiceImpl>(),
                              pending_remote.InitWithNewPipeAndPassReceiver());
  MojomSomeServiceMojoImpl* service = [[MojomSomeServiceMojoImpl alloc]
      initWithSomeService:std::move(pending_remote)];

  MojomSomeStruct* a = [[MojomSomeStruct alloc] init];
  a.value = @"foo";
  a.count = 1;
  MojomSomeStruct* b = [[MojomSomeStruct alloc] init];
  b.value = @"bar";
  b.count = 2;
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
  EXPECT_TRUE([result[0].value isEqualToString:@"foofoo"]);
  EXPECT_EQ(result[0].count, 2);
  EXPECT_TRUE([result[1].value isEqualToString:@"barbar"]);
  EXPECT_EQ(result[1].count, 4);
}

TEST_F(MojomTraitsTest, ProcessWrapperRoundTrip) {
  web::WebTaskEnvironment task_environment;

  mojo::PendingRemote<mojom::SomeService> pending_remote;
  mojo::MakeSelfOwnedReceiver(std::make_unique<SomeServiceImpl>(),
                              pending_remote.InitWithNewPipeAndPassReceiver());
  MojomSomeServiceMojoImpl* service = [[MojomSomeServiceMojoImpl alloc]
      initWithSomeService:std::move(pending_remote)];

  MojomSomeStruct* inner_struct = [[MojomSomeStruct alloc] init];
  inner_struct.value = @"hello";
  inner_struct.count = 3;
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
  EXPECT_TRUE([result.someStruct.value isEqualToString:@"hellohello"]);
  EXPECT_EQ(result.someStruct.count, 6);
}

TEST_F(MojomTraitsTest, ProcessArgOptRoundTrip) {
  web::WebTaskEnvironment task_environment;

  mojo::PendingRemote<mojom::SomeService> pending_remote;
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
    input_wrapper.value = @"brave";
    input_wrapper.count = 7;

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
    EXPECT_TRUE([result.value isEqualToString:@"bravebrave"]);
    EXPECT_EQ(result.count, 14);
  }
}

TEST_F(MojomTraitsTest, ProcessArrayOptRoundTrip) {
  web::WebTaskEnvironment task_environment;

  mojo::PendingRemote<mojom::SomeService> pending_remote;
  mojo::MakeSelfOwnedReceiver(std::make_unique<SomeServiceImpl>(),
                              pending_remote.InitWithNewPipeAndPassReceiver());
  MojomSomeServiceMojoImpl* service = [[MojomSomeServiceMojoImpl alloc]
      initWithSomeService:std::move(pending_remote)];

  MojomSomeStruct* a = [[MojomSomeStruct alloc] init];
  a.value = @"foo";
  a.count = 1;
  MojomSomeStruct* c = [[MojomSomeStruct alloc] init];
  c.value = @"bar";
  c.count = 2;
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
  EXPECT_TRUE([result[0].value isEqualToString:@"foofoo"]);
  EXPECT_EQ(result[0].count, 2);
  EXPECT_EQ(result[1], (MojomSomeStruct*)[NSNull null]);
  ASSERT_NE(result[2], nil);
  EXPECT_TRUE([result[2].value isEqualToString:@"barbar"]);
  EXPECT_EQ(result[2].count, 4);
}

TEST_F(MojomTraitsTest, ProcessWrapperOptRoundTrip) {
  web::WebTaskEnvironment task_environment;

  mojo::PendingRemote<mojom::SomeService> pending_remote;
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
    inner_struct.value = @"hello";
    inner_struct.count = 3;
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
    EXPECT_TRUE([result.someStruct.value isEqualToString:@"hellohello"]);
    EXPECT_EQ(result.someStruct.count, 6);
  }
}
