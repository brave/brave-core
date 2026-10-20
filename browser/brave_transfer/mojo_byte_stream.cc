// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/brave_transfer/mojo_byte_stream.h"

#include <algorithm>
#include <utility>

#include "base/containers/to_vector.h"
#include "base/functional/bind.h"
#include "base/location.h"
#include "base/task/sequenced_task_runner.h"
#include "net/base/net_errors.h"

namespace brave_transfer {

namespace {

// Pending bytes already written are dropped from the front of the buffer once
// there are more than this many.
constexpr size_t kCompactThreshold = 64 * 1024;

}  // namespace

MojoByteStream::MojoByteStream(
    mojo::PendingRemote<network::mojom::TCPConnectedSocket> socket,
    mojo::PendingReceiver<network::mojom::SocketObserver> observer,
    mojo::ScopedDataPipeConsumerHandle receive_stream,
    mojo::ScopedDataPipeProducerHandle send_stream)
    : receive_stream_(std::move(receive_stream)),
      send_stream_(std::move(send_stream)),
      readable_watcher_(FROM_HERE,
                        mojo::SimpleWatcher::ArmingPolicy::AUTOMATIC,
                        base::SequencedTaskRunner::GetCurrentDefault()),
      writable_watcher_(FROM_HERE,
                        mojo::SimpleWatcher::ArmingPolicy::MANUAL,
                        base::SequencedTaskRunner::GetCurrentDefault()) {
  if (socket) {
    socket_.Bind(std::move(socket));
    socket_.set_disconnect_handler(
        base::BindOnce(&MojoByteStream::Finish, base::Unretained(this), true));
  }
  if (observer) {
    observer_receiver_.Bind(std::move(observer));
  }
  if (send_stream_) {
    writable_watcher_.Watch(send_stream_.get(), MOJO_HANDLE_SIGNAL_WRITABLE,
                            base::BindRepeating(&MojoByteStream::OnWritable,
                                                base::Unretained(this)));
  } else {
    send_failed_ = true;
  }
  if (!receive_stream_) {
    Finish(true);
  }
}

MojoByteStream::~MojoByteStream() = default;

void MojoByteStream::Start(ReadCallback on_read, ClosedCallback on_closed) {
  CHECK(!on_read_);
  on_read_ = std::move(on_read);
  on_closed_ = std::move(on_closed);
  if (finished_) {
    RunClosedCallback();
    return;
  }
  readable_watcher_.Watch(
      receive_stream_.get(), MOJO_HANDLE_SIGNAL_READABLE,
      base::BindRepeating(&MojoByteStream::OnReadable, base::Unretained(this)));
}

void MojoByteStream::Write(base::span<const uint8_t> data) {
  if (send_failed_ || close_write_requested_ || data.empty()) {
    return;
  }
  pending_.insert(pending_.end(), data.begin(), data.end());
  if (!Flush()) {
    Finish(true);
  }
}

size_t MojoByteStream::BufferedBytes() const {
  return pending_.size() - pending_offset_;
}

void MojoByteStream::SetWritableCallback(base::RepeatingClosure on_writable) {
  on_writable_ = std::move(on_writable);
}

void MojoByteStream::CloseWrite() {
  close_write_requested_ = true;
  MaybeFinishWrite();
}

void MojoByteStream::OnReadError(int32_t net_error) {
  if (net_error != net::OK) {
    Finish(true);
  }
}

void MojoByteStream::OnWriteError(int32_t net_error) {
  Finish(true);
}

void MojoByteStream::OnReadable(MojoResult result) {
  if (finished_) {
    return;
  }
  if (result != MOJO_RESULT_OK) {
    Finish(false);
    return;
  }
  base::span<const uint8_t> buffer;
  MojoResult read_result =
      receive_stream_->BeginReadData(MOJO_READ_DATA_FLAG_NONE, buffer);
  if (read_result == MOJO_RESULT_SHOULD_WAIT) {
    return;
  }
  if (read_result != MOJO_RESULT_OK) {
    Finish(false);
    return;
  }
  const size_t size = std::min(buffer.size(), kMaxReadSize);
  std::vector<uint8_t> data = base::ToVector(buffer.first(size));
  receive_stream_->EndReadData(size);

  on_read_.Run(data);
}

void MojoByteStream::OnWritable(MojoResult result) {
  if (send_failed_) {
    return;
  }
  if (result != MOJO_RESULT_OK) {
    Finish(true);
    return;
  }
  const size_t before = BufferedBytes();
  if (!Flush()) {
    Finish(true);
    return;
  }
  if (BufferedBytes() < before && on_writable_) {
    on_writable_.Run();
  }
}

bool MojoByteStream::Flush() {
  while (pending_offset_ < pending_.size()) {
    size_t written = 0;
    MojoResult result =
        send_stream_->WriteData(base::span(pending_).subspan(pending_offset_),
                                MOJO_WRITE_DATA_FLAG_NONE, written);
    if (result == MOJO_RESULT_SHOULD_WAIT) {
      if (pending_offset_ > kCompactThreshold) {
        pending_.erase(pending_.begin(), pending_.begin() + pending_offset_);
        pending_offset_ = 0;
      }
      writable_watcher_.ArmOrNotify();
      return true;
    }
    if (result != MOJO_RESULT_OK) {
      return false;
    }
    pending_offset_ += written;
  }
  pending_.clear();
  pending_offset_ = 0;
  MaybeFinishWrite();
  return true;
}

void MojoByteStream::MaybeFinishWrite() {
  if (!close_write_requested_ || BufferedBytes() > 0 || !send_stream_) {
    return;
  }
  writable_watcher_.Cancel();
  send_stream_.reset();
}

void MojoByteStream::Finish(bool error) {
  if (error) {
    send_failed_ = true;
    writable_watcher_.Cancel();
    pending_.clear();
    pending_offset_ = 0;
  }
  if (finished_) {
    return;
  }
  finished_ = true;
  closed_with_error_ = error;
  readable_watcher_.Cancel();
  if (on_closed_) {
    RunClosedCallback();
  }
}

void MojoByteStream::RunClosedCallback() {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(
          [](base::WeakPtr<MojoByteStream> self) {
            if (self && self->on_closed_) {
              std::move(self->on_closed_).Run(self->closed_with_error_);
            }
          },
          weak_factory_.GetWeakPtr()));
}

}  // namespace brave_transfer
