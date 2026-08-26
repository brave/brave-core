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

// Origin and host of the fixture's child frame, standing in for AI Chat's
// chrome-untrusted:// conversation-entries iframe.
constexpr char kChildHost[] = "test-child";
// The poll loop's own JavaScript. Every facade re-arms on its frame, and a
// retry can land at any point, so it is noise for tests that care about what
// MojoFacade delivers to a frame.
constexpr char kPollScript[] =
    "return await Mojo.internal.fetchNextMessageFromJS();";
constexpr char kChildOriginSpec[] = "chrome-untrusted://test-child";

}  // namespace

// Frame type enum for parameterized tests.
enum class FrameType { kMainFrame, kChildFrame };

// A test fixture for MojoFacade's per-host behaviour. Copies the helper
// methods from //ios/web/webui/mojo_facade_unittest.mm, adding a child frame
// alongside the main one and a facade bound to that frame's host, mirroring
// the second WebUIIOS Brave creates for a chrome-untrusted:// frame.
class MojoFacadeTest : public WebTest {
 protected:
  MojoFacadeTest() {
    auto web_frames_manager = std::make_unique<web::FakeWebFramesManager>();
    frames_manager_ = web_frames_manager.get();
    web_state_.SetWebFramesManager(std::move(web_frames_manager));
    facade_ = std::make_unique<MojoFacade>(&web_state_);

    auto main_frame = FakeWebFrame::CreateMainWebFrame();
    // ServesFrame() matches on the frame's security origin host, so the
    // child needs one for its facade to find it.
    auto child_frame =
        FakeWebFrame::CreateChildWebFrame(GURL(kChildOriginSpec));

    main_frame_ = main_frame.get();
    child_frame_ = child_frame.get();
    frames_manager_->AddWebFrame(std::move(main_frame));
    frames_manager_->AddWebFrame(std::move(child_frame));

    child_facade_ = std::make_unique<MojoFacade>(&web_state_, kChildHost);

    main_frame_->ClearJavaScriptCallHistory();
    child_frame_->ClearJavaScriptCallHistory();
  }

  FakeWebFrame* main_frame() { return main_frame_; }
  FakeWebFrame* child_frame() { return child_frame_; }
  // The facade serving the main frame, which also owns the sub-frame facades.
  MojoFacade* facade() { return facade_.get(); }
  MojoFacade* child_facade() { return child_facade_.get(); }
  FakeWebStateWithInterfaceBinder& web_state() { return web_state_; }
  FakeWebFramesManager* frames_manager() { return frames_manager_; }

  MojoFacade* FacadeForType(FrameType frame_type) {
    return frame_type == FrameType::kMainFrame ? facade() : child_facade();
  }

  FakeWebFrame* FrameForType(FrameType frame_type) {
    return frame_type == FrameType::kMainFrame ? main_frame() : child_frame();
  }

  std::string HandleMessage(MojoFacade* facade,
                            const base::DictValue& message) {
    base::test::TestFuture<int, std::string> future;
    facade->HandleMojoMessage(0, &message, future.GetCallback());
    return future.Get<1>();
  }

  // Creates a message pipe as `facade`'s frame would, with `handle0_id` and
  // `handle1_id` standing in for the ids that frame's own
  // Mojo.nextAvailableHandleId counter hands out.
  void CreateMessagePipeWithIds(MojoFacade* facade,
                                int handle0_id,
                                int handle1_id,
                                uint32_t* handle0,
                                uint32_t* handle1) {
    base::DictValue create;
    create.Set("name", "Mojo.createMessagePipe");
    base::DictValue args;
    args.Set("handle0Id", handle0_id);
    args.Set("handle1Id", handle1_id);
    create.Set("args", std::move(args));
    std::string response_as_string = HandleMessage(facade, create);

    ASSERT_FALSE(response_as_string.empty());
    NSDictionary* response_as_dict = GetObject(response_as_string);
    ASSERT_TRUE([response_as_dict isKindOfClass:[NSDictionary class]]);
    ASSERT_EQ(MOJO_RESULT_OK, [response_as_dict[@"result"] unsignedIntValue]);
    *handle0 = [response_as_dict[@"handle0"] unsignedIntValue];
    *handle1 = [response_as_dict[@"handle1"] unsignedIntValue];
  }

  void CreateMessagePipe(MojoFacade* facade,
                         uint32_t* handle0,
                         uint32_t* handle1) {
    int handle0_id = next_handle_id_++;
    int handle1_id = next_handle_id_++;
    CreateMessagePipeWithIds(facade, handle0_id, handle1_id, handle0, handle1);
  }

  void CloseHandle(MojoFacade* facade, uint32_t handle) {
    base::DictValue close;
    close.Set("name", "MojoHandle.close");
    base::DictValue args;
    args.Set("handle", static_cast<int>(handle));
    close.Set("args", std::move(args));
    EXPECT_TRUE(HandleMessage(facade, close).empty());
  }

  int WatchHandle(MojoFacade* facade, uint32_t handle, int callback_id) {
    base::DictValue watch;
    watch.Set("name", "MojoHandle.watch");
    base::DictValue args;
    args.Set("handle", static_cast<int>(handle));
    args.Set("signals", static_cast<int>(MOJO_HANDLE_SIGNAL_READABLE));
    args.Set("callbackId", callback_id);
    watch.Set("args", std::move(args));
    const std::string watch_id_as_string = HandleMessage(facade, watch);
    EXPECT_FALSE(watch_id_as_string.empty());
    int watch_id = 0;
    EXPECT_TRUE(base::StringToInt(watch_id_as_string, &watch_id));
    return watch_id;
  }

  void CancelWatch(MojoFacade* facade, int watch_id) {
    base::DictValue cancel_watch;
    cancel_watch.Set("name", "MojoWatcher.cancel");
    base::DictValue args;
    args.Set("watchId", watch_id);
    cancel_watch.Set("args", std::move(args));
    EXPECT_TRUE(HandleMessage(facade, cancel_watch).empty());
  }

  void WriteMessage(MojoFacade* facade,
                    uint32_t handle,
                    std::string_view buffer) {
    base::DictValue write;
    write.Set("name", "MojoHandle.writeMessage");
    base::DictValue args;
    args.Set("handle", static_cast<int>(handle));
    args.Set("handles", base::ListValue());
    args.Set("buffer", buffer);
    write.Set("args", std::move(args));
    const std::string result_as_string = HandleMessage(facade, write);
    EXPECT_FALSE(result_as_string.empty());
    unsigned result = 0u;
    EXPECT_TRUE(base::StringToUint(result_as_string, &result));
    EXPECT_EQ(MOJO_RESULT_OK, result);
  }

  // Last JavaScript call on `frame` that isn't the poll loop re-arming.
  std::string LastDeliveredJavaScriptCall(FakeWebFrame* frame) {
    const std::vector<std::u16string>& history =
        frame->GetJavaScriptCallHistory();
    for (auto it = history.rbegin(); it != history.rend(); ++it) {
      std::string call = base::UTF16ToUTF8(*it);
      if (call != kPollScript) {
        return call;
      }
    }
    return std::string();
  }

  std::string WaitForLastJavaScriptCallOnFrame(FakeWebFrame* frame) {
    EXPECT_TRUE(WaitUntilConditionOrTimeout(
        kWaitForJSCompletionTimeout, /*run_message_loop=*/true, ^bool {
          return !LastDeliveredJavaScriptCall(frame).empty();
        }));

    std::string last_js_call = LastDeliveredJavaScriptCall(frame);
    frame->ClearJavaScriptCallHistory();
    return last_js_call;
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
  std::unique_ptr<MojoFacade> child_facade_;
};

// Parameterized test fixture for frame-specific tests. Logic is copied from
// //ios/web/webui/mojo_facade_unittest.mm, run against both the main and
// child frames to verify each facade drives the frame it serves rather than
// always the main frame.
class MojoFacadeWatchFramesTest
    : public MojoFacadeTest,
      public ::testing::WithParamInterface<FrameType> {};

TEST_P(MojoFacadeWatchFramesTest, Watch) {
  MojoFacade* facade = FacadeForType(GetParam());
  FakeWebFrame* test_frame = FrameForType(GetParam());

  uint32_t handle0, handle1;
  CreateMessagePipe(facade, &handle0, &handle1);

  const int kCallbackId = 99;
  WatchHandle(facade, handle0, kCallbackId);

  WriteMessage(facade, handle1, "QUJDRA==");  // "ABCD" in base-64

  EXPECT_EQ(GetExpectedWatchCallbackScript(handle0, kCallbackId),
            WaitForLastJavaScriptCallOnFrame(test_frame));

  CloseHandle(facade, handle0);
  CloseHandle(facade, handle1);
}

TEST_P(MojoFacadeWatchFramesTest, WatcherRearming) {
  MojoFacade* facade = FacadeForType(GetParam());
  FakeWebFrame* test_frame = FrameForType(GetParam());

  uint32_t handle0, handle1;
  CreateMessagePipe(facade, &handle0, &handle1);

  const int kCallbackId = 99;
  WatchHandle(facade, handle0, kCallbackId);

  WriteMessage(facade, handle1, "QUJDRA==");  // "ABCD" in base-64

  EXPECT_EQ(GetExpectedWatchCallbackScript(handle0, kCallbackId),
            WaitForLastJavaScriptCallOnFrame(test_frame));

  WriteMessage(facade, handle1, "QUJDRA==");  // "ABCD" in base-64

  // Check the watcher was rearmed and still targets the same frame.
  EXPECT_EQ(GetExpectedWatchCallbackScript(handle0, kCallbackId),
            WaitForLastJavaScriptCallOnFrame(test_frame));

  CloseHandle(facade, handle0);
  CloseHandle(facade, handle1);
}

TEST_P(MojoFacadeWatchFramesTest, CancelWatch) {
  MojoFacade* facade = FacadeForType(GetParam());
  FakeWebFrame* test_frame = FrameForType(GetParam());

  uint32_t handle0, handle1;
  CreateMessagePipe(facade, &handle0, &handle1);

  const int kCallbackId1 = 99;
  const int kCallbackId2 = 101;
  WatchHandle(facade, handle0, kCallbackId1);
  const int watch_id2 = WatchHandle(facade, handle0, kCallbackId2);
  const auto expected_script2 = base::StringPrintf(
      "Mojo.internal.fetchNextMessageFromNative(%d, "
      "{\"result\":%d}); "
      "Mojo.internal.watchCallbacksHolder.callCallback(%d, %d);",
      handle0, MOJO_RESULT_SHOULD_WAIT, kCallbackId2, MOJO_RESULT_OK);

  WriteMessage(facade, handle1, "QUJDRA==");  // "ABCD" in base-64

  // `expected_script` for callback 1 also fires, but only the last call is
  // kept.
  EXPECT_EQ(expected_script2, WaitForLastJavaScriptCallOnFrame(test_frame));

  CancelWatch(facade, watch_id2);
  WriteMessage(facade, handle1, "QUJDRA==");  // "ABCD" in base-64

  // Only the first watcher should be notified now.
  EXPECT_EQ(GetExpectedWatchCallbackScript(handle0, kCallbackId1),
            WaitForLastJavaScriptCallOnFrame(test_frame));

  CloseHandle(facade, handle0);
  CloseHandle(facade, handle1);
}

INSTANTIATE_TEST_SUITE_P(FrameTypes,
                         MojoFacadeWatchFramesTest,
                         ::testing::Values(FrameType::kMainFrame,
                                           FrameType::kChildFrame));

// Tests that consuming a handle in one frame leaves the identically numbered
// handle of another frame alone. Every frame runs its own copy of
// ui/webui/resources/js/ios/mojo_api.js, whose Mojo.nextAvailableHandleId
// counter restarts at 1 per JS context, so AI Chat's WebUI page and the
// chrome-untrusted:// conversation iframe it embeds routinely mint the same
// ids. Sharing one pipe table across frames, the close below erased the
// entry the main frame was still using and the write that follows hit
// CHECK(pipe.is_valid()) in HandleMojoHandleWriteMessage.
TEST_F(MojoFacadeTest, HandleIdsAreIndependentPerFrame) {
  uint32_t main_handle0, main_handle1;
  CreateMessagePipeWithIds(facade(), 1, 2, &main_handle0, &main_handle1);

  uint32_t child_handle0, child_handle1;
  CreateMessagePipeWithIds(child_facade(), 1, 2, &child_handle0,
                           &child_handle1);

  // Both frames handed out the very same ids.
  ASSERT_EQ(main_handle0, child_handle0);
  ASSERT_EQ(main_handle1, child_handle1);

  CloseHandle(child_facade(), child_handle0);
  CloseHandle(child_facade(), child_handle1);

  // The main frame's pipe must still be intact and writable.
  WriteMessage(facade(), main_handle0, "QUJDRA==");  // "ABCD" in base-64

  CloseHandle(facade(), main_handle0);
  CloseHandle(facade(), main_handle1);
}

// Tests that a watch notification reads from the pipe of the frame that
// created the watch, rather than whichever frame last claimed that handle id.
TEST_F(MojoFacadeTest, WatchReadsTheOwningFramesPipe) {
  uint32_t main_handle0, main_handle1;
  CreateMessagePipeWithIds(facade(), 1, 2, &main_handle0, &main_handle1);

  uint32_t child_handle0, child_handle1;
  CreateMessagePipeWithIds(child_facade(), 1, 2, &child_handle0,
                           &child_handle1);

  const int kMainCallbackId = 99;
  const int kChildCallbackId = 101;
  WatchHandle(facade(), main_handle0, kMainCallbackId);
  WatchHandle(child_facade(), child_handle0, kChildCallbackId);

  // Give each frame's pipe a payload of its own.
  WriteMessage(facade(), main_handle1, "QUJDRA==");         // "ABCD"
  WriteMessage(child_facade(), child_handle1, "RUZHSA==");  // "EFGH"

  EXPECT_EQ(GetExpectedWatchCallbackScript(main_handle0, kMainCallbackId,
                                           "[65,66,67,68]"),
            WaitForLastJavaScriptCallOnFrame(main_frame()));
  EXPECT_EQ(GetExpectedWatchCallbackScript(child_handle0, kChildCallbackId,
                                           "[69,70,71,72]"),
            WaitForLastJavaScriptCallOnFrame(child_frame()));

  CloseHandle(facade(), main_handle0);
  CloseHandle(facade(), main_handle1);
  CloseHandle(child_facade(), child_handle0);
  CloseHandle(child_facade(), child_handle1);
}

// Tests that a sub-frame's watch doesn't outlive the frame that created it,
// as would happen if e.g. AI Chat's untrusted conversation-entries iframe is
// torn down while a watch is still pending.
TEST_F(MojoFacadeTest, WatchIsErasedWhenOwningFrameDisappears) {
  const std::string frame_id = child_frame()->GetFrameId();

  uint32_t handle0, handle1;
  CreateMessagePipe(child_facade(), &handle0, &handle1);

  const int kCallbackId = 99;
  WatchHandle(child_facade(), handle0, kCallbackId);

  // Signal the pipe so the watcher posts its notification, but don't let the
  // run loop deliver it yet.
  WriteMessage(child_facade(), handle1, "QUJDRA==");  // "ABCD" in base-64

  // `child_frame()` is dangling after this call.
  frames_manager()->RemoveWebFrame(frame_id);

  // The pending notification must be dropped along with the frame, rather
  // than landing on the main frame. A short wait is enough: the watcher had
  // already posted before the frame went away, so a notification that was
  // going to arrive would do so within a few run loop turns.
  EXPECT_FALSE(WaitUntilConditionOrTimeout(
      base::Milliseconds(500), /*run_message_loop=*/true, ^bool {
        return !LastDeliveredJavaScriptCall(main_frame()).empty();
      }));
}

// Tests that a facade bound to a host serves that host's frame and leaves
// the main frame to the facade that has no host, rather than both polling the
// main frame as they did when every WebUIIOS built a host-less facade.
TEST_F(MojoFacadeTest, EachFacadeServesItsOwnHostsFrame) {
  uint32_t main_handle0, main_handle1;
  CreateMessagePipeWithIds(facade(), 1, 2, &main_handle0, &main_handle1);

  const int kMainCallbackId = 99;
  const int kChildCallbackId = 101;
  WatchHandle(facade(), main_handle0, kMainCallbackId);

  uint32_t child_handle0, child_handle1;
  CreateMessagePipeWithIds(child_facade(), 1, 2, &child_handle0,
                           &child_handle1);
  WatchHandle(child_facade(), child_handle0, kChildCallbackId);

  // Each frame's JS counter starts at 1, so both facades hand out the same
  // ids. They must stay in separate tables.
  ASSERT_EQ(main_handle0, child_handle0);

  WriteMessage(facade(), main_handle1, "QUJDRA==");         // "ABCD"
  WriteMessage(child_facade(), child_handle1, "RUZHSA==");  // "EFGH"

  // Each notification is delivered to the frame whose host owns the pipe.
  EXPECT_EQ(GetExpectedWatchCallbackScript(main_handle0, kMainCallbackId,
                                           "[65,66,67,68]"),
            WaitForLastJavaScriptCallOnFrame(main_frame()));
  EXPECT_EQ(GetExpectedWatchCallbackScript(child_handle0, kChildCallbackId,
                                           "[69,70,71,72]"),
            WaitForLastJavaScriptCallOnFrame(child_frame()));

  CloseHandle(facade(), main_handle0);
  CloseHandle(facade(), main_handle1);
  CloseHandle(child_facade(), child_handle0);
  CloseHandle(child_facade(), child_handle1);
}

// Tests the chrome-untrusted:// per-origin interface allowlist gate on
// Mojo.bindInterface (InterfaceBinder::IsAllowedForOrigin, see
// brave/chromium_src/ios/web/public/web_state.h).
class MojoFacadeBindInterfaceOriginTest : public MojoFacadeTest {
 protected:
  // Adds a frame with the given origin and returns a facade bound to its
  // host, as the WebUIIOS created for that host would own.
  MojoFacade* AddFrameWithOrigin(const GURL& origin) {
    auto frame = FakeWebFrame::Create("originTestFrameId",
                                      /*is_main_frame=*/false, origin);
    frames_manager()->AddWebFrame(std::move(frame));
    origin_facade_ =
        std::make_unique<MojoFacade>(&web_state(), origin.GetHost());
    return origin_facade_.get();
  }

  // Attempts Mojo.bindInterface for `interface_name` from `facade`'s frame,
  // and reports whether the interface's registered callback actually ran.
  bool TryBindInterface(MojoFacade* facade, const std::string& interface_name) {
    uint32_t handle0, handle1;
    CreateMessagePipe(facade, &handle0, &handle1);

    base::DictValue connect;
    connect.Set("name", "Mojo.bindInterface");
    base::DictValue args;
    args.Set("interfaceName", interface_name);
    args.Set("requestHandle", static_cast<int>(handle0));
    connect.Set("args", std::move(args));

    HandleMessage(facade, connect);
    CloseHandle(facade, handle1);
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

  std::unique_ptr<MojoFacade> origin_facade_;
  bool interface_bound_ = false;
};

TEST_F(MojoFacadeBindInterfaceOriginTest, AllowedForRegisteredOrigin) {
  const GURL kAllowedOrigin("chrome-untrusted://allowed-host");
  RegisterUntrustedInterface(kAllowedOrigin);
  MojoFacade* facade = AddFrameWithOrigin(kAllowedOrigin);
  ASSERT_NE(nullptr, facade);

  EXPECT_TRUE(TryBindInterface(facade, web::mojom::TestUIHandlerMojo::Name_));
}

TEST_F(MojoFacadeBindInterfaceOriginTest, BlockedForUnregisteredOrigin) {
  const GURL kAllowedOrigin("chrome-untrusted://allowed-host");
  RegisterUntrustedInterface(kAllowedOrigin);
  MojoFacade* facade =
      AddFrameWithOrigin(GURL("chrome-untrusted://blocked-host"));
  ASSERT_NE(nullptr, facade);

  EXPECT_FALSE(TryBindInterface(facade, web::mojom::TestUIHandlerMojo::Name_));
}

TEST_F(MojoFacadeBindInterfaceOriginTest, NotGatedForTrustedOrigin) {
  // Ordinary (non-origin-scoped) interfaces aren't subject to the
  // chrome-untrusted:// allowlist; binding from a trusted chrome:// frame
  // should go through even without any AddUntrustedInterface registration.
  RegisterTrustedInterface();
  MojoFacade* facade = AddFrameWithOrigin(GURL("chrome://test-host"));
  ASSERT_NE(nullptr, facade);

  EXPECT_TRUE(TryBindInterface(facade, web::mojom::TestUIHandlerMojo::Name_));
}

}  // namespace web
