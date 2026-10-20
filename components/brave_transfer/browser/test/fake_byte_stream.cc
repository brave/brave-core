// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_transfer/browser/test/fake_byte_stream.h"

#include "base/containers/to_vector.h"
#include "base/functional/bind.h"
#include "base/task/sequenced_task_runner.h"

namespace brave_transfer {

// static
std::pair<std::unique_ptr<FakeByteStream>, std::unique_ptr<FakeByteStream>>
FakeByteStream::CreatePair() {
  auto a = std::make_unique<FakeByteStream>();
  auto b = std::make_unique<FakeByteStream>();
  a->peer_ = b->weak_factory_.GetWeakPtr();
  b->peer_ = a->weak_factory_.GetWeakPtr();
  return {std::move(a), std::move(b)};
}

FakeByteStream::FakeByteStream() = default;

FakeByteStream::~FakeByteStream() {
  if (peer_ && !write_closed_) {
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE,
        base::BindOnce(&FakeByteStream::OnRemoteClosed, peer_, false));
  }
}

void FakeByteStream::Start(ReadCallback on_read, ClosedCallback on_closed) {
  on_read_ = std::move(on_read);
  on_closed_ = std::move(on_closed);
  started_ = true;
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(&FakeByteStream::Flush, weak_factory_.GetWeakPtr()));
}

void FakeByteStream::Write(base::span<const uint8_t> data) {
  if (write_closed_ || !peer_) {
    return;
  }
  bytes_written_ += data.size();
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(&FakeByteStream::Deliver, peer_, base::ToVector(data)));
}

size_t FakeByteStream::BufferedBytes() const {
  return 0;
}

void FakeByteStream::SetWritableCallback(base::RepeatingClosure on_writable) {}

void FakeByteStream::CloseWrite() {
  if (write_closed_) {
    return;
  }
  write_closed_ = true;
  if (peer_) {
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE,
        base::BindOnce(&FakeByteStream::OnRemoteClosed, peer_, false));
  }
}

void FakeByteStream::SimulateError() {
  write_closed_ = true;
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&FakeByteStream::OnRemoteClosed,
                                weak_factory_.GetWeakPtr(), true));
  if (peer_) {
    peer_->write_closed_ = true;
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE,
        base::BindOnce(&FakeByteStream::OnRemoteClosed, peer_, true));
  }
}

void FakeByteStream::Deliver(std::vector<uint8_t> data) {
  pending_.push_back(std::move(data));
  Flush();
}

void FakeByteStream::OnRemoteClosed(bool error) {
  if (!pending_close_) {
    pending_close_ = error;
  }
  Flush();
}

void FakeByteStream::Flush() {
  if (!started_) {
    return;
  }
  auto weak = weak_factory_.GetWeakPtr();
  while (!pending_.empty()) {
    std::vector<uint8_t> data = std::move(pending_.front());
    pending_.pop_front();
    on_read_.Run(data);
    if (!weak) {
      return;
    }
  }
  if (pending_close_ && on_closed_) {
    std::move(on_closed_).Run(*pending_close_);
  }
}

}  // namespace brave_transfer
