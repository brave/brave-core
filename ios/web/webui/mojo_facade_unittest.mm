// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ios/web/webui/mojo_facade.h"

#include <memory>

#include "base/functional/bind.h"
#include "base/memory/raw_ptr.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/ios/wait_util.h"
#include "ios/web/public/test/fakes/fake_web_frame.h"
#include "ios/web/public/test/fakes/fake_web_frames_manager.h"
#include "ios/web/public/test/fakes/fake_web_state.h"
#include "ios/web/public/test/web_test.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

using base::test::ios::kWaitForJSCompletionTimeout;
using base::test::ios::WaitUntilConditionOrTimeout;

namespace web {

namespace {

// Serializes the given `object` to JSON string.
std::string GetJson(id object) {
  NSData* json_as_data = [NSJSONSerialization dataWithJSONObject:object
                                                         options:0
                                                           error:nil];
  NSString* json_as_string =
      [[NSString alloc] initWithData:json_as_data
                            encoding:NSUTF8StringEncoding];
  return base::SysNSStringToUTF8(json_as_string);
}

// Deserializes the given `json` to an object.
id GetObject(const std::string& json) {
  NSData* json_as_data =
      [base::SysUTF8ToNSString(json) dataUsingEncoding:NSUTF8StringEncoding];
  return [NSJSONSerialization JSONObjectWithData:json_as_data
                                         options:0
                                           error:nil];
}

class FakeWebStateWithInterfaceBinder : public FakeWebState {
 public:
  InterfaceBinder* GetInterfaceBinderForMainFrame() override {
    return &interface_binder_;
  }

 private:
  InterfaceBinder interface_binder_{this};
};

}  // namespace

// Frame type enum for parameterized tests
enum class FrameType { kMainFrame, kChildFrame };

// A test fixture to test MojoFacade class. It creates a frames manager, along
// with a main & child frame associated with it and copies the neccesssary
// helper methods that exist in //ios/web/webui/mojo_facade_unittest.mm
class MojoFacadeTest : public WebTest {
 protected:
  MojoFacadeTest() {
    facade_ = std::make_unique<MojoFacade>(&web_state_);

    auto web_frames_manager = std::make_unique<web::FakeWebFramesManager>();
    frames_manager_ = web_frames_manager.get();
    web_state_.SetWebFramesManager(std::move(web_frames_manager));

    auto main_frame = FakeWebFrame::CreateMainWebFrame();
    auto child_frame = FakeWebFrame::CreateChildWebFrame();

    main_frame_ = main_frame.get();
    child_frame_ = child_frame.get();
    frames_manager_->AddWebFrame(std::move(main_frame));
    frames_manager_->AddWebFrame(std::move(child_frame));
  }

  FakeWebFrame* main_frame() { return main_frame_; }
  FakeWebFrame* child_frame() { return child_frame_; }
  MojoFacade* facade() { return facade_.get(); }

  void CreateMessagePipe(uint32_t* handle0, uint32_t* handle1) {
    NSDictionary* create = @{
      @"name" : @"Mojo.createMessagePipe",
      @"args" : @{},
    };
    std::string response_as_string =
        facade()->HandleMojoMessage(GetJson(create));

    // Verify handles.
    ASSERT_FALSE(response_as_string.empty());
    NSDictionary* response_as_dict = GetObject(response_as_string);
    ASSERT_TRUE([response_as_dict isKindOfClass:[NSDictionary class]]);
    ASSERT_EQ(MOJO_RESULT_OK, [response_as_dict[@"result"] unsignedIntValue]);
    *handle0 = [response_as_dict[@"handle0"] unsignedIntValue];
    *handle1 = [response_as_dict[@"handle1"] unsignedIntValue];
  }

  void CloseHandle(uint32_t handle) {
    NSDictionary* close = @{
      @"name" : @"MojoHandle.close",
      @"args" : @{
        @"handle" : @(handle),
      },
    };
    std::string result = facade()->HandleMojoMessage(GetJson(close));
    EXPECT_TRUE(result.empty());
  }

  NSDictionary* ReadMessage(uint32_t handle) {
    // Read the message from the pipe.
    NSDictionary* read = @{
      @"name" : @"MojoHandle.readMessage",
      @"args" : @{
        @"handle" : @(handle),
      },
    };
    NSDictionary* message =
        GetObject(facade()->HandleMojoMessage(GetJson(read)));
    EXPECT_TRUE([message isKindOfClass:[NSDictionary class]]);
    return message;
  }

  int WatchHandle(uint32_t handle, int callback_id, std::string frame_id) {
    NSDictionary* watch = @{
      @"name" : @"MojoHandle.watch",
      @"args" : @{
        @"handle" : @(handle),
        @"signals" : @(MOJO_HANDLE_SIGNAL_READABLE),
        @"callbackId" : @(callback_id),
        @"frameId" : base::SysUTF8ToNSString(frame_id)
      },
    };
    const std::string watch_id_as_string =
        facade()->HandleMojoMessage(GetJson(watch));
    EXPECT_FALSE(watch_id_as_string.empty());
    int watch_id = 0;
    EXPECT_TRUE(base::StringToInt(watch_id_as_string, &watch_id));
    return watch_id;
  }

  void CancelWatch(uint32_t handle, int watch_id) {
    NSDictionary* cancel_watch = @{
      @"name" : @"MojoWatcher.cancel",
      @"args" : @{
        @"watchId" : @(watch_id),
      },
    };
    EXPECT_TRUE(facade_->HandleMojoMessage(GetJson(cancel_watch)).empty());
  }

  void WriteMessage(uint32_t handle, NSString* buffer) {
    NSDictionary* write = @{
      @"name" : @"MojoHandle.writeMessage",
      @"args" : @{@"handle" : @(handle), @"handles" : @[], @"buffer" : buffer},
    };
    const std::string result_as_string =
        facade()->HandleMojoMessage(GetJson(write));
    EXPECT_FALSE(result_as_string.empty());
    unsigned result = 0u;
    EXPECT_TRUE(base::StringToUint(result_as_string, &result));
    EXPECT_EQ(MOJO_RESULT_OK, result);
  }

  std::string WaitForLastJavaScriptCallOnFrame(FakeWebFrame* frame) {
    EXPECT_TRUE(WaitUntilConditionOrTimeout(kWaitForJSCompletionTimeout, ^bool {
      // Flush any pending tasks. Don't RunUntilIdle() because
      // RunUntilIdle() is incompatible with mojo::SimpleWatcher's
      // automatic arming behavior, which Mojo JS still depends upon.
      base::RunLoop loop;
      base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(
          FROM_HERE, loop.QuitClosure());
      loop.Run();
      return !frame->GetLastJavaScriptCall().empty();
    }));

    const auto last_js_call = frame->GetLastJavaScriptCall();
    frame->ClearJavaScriptCallHistory();
    return base::UTF16ToUTF8(last_js_call);
  }

 private:
  FakeWebStateWithInterfaceBinder web_state_;
  raw_ptr<web::FakeWebFramesManager> frames_manager_;
  raw_ptr<FakeWebFrame> main_frame_;
  raw_ptr<FakeWebFrame> child_frame_;
  std::unique_ptr<MojoFacade> facade_;
};

// Test to ensure that even when an invalid frame ID is pased into the watcher
// it will execute the callback on the main frame by default
TEST_F(MojoFacadeTest, WatchWithInvalidFrameId) {
  uint32_t handle0, handle1;
  CreateMessagePipe(&handle0, &handle1);

  // Start watching one end of the pipe.
  const int kCallbackId = 99;
  WatchHandle(handle0, kCallbackId, "invalid");

  // Write to the other end of the pipe.
  WriteMessage(handle1, @"QUJDRA==");  // "ABCD" in base-64

  const auto expected_script =
      absl::StrFormat("Mojo.internal.watchCallbacksHolder.callCallback(%d, %d)",
                      kCallbackId, MOJO_RESULT_OK);
  EXPECT_EQ(expected_script, WaitForLastJavaScriptCallOnFrame(main_frame()));

  CloseHandle(handle0);
  CloseHandle(handle1);
}

// Parameterized test fixture for frame-specific tests. The actual logic being
// tested is copied from //ios/web/webui/mojo_facade_unittest.mm with the
// exception that we are testing it on both the main & child frames rather
// than just main. This ensures that our patch & overrides work correctly and
// that the mojo communication JavaScript is being run on the correct frame
// based on the frame ID passed in. This suite only copies tests related to the
// Watch commands as they are the ones that now pass in a frame ID
class MojoFacadeWatchFramesTest
    : public MojoFacadeTest,
      public ::testing::WithParamInterface<FrameType> {
 protected:
  // Helper method for parameterized tests
  FakeWebFrame* GetFrameForType(FrameType frame_type) {
    return frame_type == FrameType::kMainFrame ? main_frame() : child_frame();
  }
};

// Tests watching the pipe.
TEST_P(MojoFacadeWatchFramesTest, Watch) {
  const FrameType frame_type = GetParam();
  FakeWebFrame* test_frame = GetFrameForType(frame_type);
  const std::string frame_id = test_frame->GetFrameId();

  uint32_t handle0, handle1;
  CreateMessagePipe(&handle0, &handle1);

  // Start watching one end of the pipe.
  const int kCallbackId = 99;
  WatchHandle(handle0, kCallbackId, frame_id);

  // Write to the other end of the pipe.
  WriteMessage(handle1, @"QUJDRA==");  // "ABCD" in base-64

  const auto expected_script =
      absl::StrFormat("Mojo.internal.watchCallbacksHolder.callCallback(%d, %d)",
                      kCallbackId, MOJO_RESULT_OK);
  EXPECT_EQ(expected_script, WaitForLastJavaScriptCallOnFrame(test_frame));

  CloseHandle(handle0);
  CloseHandle(handle1);
}

TEST_P(MojoFacadeWatchFramesTest, WatcherRearming) {
  const FrameType frame_type = GetParam();
  FakeWebFrame* test_frame = GetFrameForType(frame_type);
  const std::string frame_id = test_frame->GetFrameId();

  uint32_t handle0, handle1;
  CreateMessagePipe(&handle0, &handle1);

  // Start watching one end of the pipe.
  const int kCallbackId = 99;
  WatchHandle(handle0, kCallbackId, frame_id);
  const auto expected_script =
      absl::StrFormat("Mojo.internal.watchCallbacksHolder.callCallback(%d, %d)",
                      kCallbackId, MOJO_RESULT_OK);

  // Write to the other end of the pipe.
  WriteMessage(handle1, @"QUJDRA==");  // "ABCD" in base-64

  EXPECT_EQ(expected_script, WaitForLastJavaScriptCallOnFrame(test_frame));

  // Read the pipe until MOJO_RESULT_SHOULD_WAIT is returned
  // (the usual watcher behavior).
  EXPECT_EQ([ReadMessage(handle0)[@"result"] unsignedIntValue], MOJO_RESULT_OK);
  EXPECT_EQ([ReadMessage(handle0)[@"result"] unsignedIntValue],
            MOJO_RESULT_SHOULD_WAIT);

  // Write to the other end of the pipe.
  WriteMessage(handle1, @"QUJDRA==");  // "ABCD" in base-64

  // Check the watcher was reamed and works.
  EXPECT_EQ(expected_script, WaitForLastJavaScriptCallOnFrame(test_frame));

  CloseHandle(handle0);
  CloseHandle(handle1);
}

TEST_P(MojoFacadeWatchFramesTest, CancelWatch) {
  const FrameType frame_type = GetParam();
  FakeWebFrame* test_frame = GetFrameForType(frame_type);
  const std::string frame_id = test_frame->GetFrameId();

  uint32_t handle0, handle1;
  CreateMessagePipe(&handle0, &handle1);

  // Make 2 watchers on one end of the pipe.
  const int kCallbackId1 = 99;
  const int kCallbackId2 = 101;
  WatchHandle(handle0, kCallbackId1, frame_id);
  const int watch_id2 = WatchHandle(handle0, kCallbackId2, frame_id);
  const auto expected_script1 =
      absl::StrFormat("Mojo.internal.watchCallbacksHolder.callCallback(%d, %d)",
                      kCallbackId1, MOJO_RESULT_OK);
  const auto expected_script2 =
      absl::StrFormat("Mojo.internal.watchCallbacksHolder.callCallback(%d, %d)",
                      kCallbackId2, MOJO_RESULT_OK);

  // Write to the other end of the pipe.
  WriteMessage(handle1, @"QUJDRA==");  // "ABCD" in base-64

  // `expected_script1` is also called, but GetLastJavaScriptCall() will store
  // only the last one.
  EXPECT_EQ(expected_script2, WaitForLastJavaScriptCallOnFrame(test_frame));

  // Read the pipe until MOJO_RESULT_SHOULD_WAIT is returned
  // (the usual watcher behavior).
  EXPECT_EQ([ReadMessage(handle0)[@"result"] unsignedIntValue], MOJO_RESULT_OK);
  EXPECT_EQ([ReadMessage(handle0)[@"result"] unsignedIntValue],
            MOJO_RESULT_SHOULD_WAIT);

  // Cancel the second watcher and write again.
  CancelWatch(handle0, watch_id2);
  WriteMessage(handle1, @"QUJDRA==");  // "ABCD" in base-64

  // Only the second watcher should be notified.
  EXPECT_EQ(expected_script1, WaitForLastJavaScriptCallOnFrame(test_frame));

  CloseHandle(handle0);
  CloseHandle(handle1);
}

INSTANTIATE_TEST_SUITE_P(FrameTypes,
                         MojoFacadeWatchFramesTest,
                         ::testing::Values(FrameType::kMainFrame,
                                           FrameType::kChildFrame));

}  // namespace web
