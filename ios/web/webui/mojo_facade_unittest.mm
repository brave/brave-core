// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ios/web/webui/mojo_facade.h"

#include <memory>

#include "base/functional/bind.h"
#include "base/memory/raw_ptr.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/ios/wait_util.h"
#include "base/test/test_future.h"
#include "base/values.h"
#include "ios/web/public/test/fakes/fake_web_frame.h"
#include "ios/web/public/test/fakes/fake_web_frames_manager.h"
#include "ios/web/public/test/fakes/fake_web_state.h"
#include "ios/web/public/test/web_test.h"
#include "ios/web/test/mojo_test.test-mojom.h"
#include "testing/gtest_mac.h"
#include "url/gurl.h"
#include "url/origin.h"

using base::test::ios::kWaitForJSCompletionTimeout;
using base::test::ios::WaitUntilConditionOrTimeout;

namespace web {

namespace {

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

// Frame type enum for parameterized tests.
enum class FrameType { kMainFrame, kChildFrame };

// A test fixture for MojoFacade's sub-frame routing. Copies the helper
// methods from //ios/web/webui/mojo_facade_unittest.mm, adding a child frame
// alongside the main one and a `frameId` argument to WatchHandle, so the
// same request flow can be exercised for either frame.
class MojoFacadeTest : public WebTest {
 protected:
  MojoFacadeTest() {
    auto web_frames_manager = std::make_unique<web::FakeWebFramesManager>();
    frames_manager_ = web_frames_manager.get();
    web_state_.SetWebFramesManager(std::move(web_frames_manager));
    facade_ = std::make_unique<MojoFacade>(&web_state_);

    auto main_frame = FakeWebFrame::CreateMainWebFrame();
    auto child_frame = FakeWebFrame::CreateChildWebFrame();

    main_frame_ = main_frame.get();
    child_frame_ = child_frame.get();
    frames_manager_->AddWebFrame(std::move(main_frame));
    frames_manager_->AddWebFrame(std::move(child_frame));
    main_frame_->ClearJavaScriptCallHistory();
    child_frame_->ClearJavaScriptCallHistory();
  }

  FakeWebFrame* main_frame() { return main_frame_; }
  FakeWebFrame* child_frame() { return child_frame_; }
  MojoFacade* facade() { return facade_.get(); }
  FakeWebStateWithInterfaceBinder& web_state() { return web_state_; }
  FakeWebFramesManager* frames_manager() { return frames_manager_; }

  std::string HandleMessage(const base::DictValue& message) {
    base::test::TestFuture<int, std::string> future;
    facade()->HandleMojoMessage(0, &message, future.GetCallback());
    return future.Get<1>();
  }

  // Tags `args` the way Brave's mojo_api.js plaster does. An empty
  // `frame_id` stands for JS without that plaster, which MojoFacade treats as
  // the main frame.
  static void SetFrameId(base::DictValue* args, const std::string& frame_id) {
    if (!frame_id.empty()) {
      args->Set("frameId", frame_id);
    }
  }

  // Creates a message pipe as `frame_id`'s JS would, with `handle0_id` and
  // `handle1_id` standing in for the ids that frame's own
  // Mojo.nextAvailableHandleId counter hands out.
  void CreateMessagePipeWithIds(int handle0_id,
                                int handle1_id,
                                const std::string& frame_id,
                                uint32_t* handle0,
                                uint32_t* handle1) {
    base::DictValue create;
    create.Set("name", "Mojo.createMessagePipe");
    base::DictValue args;
    args.Set("handle0Id", handle0_id);
    args.Set("handle1Id", handle1_id);
    SetFrameId(&args, frame_id);
    create.Set("args", std::move(args));
    std::string response_as_string = HandleMessage(create);

    ASSERT_FALSE(response_as_string.empty());
    NSDictionary* response_as_dict = GetObject(response_as_string);
    ASSERT_TRUE([response_as_dict isKindOfClass:[NSDictionary class]]);
    ASSERT_EQ(MOJO_RESULT_OK, [response_as_dict[@"result"] unsignedIntValue]);
    *handle0 = [response_as_dict[@"handle0"] unsignedIntValue];
    *handle1 = [response_as_dict[@"handle1"] unsignedIntValue];
  }

  void CreateMessagePipe(uint32_t* handle0,
                         uint32_t* handle1,
                         const std::string& frame_id = std::string()) {
    int handle0_id = next_handle_id_++;
    int handle1_id = next_handle_id_++;
    CreateMessagePipeWithIds(handle0_id, handle1_id, frame_id, handle0,
                             handle1);
  }

  void CloseHandle(uint32_t handle,
                   const std::string& frame_id = std::string()) {
    base::DictValue close;
    close.Set("name", "MojoHandle.close");
    base::DictValue args;
    args.Set("handle", static_cast<int>(handle));
    SetFrameId(&args, frame_id);
    close.Set("args", std::move(args));
    std::string result = HandleMessage(close);
    EXPECT_TRUE(result.empty());
  }

  // `frame_id` is optional, matching how Brave's mojo_api.js plaster only
  // adds it once __gCrWeb.frameId is available.
  int WatchHandle(uint32_t handle,
                  int callback_id,
                  const std::string& frame_id = std::string()) {
    base::DictValue watch;
    watch.Set("name", "MojoHandle.watch");
    base::DictValue args;
    args.Set("handle", static_cast<int>(handle));
    args.Set("signals", static_cast<int>(MOJO_HANDLE_SIGNAL_READABLE));
    args.Set("callbackId", callback_id);
    SetFrameId(&args, frame_id);
    watch.Set("args", std::move(args));
    const std::string watch_id_as_string = HandleMessage(watch);
    EXPECT_FALSE(watch_id_as_string.empty());
    int watch_id = 0;
    EXPECT_TRUE(base::StringToInt(watch_id_as_string, &watch_id));
    return watch_id;
  }

  void CancelWatch(uint32_t handle, int watch_id) {
    base::DictValue cancel_watch;
    cancel_watch.Set("name", "MojoWatcher.cancel");
    base::DictValue args;
    args.Set("watchId", watch_id);
    cancel_watch.Set("args", std::move(args));
    EXPECT_TRUE(HandleMessage(cancel_watch).empty());
  }

  void WriteMessage(uint32_t handle,
                    std::string_view buffer,
                    const std::string& frame_id = std::string()) {
    base::DictValue write;
    write.Set("name", "MojoHandle.writeMessage");
    base::DictValue args;
    args.Set("handle", static_cast<int>(handle));
    args.Set("handles", base::ListValue());
    args.Set("buffer", buffer);
    SetFrameId(&args, frame_id);
    write.Set("args", std::move(args));
    const std::string result_as_string = HandleMessage(write);
    EXPECT_FALSE(result_as_string.empty());
    unsigned result = 0u;
    EXPECT_TRUE(base::StringToUint(result_as_string, &result));
    EXPECT_EQ(MOJO_RESULT_OK, result);
  }

  std::string WaitForLastJavaScriptCallOnFrame(FakeWebFrame* frame) {
    EXPECT_TRUE(WaitUntilConditionOrTimeout(
        kWaitForJSCompletionTimeout, /*run_message_loop=*/true, ^bool {
          return !frame->GetLastJavaScriptCall().empty();
        }));

    const auto last_js_call = frame->GetLastJavaScriptCall();
    frame->ClearJavaScriptCallHistory();
    return base::UTF16ToUTF8(last_js_call);
  }

  // `buffer` is the JSON array of bytes the watched pipe is expected to
  // yield, defaulting to "ABCD".
  std::string GetExpectedWatchCallbackScript(
      uint32_t handle,
      int callback_id,
      std::string_view buffer = "[65,66,67,68]") {
    return base::StringPrintf(
        "Mojo.internal.fetchNextMessageFromNative(%d, "
        "{\"buffer\":%s,\"handles\":[],\"result\":0}); "
        "Mojo.internal.watchCallbacksHolder.callCallback(%d, %d);",
        handle, buffer, callback_id, MOJO_RESULT_OK);
  }

 private:
  int next_handle_id_ = 1;
  FakeWebStateWithInterfaceBinder web_state_;
  raw_ptr<web::FakeWebFramesManager> frames_manager_;
  raw_ptr<FakeWebFrame> main_frame_;
  raw_ptr<FakeWebFrame> child_frame_;
  std::unique_ptr<MojoFacade> facade_;
};

// Ensures that when an invalid frame id is passed into the watcher, it still
// executes the callback on the main frame by default.
TEST_F(MojoFacadeTest, WatchWithInvalidFrameId) {
  uint32_t handle0, handle1;
  CreateMessagePipe(&handle0, &handle1);

  const int kCallbackId = 99;
  WatchHandle(handle0, kCallbackId, "invalid");

  WriteMessage(handle1, "QUJDRA==");  // "ABCD" in base-64

  EXPECT_EQ(GetExpectedWatchCallbackScript(handle0, kCallbackId),
            WaitForLastJavaScriptCallOnFrame(main_frame()));

  CloseHandle(handle0);
  CloseHandle(handle1);
}

// Parameterized test fixture for frame-specific tests. Logic is copied from
// //ios/web/webui/mojo_facade_unittest.mm, run against both the main and
// child frames to verify watch notifications are routed to whichever frame
// created the watch, not always the main frame.
class MojoFacadeWatchFramesTest
    : public MojoFacadeTest,
      public ::testing::WithParamInterface<FrameType> {
 protected:
  FakeWebFrame* GetFrameForType(FrameType frame_type) {
    return frame_type == FrameType::kMainFrame ? main_frame() : child_frame();
  }
};

TEST_P(MojoFacadeWatchFramesTest, Watch) {
  FakeWebFrame* test_frame = GetFrameForType(GetParam());
  const std::string frame_id = test_frame->GetFrameId();

  uint32_t handle0, handle1;
  CreateMessagePipe(&handle0, &handle1, frame_id);

  const int kCallbackId = 99;
  WatchHandle(handle0, kCallbackId, frame_id);

  WriteMessage(handle1, "QUJDRA==", frame_id);  // "ABCD" in base-64

  EXPECT_EQ(GetExpectedWatchCallbackScript(handle0, kCallbackId),
            WaitForLastJavaScriptCallOnFrame(test_frame));

  CloseHandle(handle0, frame_id);
  CloseHandle(handle1, frame_id);
}

TEST_P(MojoFacadeWatchFramesTest, WatcherRearming) {
  FakeWebFrame* test_frame = GetFrameForType(GetParam());
  const std::string frame_id = test_frame->GetFrameId();

  uint32_t handle0, handle1;
  CreateMessagePipe(&handle0, &handle1, frame_id);

  const int kCallbackId = 99;
  WatchHandle(handle0, kCallbackId, frame_id);

  WriteMessage(handle1, "QUJDRA==", frame_id);  // "ABCD" in base-64

  EXPECT_EQ(GetExpectedWatchCallbackScript(handle0, kCallbackId),
            WaitForLastJavaScriptCallOnFrame(test_frame));

  WriteMessage(handle1, "QUJDRA==", frame_id);  // "ABCD" in base-64

  // Check the watcher was rearmed and still targets the same frame.
  EXPECT_EQ(GetExpectedWatchCallbackScript(handle0, kCallbackId),
            WaitForLastJavaScriptCallOnFrame(test_frame));

  CloseHandle(handle0, frame_id);
  CloseHandle(handle1, frame_id);
}

TEST_P(MojoFacadeWatchFramesTest, CancelWatch) {
  FakeWebFrame* test_frame = GetFrameForType(GetParam());
  const std::string frame_id = test_frame->GetFrameId();

  uint32_t handle0, handle1;
  CreateMessagePipe(&handle0, &handle1, frame_id);

  const int kCallbackId1 = 99;
  const int kCallbackId2 = 101;
  WatchHandle(handle0, kCallbackId1, frame_id);
  const int watch_id2 = WatchHandle(handle0, kCallbackId2, frame_id);
  const auto expected_script2 = base::StringPrintf(
      "Mojo.internal.fetchNextMessageFromNative(%d, "
      "{\"result\":%d}); "
      "Mojo.internal.watchCallbacksHolder.callCallback(%d, %d);",
      handle0, MOJO_RESULT_SHOULD_WAIT, kCallbackId2, MOJO_RESULT_OK);

  WriteMessage(handle1, "QUJDRA==", frame_id);  // "ABCD" in base-64

  // `expected_script` for callback 1 also fires, but only the last call is
  // kept.
  EXPECT_EQ(expected_script2, WaitForLastJavaScriptCallOnFrame(test_frame));

  CancelWatch(handle0, watch_id2);
  WriteMessage(handle1, "QUJDRA==", frame_id);  // "ABCD" in base-64

  // Only the first watcher should be notified now.
  EXPECT_EQ(GetExpectedWatchCallbackScript(handle0, kCallbackId1),
            WaitForLastJavaScriptCallOnFrame(test_frame));

  CloseHandle(handle0, frame_id);
  CloseHandle(handle1, frame_id);
}

INSTANTIATE_TEST_SUITE_P(FrameTypes,
                         MojoFacadeWatchFramesTest,
                         ::testing::Values(FrameType::kMainFrame,
                                           FrameType::kChildFrame));

// Tests that a sub-frame's watch doesn't outlive the frame that created it.
// Without cleanup, a stale watch would later misdirect its notification to
// the main frame (via GetFrameForWatch's "unknown frame" fallback) instead
// of being dropped, since the frame's own removal never canceled it.
TEST_F(MojoFacadeTest, WatchIsErasedWhenOwningFrameDisappears) {
  const std::string frame_id = child_frame()->GetFrameId();

  uint32_t handle0, handle1;
  CreateMessagePipe(&handle0, &handle1, frame_id);

  const int kCallbackId = 99;
  WatchHandle(handle0, kCallbackId, frame_id);

  // Signal the pipe so the watcher posts its notification, but don't let the
  // run loop deliver it yet.
  WriteMessage(handle1, "QUJDRA==", frame_id);  // "ABCD" in base-64

  // Remove the frame without canceling its watch first, as would happen if
  // e.g. AI Chat's untrusted conversation-entries iframe is torn down while
  // a watch is still pending. `child_frame()` is dangling after this call.
  frames_manager()->RemoveWebFrame(frame_id);

  // The pending notification must be dropped with the frame. If the watch had
  // outlived it, GetFrameForWatch's "unknown frame" fallback would misdirect
  // the callback to the main frame.
  EXPECT_FALSE(WaitUntilConditionOrTimeout(
      kWaitForJSCompletionTimeout, /*run_message_loop=*/true, ^bool {
        return !main_frame()->GetLastJavaScriptCall().empty();
      }));
}

// Tests that consuming a handle in one frame leaves the identically numbered
// handle of another frame alone. Every frame runs its own copy of
// ui/webui/resources/js/ios/mojo_api.js, whose Mojo.nextAvailableHandleId
// counter restarts at 1 per JS context, so AI Chat's WebUI page and the
// chrome-untrusted:// conversation iframe it embeds routinely mint the same
// ids. Keyed by id alone, the close below erased the single shared entry and
// the write that follows hit CHECK(pipe.is_valid()) in
// HandleMojoHandleWriteMessage.
TEST_F(MojoFacadeTest, ClosingAHandleLeavesTheSameIdInAnotherFrameAlone) {
  const std::string main_frame_id = main_frame()->GetFrameId();
  const std::string child_frame_id = child_frame()->GetFrameId();

  uint32_t main_handle0, main_handle1;
  CreateMessagePipeWithIds(1, 2, main_frame_id, &main_handle0, &main_handle1);

  uint32_t child_handle0, child_handle1;
  CreateMessagePipeWithIds(1, 2, child_frame_id, &child_handle0,
                           &child_handle1);

  // Both frames handed out the very same ids.
  ASSERT_EQ(main_handle0, child_handle0);
  ASSERT_EQ(main_handle1, child_handle1);

  CloseHandle(child_handle0, child_frame_id);
  CloseHandle(child_handle1, child_frame_id);

  // The main frame's pipe must still be intact and writable.
  WriteMessage(main_handle0, "QUJDRA==", main_frame_id);  // "ABCD" in base-64

  CloseHandle(main_handle0, main_frame_id);
  CloseHandle(main_handle1, main_frame_id);
}

// Tests that a watch notification reads from the pipe of the frame that
// created the watch, rather than whichever frame last claimed that handle id.
TEST_F(MojoFacadeTest, WatchReadsTheOwningFramesPipe) {
  const std::string main_frame_id = main_frame()->GetFrameId();
  const std::string child_frame_id = child_frame()->GetFrameId();

  uint32_t main_handle0, main_handle1;
  CreateMessagePipeWithIds(1, 2, main_frame_id, &main_handle0, &main_handle1);

  uint32_t child_handle0, child_handle1;
  CreateMessagePipeWithIds(1, 2, child_frame_id, &child_handle0,
                           &child_handle1);

  const int kMainCallbackId = 99;
  const int kChildCallbackId = 101;
  WatchHandle(main_handle0, kMainCallbackId, main_frame_id);
  WatchHandle(child_handle0, kChildCallbackId, child_frame_id);

  // Give each frame's pipe a payload of its own.
  WriteMessage(main_handle1, "QUJDRA==", main_frame_id);    // "ABCD"
  WriteMessage(child_handle1, "RUZHSA==", child_frame_id);  // "EFGH"

  EXPECT_EQ(GetExpectedWatchCallbackScript(main_handle0, kMainCallbackId,
                                           "[65,66,67,68]"),
            WaitForLastJavaScriptCallOnFrame(main_frame()));
  EXPECT_EQ(GetExpectedWatchCallbackScript(child_handle0, kChildCallbackId,
                                           "[69,70,71,72]"),
            WaitForLastJavaScriptCallOnFrame(child_frame()));

  CloseHandle(main_handle0, main_frame_id);
  CloseHandle(main_handle1, main_frame_id);
  CloseHandle(child_handle0, child_frame_id);
  CloseHandle(child_handle1, child_frame_id);
}

// Tests the chrome-untrusted:// per-origin interface allowlist gate on
// Mojo.bindInterface (InterfaceBinder::IsAllowedForOrigin, see
// brave/chromium_src/ios/web/public/web_state.h).
class MojoFacadeBindInterfaceOriginTest : public MojoFacadeTest {
 protected:
  // Adds a frame with the given origin and returns it. Not the fixture's
  // main/child frame, since those have no origin set.
  FakeWebFrame* AddFrameWithOrigin(const GURL& origin) {
    auto frame = FakeWebFrame::Create("originTestFrameId",
                                      /*is_main_frame=*/false, origin);
    FakeWebFrame* frame_ptr = frame.get();
    frames_manager()->AddWebFrame(std::move(frame));
    return frame_ptr;
  }

  // Attempts Mojo.bindInterface for `interface_name` from `frame`, and
  // reports whether the interface's registered callback actually ran.
  bool TryBindInterface(FakeWebFrame* frame,
                        const std::string& interface_name) {
    // The pipe has to be created by the same frame that binds it, since a
    // frame can only reach its own handles.
    const std::string frame_id = frame->GetFrameId();
    uint32_t handle0, handle1;
    CreateMessagePipe(&handle0, &handle1, frame_id);

    base::DictValue connect;
    connect.Set("name", "Mojo.bindInterface");
    base::DictValue args;
    args.Set("interfaceName", interface_name);
    args.Set("requestHandle", static_cast<int>(handle0));
    args.Set("frameId", frame_id);
    connect.Set("args", std::move(args));

    HandleMessage(connect);
    CloseHandle(handle1, frame_id);
    return interface_bound_;
  }

  void RegisterUntrustedInterface(const GURL& origin) {
    web_state()
        .GetInterfaceBinderForMainFrame()
        ->AddUntrustedInterface<web::mojom::TestUIHandlerMojo>(
            origin,
            base::BindRepeating(
                [](bool* bound_flag,
                   mojo::PendingReceiver<web::mojom::TestUIHandlerMojo>) {
                  *bound_flag = true;
                },
                &interface_bound_));
  }

  // Registers the interface via the ordinary (non-origin-scoped) path,
  // which isn't subject to the chrome-untrusted:// allowlist at all.
  void RegisterTrustedInterface() {
    web_state().GetInterfaceBinderForMainFrame()->AddInterface(
        web::mojom::TestUIHandlerMojo::Name_,
        base::BindRepeating(
            [](bool* bound_flag, mojo::GenericPendingReceiver*) {
              *bound_flag = true;
            },
            &interface_bound_));
  }

  bool interface_bound_ = false;
};

TEST_F(MojoFacadeBindInterfaceOriginTest, AllowedForRegisteredOrigin) {
  const GURL kAllowedOrigin("chrome-untrusted://allowed-host");
  RegisterUntrustedInterface(kAllowedOrigin);
  FakeWebFrame* frame = AddFrameWithOrigin(kAllowedOrigin);

  EXPECT_TRUE(TryBindInterface(frame, web::mojom::TestUIHandlerMojo::Name_));
}

TEST_F(MojoFacadeBindInterfaceOriginTest, BlockedForUnregisteredOrigin) {
  const GURL kAllowedOrigin("chrome-untrusted://allowed-host");
  RegisterUntrustedInterface(kAllowedOrigin);
  FakeWebFrame* frame =
      AddFrameWithOrigin(GURL("chrome-untrusted://blocked-host"));

  EXPECT_FALSE(TryBindInterface(frame, web::mojom::TestUIHandlerMojo::Name_));
}

TEST_F(MojoFacadeBindInterfaceOriginTest, NotGatedForTrustedOrigin) {
  // Ordinary (non-origin-scoped) interfaces aren't subject to the
  // chrome-untrusted:// allowlist; binding from a trusted chrome:// frame
  // should go through even without any AddUntrustedInterface registration.
  RegisterTrustedInterface();
  FakeWebFrame* frame = AddFrameWithOrigin(GURL("chrome://test-host"));

  EXPECT_TRUE(TryBindInterface(frame, web::mojom::TestUIHandlerMojo::Name_));
}

}  // namespace web
