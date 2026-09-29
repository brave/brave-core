// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/mojo/brave_mojo_system.h"

#include <cstdint>
#include <utility>
#include <vector>

#include "base/apple/foundation_util.h"
#include "base/containers/span.h"
#include "base/functional/callback.h"
#include "base/numerics/byte_conversions.h"
#include "base/run_loop.h"
#include "base/test/run_until.h"
#include "brave/ios/browser/mojo/test/brave_mojo_system.test-mojom.h"
#include "ios/web/public/test/web_task_environment.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "mojo/public/cpp/system/handle.h"
#include "mojo/public/cpp/system/handle_signals_state.h"
#include "mojo/public/cpp/system/message_pipe.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

namespace {

struct ReadResult {
  BraveMojoResult result = BraveMojoResultUnknown;
  std::vector<uint8_t> bytes;
  std::vector<BraveMojoHandle> handles;
};

ReadResult ReadMessage(BraveMojoHandle pipe) {
  ReadResult read;
  ReadResult* read_ptr = &read;
  read.result =
      BraveMojoReadMessage(pipe, ^(NSData* bytes, NSArray<NSNumber*>* handles) {
        base::span<const uint8_t> bytes_span = base::apple::NSDataToSpan(bytes);
        read_ptr->bytes.assign(bytes_span.begin(), bytes_span.end());
        for (NSNumber* handle in handles) {
          read_ptr->handles.push_back(handle.unsignedLongValue);
        }
      });
  return read;
}

// Records the notifications a watcher receives. Blocks capture a pointer to
// it, it must outlive the watcher.
struct WatchRecorder {
  std::vector<BraveMojoResult> results;
  base::OnceClosure on_notified;
};

// Watches |pipe|, recording notifications to |recorder| and cancelling
// |*cancel_on_notify| (if set) from within the callback. |*weak_token| is set
// to an object only the callback retains, so it becomes nil once the callback
// has been released.
//
// The callback is created here rather than by the test, as a block literal's
// captures are retained until the end of the scope declaring it, even after
// the block has been copied.
BraveMojoWatcherId WatchWithToken(BraveMojoHandle pipe,
                                  WatchRecorder* recorder,
                                  BraveMojoWatcherId* cancel_on_notify,
                                  __weak NSObject** weak_token) {
  NSObject* token = [[NSObject alloc] init];
  *weak_token = token;
  return BraveMojoWatch(pipe, ^(BraveMojoResult result) {
    [token self];
    recorder->results.push_back(result);
    if (cancel_on_notify) {
      BraveMojoCancelWatch(*cancel_on_notify);
    }
  });
}

BraveMojoResult WriteMessage(BraveMojoHandle pipe,
                             base::span<const uint8_t> bytes,
                             base::span<const BraveMojoHandle> handles = {}) {
  NSMutableArray<NSNumber*>* handle_numbers = [NSMutableArray array];
  for (BraveMojoHandle handle : handles) {
    [handle_numbers addObject:@(handle)];
  }
  return BraveMojoWriteMessage(
      pipe, [NSData dataWithBytes:bytes.data() length:bytes.size()],
      handle_numbers);
}

bool IsPeerClosed(BraveMojoHandle handle) {
  return mojo::Handle(handle).QuerySignalsState().peer_closed();
}

}  // namespace

class BraveMojoSystemTest : public PlatformTest {
 protected:
  void SetUp() override {
    PlatformTest::SetUp();
    ASSERT_EQ(BraveMojoCreateMessagePipe(&handle0_, &handle1_),
              BraveMojoResultOk);
  }

  void TearDown() override {
    BraveMojoClose(handle0_);
    BraveMojoClose(handle1_);
    PlatformTest::TearDown();
  }

  // Takes ownership of one of the fixture's pipe ends.
  BraveMojoHandle TakeHandle0() { return std::exchange(handle0_, 0); }
  BraveMojoHandle TakeHandle1() { return std::exchange(handle1_, 0); }

  web::WebTaskEnvironment task_environment_;
  BraveMojoHandle handle0_ = 0;
  BraveMojoHandle handle1_ = 0;
};

TEST_F(BraveMojoSystemTest, IsInitialized) {
  EXPECT_TRUE(BraveMojoIsInitialized());
}

TEST_F(BraveMojoSystemTest, CreateMessagePipe) {
  EXPECT_NE(handle0_, 0u);
  EXPECT_NE(handle1_, 0u);
  EXPECT_NE(handle0_, handle1_);
}

TEST_F(BraveMojoSystemTest, CloseInvalidHandleIsNoOp) {
  BraveMojoClose(0);
}

TEST_F(BraveMojoSystemTest, CloseSignalsPeerClosed) {
  EXPECT_FALSE(IsPeerClosed(handle1_));
  BraveMojoClose(TakeHandle0());
  EXPECT_TRUE(IsPeerClosed(handle1_));
}

TEST_F(BraveMojoSystemTest, WriteReadRoundTrip) {
  const uint8_t kPayload[] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
  ASSERT_EQ(WriteMessage(handle0_, kPayload), BraveMojoResultOk);

  ReadResult read = ReadMessage(handle1_);
  EXPECT_EQ(read.result, BraveMojoResultOk);
  EXPECT_EQ(read.bytes,
            std::vector<uint8_t>(std::begin(kPayload), std::end(kPayload)));
  EXPECT_TRUE(read.handles.empty());
}

TEST_F(BraveMojoSystemTest, MessagesAreReadInOrder) {
  for (uint8_t i = 0; i < 3; ++i) {
    const uint8_t payload[] = {i};
    ASSERT_EQ(WriteMessage(handle0_, payload), BraveMojoResultOk);
  }
  for (uint8_t i = 0; i < 3; ++i) {
    ReadResult read = ReadMessage(handle1_);
    ASSERT_EQ(read.result, BraveMojoResultOk);
    EXPECT_EQ(read.bytes, std::vector<uint8_t>{i});
  }
}

TEST_F(BraveMojoSystemTest, ReadEmptyPipeShouldWait) {
  __block bool consumed = false;
  EXPECT_EQ(BraveMojoReadMessage(handle1_,
                                 ^(NSData*, NSArray<NSNumber*>*) {
                                   consumed = true;
                                 }),
            BraveMojoResultShouldWait);
  EXPECT_FALSE(consumed);
}

TEST_F(BraveMojoSystemTest, ReadAfterPeerClosedDrainsThenFails) {
  const uint8_t kPayload[] = {42};
  ASSERT_EQ(WriteMessage(handle0_, kPayload), BraveMojoResultOk);
  BraveMojoClose(TakeHandle0());

  // Messages written before the peer was closed are still delivered.
  EXPECT_EQ(ReadMessage(handle1_).result, BraveMojoResultOk);
  EXPECT_EQ(ReadMessage(handle1_).result, BraveMojoResultFailedPrecondition);
}

TEST_F(BraveMojoSystemTest, WriteToClosedPeerFails) {
  BraveMojoClose(TakeHandle1());
  const uint8_t kPayload[] = {1};
  EXPECT_EQ(WriteMessage(handle0_, kPayload),
            BraveMojoResultFailedPrecondition);
}

TEST_F(BraveMojoSystemTest, TransfersHandles) {
  BraveMojoHandle other0 = 0;
  BraveMojoHandle other1 = 0;
  ASSERT_EQ(BraveMojoCreateMessagePipe(&other0, &other1), BraveMojoResultOk);

  const uint8_t kPayload[] = {7};
  const BraveMojoHandle kHandles[] = {other1};
  ASSERT_EQ(WriteMessage(handle0_, kPayload, kHandles), BraveMojoResultOk);

  ReadResult read = ReadMessage(handle1_);
  ASSERT_EQ(read.result, BraveMojoResultOk);
  ASSERT_EQ(read.handles.size(), 1u);
  BraveMojoHandle received = read.handles[0];
  EXPECT_NE(received, 0u);

  // The received handle is the other end of |other0|.
  const uint8_t kInnerPayload[] = {8, 9};
  ASSERT_EQ(WriteMessage(other0, kInnerPayload), BraveMojoResultOk);
  ReadResult inner = ReadMessage(received);
  EXPECT_EQ(inner.result, BraveMojoResultOk);
  EXPECT_EQ(inner.bytes, std::vector<uint8_t>({8, 9}));

  BraveMojoClose(received);
  EXPECT_TRUE(base::test::RunUntil([&] { return IsPeerClosed(other0); }));
  BraveMojoClose(other0);
}

TEST_F(BraveMojoSystemTest, FailedWriteClosesHandles) {
  BraveMojoHandle other0 = 0;
  BraveMojoHandle other1 = 0;
  ASSERT_EQ(BraveMojoCreateMessagePipe(&other0, &other1), BraveMojoResultOk);

  BraveMojoClose(TakeHandle1());
  const BraveMojoHandle kHandles[] = {other1};
  EXPECT_EQ(WriteMessage(handle0_, {}, kHandles),
            BraveMojoResultFailedPrecondition);
  EXPECT_TRUE(IsPeerClosed(other0));
  BraveMojoClose(other0);
}

TEST_F(BraveMojoSystemTest, WriteToInvalidPipeClosesHandles) {
  const BraveMojoHandle kHandles[] = {TakeHandle1()};
  EXPECT_NE(WriteMessage(0, {}, kHandles), BraveMojoResultOk);
  EXPECT_TRUE(IsPeerClosed(handle0_));
}

TEST_F(BraveMojoSystemTest, WriteInvalidHandleClosesOtherHandles) {
  BraveMojoHandle other0 = 0;
  BraveMojoHandle other1 = 0;
  ASSERT_EQ(BraveMojoCreateMessagePipe(&other0, &other1), BraveMojoResultOk);

  const BraveMojoHandle kHandles[] = {other1, 0};
  EXPECT_EQ(WriteMessage(handle0_, {}, kHandles),
            BraveMojoResultInvalidArgument);
  EXPECT_TRUE(IsPeerClosed(other0));
  EXPECT_EQ(ReadMessage(handle1_).result, BraveMojoResultShouldWait);
  BraveMojoClose(other0);
}

TEST_F(BraveMojoSystemTest, ReadsLazilySerializedMessageFromCppRemote) {
  mojo::Remote<brave_mojo_system_test::mojom::Pinger> remote(
      mojo::PendingRemote<brave_mojo_system_test::mojom::Pinger>(
          mojo::ScopedMessagePipeHandle(mojo::MessagePipeHandle(TakeHandle0())),
          0));

  mojo::PendingRemote<brave_mojo_system_test::mojom::Pinger> inner_remote;
  mojo::PendingReceiver<brave_mojo_system_test::mojom::Pinger> inner_receiver =
      inner_remote.InitWithNewPipeAndPassReceiver();

  remote->Ping("hello");
  remote->PingWithRemote(std::move(inner_remote));

  // The first uint32 of every mojo message is the size of its header.
  ReadResult ping = ReadMessage(handle1_);
  ASSERT_EQ(ping.result, BraveMojoResultOk);
  ASSERT_GE(ping.bytes.size(), sizeof(uint32_t));
  uint32_t header_size =
      base::U32FromLittleEndian(base::span(ping.bytes).first<4>());
  EXPECT_GE(header_size, 24u);
  EXPECT_LT(header_size, ping.bytes.size());
  EXPECT_TRUE(ping.handles.empty());

  ReadResult ping_with_remote = ReadMessage(handle1_);
  ASSERT_EQ(ping_with_remote.result, BraveMojoResultOk);
  ASSERT_EQ(ping_with_remote.handles.size(), 1u);

  mojo::ScopedMessagePipeHandle inner_pipe = inner_receiver.PassPipe();
  EXPECT_FALSE(inner_pipe->QuerySignalsState().peer_closed());
  BraveMojoClose(ping_with_remote.handles[0]);
  EXPECT_TRUE(base::test::RunUntil(
      [&] { return inner_pipe->QuerySignalsState().peer_closed(); }));
}

TEST_F(BraveMojoSystemTest, WatchNotifiesReadable) {
  base::RunLoop run_loop;
  BraveMojoHandle pipe = handle1_;
  std::vector<uint8_t> received;
  std::vector<uint8_t>* received_ptr = &received;
  WatchRecorder recorder{.on_notified = run_loop.QuitClosure()};
  WatchRecorder* recorder_ptr = &recorder;
  BraveMojoWatcherId watcher = BraveMojoWatch(pipe, ^(BraveMojoResult result) {
    recorder_ptr->results.push_back(result);
    *received_ptr = ReadMessage(pipe).bytes;
    if (recorder_ptr->on_notified) {
      std::move(recorder_ptr->on_notified).Run();
    }
  });
  ASSERT_NE(watcher, 0u);

  const uint8_t kPayload[] = {3, 1, 4};
  ASSERT_EQ(WriteMessage(handle0_, kPayload), BraveMojoResultOk);
  run_loop.Run();

  ASSERT_EQ(recorder.results.size(), 1u);
  EXPECT_EQ(recorder.results[0], BraveMojoResultOk);
  EXPECT_EQ(received, std::vector<uint8_t>({3, 1, 4}));
  BraveMojoCancelWatch(watcher);
}

TEST_F(BraveMojoSystemTest, WatchRearmsAfterNotification) {
  BraveMojoHandle pipe = handle1_;
  WatchRecorder recorder;
  WatchRecorder* recorder_ptr = &recorder;
  BraveMojoWatcherId watcher = BraveMojoWatch(pipe, ^(BraveMojoResult result) {
    recorder_ptr->results.push_back(result);
    while (ReadMessage(pipe).result == BraveMojoResultOk) {
    }
  });
  ASSERT_NE(watcher, 0u);

  const uint8_t kPayload[] = {1};
  ASSERT_EQ(WriteMessage(handle0_, kPayload), BraveMojoResultOk);
  ASSERT_TRUE(
      base::test::RunUntil([&] { return recorder.results.size() == 1; }));

  ASSERT_EQ(WriteMessage(handle0_, kPayload), BraveMojoResultOk);
  ASSERT_TRUE(
      base::test::RunUntil([&] { return recorder.results.size() == 2; }));
  EXPECT_EQ(recorder.results,
            std::vector<BraveMojoResult>(2, BraveMojoResultOk));
  BraveMojoCancelWatch(watcher);
}

TEST_F(BraveMojoSystemTest, WatchNotifiesPeerClosed) {
  base::RunLoop run_loop;
  WatchRecorder recorder{.on_notified = run_loop.QuitClosure()};
  WatchRecorder* recorder_ptr = &recorder;
  BraveMojoWatcherId watcher =
      BraveMojoWatch(handle1_, ^(BraveMojoResult result) {
        recorder_ptr->results.push_back(result);
        if (recorder_ptr->on_notified) {
          std::move(recorder_ptr->on_notified).Run();
        }
      });
  ASSERT_NE(watcher, 0u);

  BraveMojoClose(TakeHandle0());
  run_loop.Run();

  ASSERT_FALSE(recorder.results.empty());
  EXPECT_EQ(recorder.results[0], BraveMojoResultOk);
  EXPECT_EQ(ReadMessage(handle1_).result, BraveMojoResultFailedPrecondition);
  BraveMojoCancelWatch(watcher);
}

TEST_F(BraveMojoSystemTest, WatchNotifiesCancelledWhenHandleClosed) {
  base::RunLoop run_loop;
  WatchRecorder recorder{.on_notified = run_loop.QuitClosure()};
  WatchRecorder* recorder_ptr = &recorder;
  BraveMojoWatcherId watcher =
      BraveMojoWatch(handle1_, ^(BraveMojoResult result) {
        recorder_ptr->results.push_back(result);
        if (recorder_ptr->on_notified) {
          std::move(recorder_ptr->on_notified).Run();
        }
      });
  ASSERT_NE(watcher, 0u);

  BraveMojoClose(TakeHandle1());
  run_loop.Run();

  EXPECT_EQ(recorder.results,
            std::vector<BraveMojoResult>({BraveMojoResultCancelled}));
  BraveMojoCancelWatch(watcher);
}

TEST_F(BraveMojoSystemTest, CancelWatchReleasesCallback) {
  WatchRecorder recorder;
  __weak NSObject* weak_token = nil;
  BraveMojoWatcherId watcher =
      WatchWithToken(handle1_, &recorder, nullptr, &weak_token);
  ASSERT_NE(watcher, 0u);
  EXPECT_NE(weak_token, nil);

  // Releasing the callback guarantees it is never invoked again.
  BraveMojoCancelWatch(watcher);
  EXPECT_EQ(weak_token, nil);

  // Cancelling again, or cancelling an unknown watcher, is a no-op.
  BraveMojoCancelWatch(watcher);
  BraveMojoCancelWatch(0);
  EXPECT_TRUE(recorder.results.empty());
}

TEST_F(BraveMojoSystemTest, CancelWatchFromWithinCallback) {
  WatchRecorder recorder;
  __weak NSObject* weak_token = nil;
  BraveMojoWatcherId watcher = 0;
  watcher = WatchWithToken(handle1_, &recorder, &watcher, &weak_token);
  ASSERT_NE(watcher, 0u);
  EXPECT_NE(weak_token, nil);

  // The message is intentionally left unread, so a watcher that was still
  // armed would keep notifying.
  const uint8_t kPayload[] = {1};
  ASSERT_EQ(WriteMessage(handle0_, kPayload), BraveMojoResultOk);
  EXPECT_TRUE(base::test::RunUntil([&] { return weak_token == nil; }));
  EXPECT_EQ(recorder.results,
            std::vector<BraveMojoResult>({BraveMojoResultOk}));
}

TEST_F(BraveMojoSystemTest, WatchInvalidHandleFails) {
  EXPECT_EQ(BraveMojoWatch(0,
                           ^(BraveMojoResult){
                           }),
            0u);
}
