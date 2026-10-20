// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/brave_transfer/mojo_byte_stream.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/span.h"
#include "base/functional/bind.h"
#include "base/run_loop.h"
#include "base/strings/string_view_util.h"
#include "base/test/run_until.h"
#include "base/test/task_environment.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "mojo/public/cpp/system/data_pipe_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_transfer {

namespace {

constexpr size_t kPipeCapacity = 1024;

}  // namespace

class BraveTransferMojoByteStreamTest : public testing::Test {
 protected:
  void SetUp() override {
    mojo::ScopedDataPipeConsumerHandle receive_consumer;
    ASSERT_EQ(mojo::CreateDataPipe(kPipeCapacity, network_side_receive_,
                                   receive_consumer),
              MOJO_RESULT_OK);
    mojo::ScopedDataPipeProducerHandle send_producer;
    ASSERT_EQ(
        mojo::CreateDataPipe(kPipeCapacity, send_producer, network_side_send_),
        MOJO_RESULT_OK);
    socket_receiver_ = socket_.InitWithNewPipeAndPassReceiver();
    stream_ = std::make_unique<MojoByteStream>(
        std::move(socket_), observer_.InitWithNewPipeAndPassReceiver(),
        std::move(receive_consumer), std::move(send_producer));
  }

  void StartStream() {
    stream_->Start(base::BindRepeating(
                       [](std::string* received, base::span<const uint8_t> d) {
                         received->append(base::as_string_view(d));
                       },
                       &received_),
                   base::BindOnce([](std::optional<bool>* closed,
                                     bool error) { *closed = error; },
                                  &closed_with_error_));
  }

  std::string ReadSent(size_t size) {
    std::string data;
    while (data.size() < size) {
      base::span<const uint8_t> buffer;
      MojoResult result =
          network_side_send_->BeginReadData(MOJO_READ_DATA_FLAG_NONE, buffer);
      if (result == MOJO_RESULT_SHOULD_WAIT) {
        base::RunLoop().RunUntilIdle();
        continue;
      }
      EXPECT_EQ(result, MOJO_RESULT_OK);
      if (result != MOJO_RESULT_OK) {
        break;
      }
      data.append(base::as_string_view(buffer));
      network_side_send_->EndReadData(buffer.size());
    }
    return data;
  }

  base::test::TaskEnvironment task_environment_;
  mojo::PendingRemote<network::mojom::TCPConnectedSocket> socket_;
  mojo::PendingReceiver<network::mojom::TCPConnectedSocket> socket_receiver_;
  mojo::PendingRemote<network::mojom::SocketObserver> observer_;
  mojo::ScopedDataPipeProducerHandle network_side_receive_;
  mojo::ScopedDataPipeConsumerHandle network_side_send_;
  std::unique_ptr<MojoByteStream> stream_;
  std::string received_;
  std::optional<bool> closed_with_error_;
};

TEST_F(BraveTransferMojoByteStreamTest, ReadsAndWrites) {
  StartStream();
  ASSERT_TRUE(mojo::BlockingCopyFromString("hello", network_side_receive_));
  ASSERT_TRUE(base::test::RunUntil([&] { return received_ == "hello"; }));

  const std::string message = "from the Mac";
  stream_->Write(base::as_byte_span(message));
  EXPECT_EQ(ReadSent(message.size()), message);
  EXPECT_FALSE(closed_with_error_);
}

TEST_F(BraveTransferMojoByteStreamTest, BuffersWhilePipeIsFull) {
  StartStream();
  int writable_calls = 0;
  stream_->SetWritableCallback(
      base::BindRepeating([](int* calls) { ++*calls; }, &writable_calls));
  const std::string large(kPipeCapacity * 4, 'x');
  stream_->Write(base::as_byte_span(large));
  EXPECT_GT(stream_->BufferedBytes(), 0u);
  EXPECT_EQ(writable_calls, 0);

  EXPECT_EQ(ReadSent(large.size()), large);
  ASSERT_TRUE(
      base::test::RunUntil([&] { return stream_->BufferedBytes() == 0; }));
  EXPECT_GT(writable_calls, 0);
}

TEST_F(BraveTransferMojoByteStreamTest, PeerEndOfStreamIsGraceful) {
  StartStream();
  ASSERT_TRUE(mojo::BlockingCopyFromString("bye", network_side_receive_));
  network_side_receive_.reset();
  ASSERT_TRUE(
      base::test::RunUntil([&] { return closed_with_error_.has_value(); }));
  EXPECT_FALSE(*closed_with_error_);
  EXPECT_EQ(received_, "bye");

  const std::string reply = "still writable";
  stream_->Write(base::as_byte_span(reply));
  EXPECT_EQ(ReadSent(reply.size()), reply);
}

TEST_F(BraveTransferMojoByteStreamTest, CloseWriteEndsSendPipeAfterFlush) {
  StartStream();
  const std::string data(kPipeCapacity * 2, 'y');
  stream_->Write(base::as_byte_span(data));
  stream_->CloseWrite();
  EXPECT_EQ(ReadSent(data.size()), data);
  ASSERT_TRUE(base::test::RunUntil([&] {
    base::span<const uint8_t> buffer;
    return network_side_send_->BeginReadData(MOJO_READ_DATA_FLAG_NONE,
                                             buffer) ==
           MOJO_RESULT_FAILED_PRECONDITION;
  }));
}

TEST_F(BraveTransferMojoByteStreamTest, SocketDisconnectIsAnError) {
  StartStream();
  socket_receiver_.reset();
  ASSERT_TRUE(
      base::test::RunUntil([&] { return closed_with_error_.has_value(); }));
  EXPECT_TRUE(*closed_with_error_);
}

TEST_F(BraveTransferMojoByteStreamTest, CloseBeforeStartIsReported) {
  network_side_receive_.reset();
  socket_receiver_.reset();
  base::RunLoop().RunUntilIdle();
  StartStream();
  EXPECT_FALSE(closed_with_error_);
  ASSERT_TRUE(
      base::test::RunUntil([&] { return closed_with_error_.has_value(); }));
  EXPECT_TRUE(*closed_with_error_);
}

}  // namespace brave_transfer
